# Installation


## Installation from sources

## Linux 
gdalcubes can be compiled from sources via [CMake](https://cmake.org/). CMake automatically checks for mandatory and optional dependencies and adapts
the build configuration. The following commands install gdalcubes from sources. 

```
git clone https://github.com/appelmar/gdalcubes && cd gdalcubes
mkdir -p build 
cd build 
cmake -DCMAKE_BUILD_TYPE=Release ../ 
make 
sudo make install
```

If any of required libraries are not available on your system, please use your package manager to install these before.
Using Ubuntu, `sudo apt-get install libgdal-dev libnetcdf-dev libcurl4-openssl-dev libsqlite3-dev` will install the libraries needed to compile 
the core gdalcubes library. If you want to compile the command line interface, you will furthermore need `sudo apt-get install libboost-program-options-dev libboost-system-dev`
and running gdalcubes as a server additionally requires `sudo apt-get install libcpprest-dev`.




### Windows 
The core library has been successfully compiled with a MinGW-w64 toolchain. The R package on Windows e.g. links to
prebuild dependencies from [rwinlib](https://github.com/rwinlib). Detailed instructions will be added soon. To use the R package,
however, you do not need to build the gdalcubes library before.

We have not yet tried to compile gdalcubes with Microsoft Visual Studio.


## Docker
The `Dockerfile` at the root of the project is built on a minimal Ubuntu installation but installs all dependencies and compiles 
gdalcubes from sources automatically. 


```
git clone https://github.com/appelmar/gdalcubes && cd gdalcubes 
docker build -t appelmar/gdalcubes .
docker run -d -p 11111:1111 appelmar/gdalcubes # runs gdalcubes_server as a deamon 
docker run appelmar/gdalcubes /bin/bash # get a command line where you can run gdalcubes 
``` 




















