## base environment
FROM ubuntu:16.04
RUN apt update
RUN apt install -y --no-install-recommends git build-essential g++ cmake wget python3 libxml2-dev
RUN apt install -y --no-install-recommends bison flex libz-dev libevent-dev libopenmpi-dev reiserfsprogs

## application environment
ADD . /code/a0
RUN git config --global http.sslVerify false
RUN git clone https://github.com/approach0/fork-indri.git /code/indri
RUN (cd /code/indri && ./configure && make)
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

#CMD /usr/bin/indexer.out
#CMD /usr/bin/searchd.out
