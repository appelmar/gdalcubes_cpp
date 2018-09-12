FROM ubuntu:xenial
MAINTAINER Marius Appel <marius.appel@uni-muenster.de>

RUN apt-get update && apt-get install -y software-properties-common libboost-all-dev cmake g++ libsqlite3-dev git
RUN add-apt-repository -y ppa:ubuntugis/ppa && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 314DF160 && apt-get update && apt-get install -y libgdal-dev gdal-bin libproj-dev

RUN git clone https://github.com/luciad/libgpkg && cd libgpkg && mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ../ && make -j2 && make install


COPY . /opt/gdalcubes
WORKDIR /opt/gdalcubes
RUN mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ../ && make -j 2