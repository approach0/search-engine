## base environment
FROM debian:buster
#RUN sed -i s@/deb.debian.org/@/mirrors.aliyun.com/@g /etc/apt/sources.list
RUN apt-get update
RUN mkdir -p /code

## C/C++ environment
RUN apt-get install -y --no-install-recommends git build-essential g++ cmake wget python3 flex bison
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
### for crawlers
RUN apt-get install -y --no-install-recommends python3-pip python3-dev python3-setuptools libcurl4-openssl-dev libssl-dev
RUN cd demo/crawler && pip3 install -r requirements.txt
### for searchd / indexer
RUN ./configure --indri-path=/code/indri --jieba-path=/code/cppjieba
RUN export TERM=xterm-256color; make clean && make
### export to global commands
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
