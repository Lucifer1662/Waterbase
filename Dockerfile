FROM ubuntu
RUN apt-get -y update
RUN apt-get -y install cmake 
RUN apt-get -y install curl
RUN apt-get install -y build-essential

WORKDIR /temp

RUN curl -O https://capnproto.org/capnproto-c++-0.9.1.tar.gz
RUN tar zxf capnproto-c++-0.9.1.tar.gz
WORKDIR /temp/capnproto-c++-0.9.1
RUN ./configure
RUN make -j6
RUN make install
WORKDIR /temp

RUN apt-get install -y g++ python2-dev autotools-dev libicu-dev libbz2-dev 
RUN apt-get install -y wget
RUN wget -O boost_1_78_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.78.0/boost_1_78_0.tar.gz/download
RUN tar xzvf boost_1_78_0.tar.gz
WORKDIR /temp/boost_1_78_0
RUN ./bootstrap.sh --prefix=/usr/local
RUN ./b2 --with-system --with-thread --with-date_time --with-regex --with-serialization stage
RUN ./b2 install
WORKDIR /temp

# RUN rm -rf /temp

RUN apt-get install -y pkg-config


RUN mkdir -p Waterbase


