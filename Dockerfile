## base environment
FROM ubuntu:16.04
RUN apt update
RUN apt install -y --no-install-recommends git
RUN mkdir -p /code
ADD . /code/a0

# building environment
RUN apt install -y --no-install-recommends build-essential g++ cmake wget python3 libxml2-dev flex bison
RUN apt install -y --no-install-recommends libz-dev libevent-dev libopenmpi-dev reiserfsprogs psmisc
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

## devops environment
RUN git clone -b deploy-ait https://github.com/approach0/panel.git /code/panel
RUN chmod +x /code/a0/node_setup_14.x.sh
RUN /code/a0/node_setup_14.x.sh
RUN apt install -y --no-install-recommends nodejs

RUN (cd /code/panel/proxy && npm install)
RUN (cd /code/panel/jobd && npm install)
RUN (cd /code/panel/ui && npm install && npm run build)

CMD ./setup-panel.sh
