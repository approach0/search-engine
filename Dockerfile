## base environment
FROM debian:buster

### for GFW ###
#RUN sed -i s@/deb.debian.org/@/mirrors.aliyun.com/@g /etc/apt/sources.list

RUN apt-get update
RUN mkdir -p /code
ADD . /code/a0

## building environment
RUN apt-get install -y --no-install-recommends git build-essential g++ cmake wget python3 flex bison
RUN apt-get install -y --no-install-recommends libz-dev libevent-dev libopenmpi-dev libxml2-dev libfl-dev
RUN git config --global http.sslVerify false

### for GFW ###
#RUN git clone --progress --depth=1 https://gitee.com/t-k-/fork-indri.git /code/indri
RUN git clone --progress --depth=1 https://github.com/approach0/fork-indri.git /code/indri

RUN (cd /code/indri && ./configure && make)

### for GFW ###
#RUN mv /code/a0/cppjieba.tar.gz /code/
RUN wget --no-check-certificate 'https://github.com/yanyiwu/cppjieba/archive/v4.8.1.tar.gz' -O /code/cppjieba.tar.gz

RUN mkdir -p /code/cppjieba
RUN tar -xzf /code/cppjieba.tar.gz -C /code/cppjieba --strip-components=1
WORKDIR /code/a0
RUN ./configure --indri-path=/code/indri --jieba-path=/code/cppjieba
RUN export TERM=xterm-256color; make clean && make
RUN ln -sf `pwd`/indexerd/run/indexerd.out /usr/bin/indexer.out
RUN ln -sf `pwd`/searchd/run/searchd.out /usr/bin/searchd.out
RUN ln -sf `pwd`/indexerd/scripts/vdisk-creat.sh /usr/bin/vdisk-creat.sh
RUN ln -sf `pwd`/indexerd/scripts/vdisk-mount.sh /usr/bin/vdisk-mount.sh

## Enable sshd and config ssh client
RUN apt-get install -y --no-install-recommends openssh-server
RUN mkdir -p /var/run/sshd

RUN ssh-keygen -f ~/.ssh/id_rsa -t rsa -N ''
RUN cat ~/.ssh/id_rsa.pub > ~/.ssh/authorized_keys
RUN sed -i "/StrictHostKeyChecking/c StrictHostKeyChecking no" /etc/ssh/ssh_config

##CMD searchd.out -h
##CMD indexer.out -h
