## build environment
FROM debian:buster AS builder
RUN sed -i s@/deb.debian.org/@/mirrors.aliyun.com/@g /etc/apt/sources.list
RUN sed -i s@/security.debian.org/@/mirrors.aliyun.com/@g /etc/apt/sources.list
RUN apt-get update
RUN mkdir -p /code

## C/C++ and building environment
RUN apt-get install -y --no-install-recommends git build-essential g++ cmake wget flex bison python3
RUN apt-get install -y --no-install-recommends libz-dev libevent-dev libopenmpi-dev libxml2-dev libfl-dev
RUN git config --global http.sslVerify false

## Build Indri
RUN git clone --progress --depth=1 https://github.com/approach0/fork-indri.git /code/indri
RUN (cd /code/indri && ./configure && make)

## Download CppJieba header files
RUN wget --no-check-certificate 'https://github.com/yanyiwu/cppjieba/archive/v4.8.1.tar.gz' -O /code/cppjieba.tar.gz
RUN mkdir -p /code/cppjieba
RUN tar -xzf /code/cppjieba.tar.gz -C /code/cppjieba --strip-components=1

## Build this project
ADD . /code/a0
WORKDIR /code/a0
RUN ./configure --indri-path=/code/indri --jieba-path=/code/cppjieba
RUN export TERM=xterm-256color; make clean && make

## Now second-stage build for production
FROM debian:buster
RUN sed -i s@/deb.debian.org/@/mirrors.aliyun.com/@g /etc/apt/sources.list
RUN sed -i s@/security.debian.org/@/mirrors.aliyun.com/@g /etc/apt/sources.list
RUN apt-get update
# necessary binaries and dynamic libraries (IMPORTANT: rsync is used to fetch index image from indexer_syncd)
RUN apt-get install -y --no-install-recommends build-essential flex bison python3 python3-pip rsync
RUN apt-get install -y --no-install-recommends libz-dev libevent-dev libopenmpi-dev libxml2-dev libfl-dev

### for searchd / indexer
RUN apt-get install -y --no-install-recommends reiserfsprogs curl
RUN pip3 install -i http://pypi.douban.com/simple/ --trusted-host=pypi.douban.com requests # for json-feeder

## Enable sshd and config ssh client
RUN apt-get install -y --no-install-recommends openssh-server
RUN mkdir -p /var/run/sshd
RUN ssh-keygen -f ~/.ssh/id_rsa -t rsa -N ''
RUN cat ~/.ssh/id_rsa.pub > ~/.ssh/authorized_keys
RUN sed -i "/StrictHostKeyChecking/c StrictHostKeyChecking no" /etc/ssh/ssh_config
RUN echo "ClientAliveInterval 30" | tee -a /etc/ssh/sshd_config
RUN echo "ClientAliveCountMax 12" | tee -a /etc/ssh/sshd_config
RUN echo "ServerAliveInterval 30" | tee -a /etc/ssh/ssh_config
RUN echo "ServerAliveCountMax 12" | tee -a /etc/ssh/ssh_config

## Copy binaries from first-stage build image
COPY --from=builder /code/a0/indices-v3/run/doc-lookup.out /usr/bin/doc-lookup.out
COPY --from=builder /code/a0/indexerd/run/indexerd.out /usr/bin/indexer.out
COPY --from=builder /code/a0/searchd/run/searchd.out /usr/bin/searchd.out

COPY --from=builder /code/a0/indexerd/scripts/vdisk-creat.sh /usr/bin/vdisk-creat.sh
COPY --from=builder /code/a0/indexerd/scripts/vdisk-mount.sh /usr/bin/vdisk-mount.sh
COPY --from=builder /code/a0/indexerd/scripts/json-feeder.py /usr/bin/json-feeder.py
COPY --from=builder /code/a0/searchd/scripts/test-query.sh  /usr/bin/test-query.sh
COPY --from=builder /code/a0/searchd/tests/query-valid.json /tmp/test-query.json

CMD doc-lookup.out -h
