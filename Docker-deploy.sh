#!/bin/bash
NUM_REPLICAS=3
NUM_SHARDS=4
SERVICE_NAME_PREFIX=test

MPI_NET_NAME=$SERVICE_NAME_PREFIX-overlay_network
HOST_INDEX_NAME=$SERVICE_NAME_PREFIX-index
MASTER_SERVICE_NAME=${SERVICE_NAME_PREFIX}-master
SLAVE_SERVICE_NAMES=$(
	for shard in `seq 1 $((NUM_SHARDS - 1))`; do
		echo "${SERVICE_NAME_PREFIX}-slave-shard${shard}";
	done
)
MPI_HOSTLIST=$(echo $MASTER_SERVICE_NAME $SLAVE_SERVICE_NAMES | tr ' ' ',')

swarm_node_init()
{
	echo '[info] create swarm master ...'
	docker swarm init

	echo '[info] create MPI overlay network ...'
	docker network rm $MPI_NET_NAME
	# overlay network can spread over multiple host machines
	docker network create --driver overlay $MPI_NET_NAME
	docker network inspect $MPI_NET_NAME
}

swarm_check_host_index()
{
	for i in `seq 0 $((NUM_SHARDS - 1))`; do
		dir=${HOST_INDEX_NAME}-$i
		echo "[info] checking host directory: $dir"
		if [ ! -e $dir ]; then
			echo "[err] $dir does not exists, abort."
			exit 1
		fi
	done
}

swarm_create_master_pods()
{
	echo '[info] create MPI master pods (for shard#0) ...'
	docker service rm $MASTER_SERVICE_NAME
	docker service create --network $MPI_NET_NAME \
		--publish published=8921,target=8921 --replicas $NUM_REPLICAS \
		--name $MASTER_SERVICE_NAME --hostname="{{.Service.Name}}-{{.Task.Slot}}" \
		--mount type=bind,source="$(pwd)/${HOST_INDEX_NAME}-0",destination=/mnt/index \
		approach0 /usr/sbin/sshd -D -E /var/log/sshd.log
}

swarm_create_slave_pods()
{
	shard=1
	for slave_service_name in $SLAVE_SERVICE_NAMES; do
		echo "[info] create MPI slave pods for shard#${shard} ..."
		docker service rm $slave_service_name
		docker service create --network $MPI_NET_NAME --replicas $NUM_REPLICAS \
			--name $slave_service_name --hostname="{{.Service.Name}}-{{.Task.Slot}}" \
			--mount type=bind,source="$(pwd)/${HOST_INDEX_NAME}-${shard}",destination=/mnt/index \
			approach0 /usr/sbin/sshd -D -E /var/log/sshd.log
		let 'shard = shard + 1'
	done
}

swarm_run_mpi()
{
	mpi_net_space=$(docker network inspect $MPI_NET_NAME -f '{{(index .IPAM.Config 0).Subnet}}')
	master_pod=$(docker ps -q --filter "name=${MASTER_SERVICE_NAME}" | tail -1)

	echo '=== MPI run ==='
	echo $mpi_net_space
	echo docker exec -it $master_pod bash
	echo $MPI_HOSTLIST
	echo '==============='

	#exe="hostname"
	exe="searchd.out -i /mnt/index -c 0 -C 0"

	for replica in `seq 1 $NUM_REPLICAS`; do
		echo "[info] mpirun for replicate#${replica}..."

		# By default, OpenMPI will tries all the available subnets. If traffic on some subnets
		# are dropped (in docker network). We restrict both OOB (for initialization, termination)
		# and BTL (for MPI communication) to avoid this issue.
		docker exec --detach $master_pod \
			mpirun --allow-run-as-root --host $MPI_HOSTLIST -n $NUM_SHARDS --wdir /var/log/ \
			--mca oob_tcp_if_include $mpi_net_space --mca btl_tcp_if_include $mpi_net_space \
			sh -c "$exe"
		sleep 5
	done
}

swarm_view_logs()
{
	for pod in $(docker ps -q --no-trunc); do
		echo "=== ${pod} ==="
		docker exec $pod hostname
		docker exec $pod cat /var/log/searchd.log
	done
}

swarm_node_init
swarm_check_host_index
swarm_create_master_pods
swarm_create_slave_pods
swarm_run_mpi
echo "[info] wait for 10 seconds..."
sleep 10
swarm_view_logs
