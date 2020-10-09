IMAGE_NAME="approach0"

docker rm $(docker ps --all -q --filter ancestor="${IMAGE_NAME}")
docker image rm ${IMAGE_NAME}
docker images

docker build --network host --tag ${IMAGE_NAME} --no-cache .

echo "==== Use commands below to run or modify ===="
echo docker run --network host -v '`pwd`'/tmp:/mnt/index -it ${IMAGE_NAME} indexerd.out -o /mnt/index
echo docker run --network host -it ${IMAGE_NAME} /bin/bash
echo docker commit $(docker ps -q --latest) ${IMAGE_NAME}
