# gdalcubes - Earth observation data cubes from GDAL image collections

**gdalcubes** is a library to represent collections of Earth Observation (EO) images
as on-demand data cubes (or _multidimensional arrays_). It presents a model how multitemporal and multispectral 
imagery can be processed and streamed into external programs such as R or Python on local computers or cloud environments. 
gdalcubes is not a database, i.e., it does not need to store additional copies of the imagery but instead
simply links to and indexes existing files / GDAL datasets. It is written in C++ and includes a command line interface as well 
as a package for R. A python package is planned for the future. gdalcubes is licensed under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0).

# Core features

- Create image collections that link to and index existing files or cloud storage 
- Process multitemporal, multispectral image collections as on demand data cubes with desired spatiotemporal resolution, extent, and projection.
- Stream chunks of data cubes to external programs (R, python)
- Scale processing in distributed computing environments with `gdalcubes_server` and Docker

gdalcubes alone does **not** provide a rich set of algorithms besides reducing data-cubes over time, filtering by space, time, and bands, and applying
arithmetic expressions on pixels. However, streaming chunks of cubes into external programs allows to apply e.g. arbitrary R or Python code on the data with relatively little amount of reimplementations needed.     



# Objectives

- Proposing a simple image collection file format to reference many images on local disk, remote servers, or in the cloud. 
- Extend GDAL to work with multi-temporal, multi-spectral imagery.
- Read image collection data as on demand data cubes and stream data chunks into common analysis tools such as R or Python. 
- Provide simple possibilities to scale analysis to several machines  (e.g. by using Docker for deployment).



# Installation


## Installation from sources

gdalcubes uses CMake and can be compiled with a typical CMake workflow as listed below.

```
git clone https://github.com/appelmar/gdalcubes && cd gdalcubes
mkdir -p build 
cd build 
cmake -DCMAKE_BUILD_TYPE=Release ../ -DCMAKE_INSTALL_PREFIX=/usr
make 
make install
```

You might need to install a few libraries before compiling gdalcubes successfully. For the core library, you need to install development versions for:

On Ubuntu `apt install libgdal-dev libnetcdf-dev libcurl4-openssl-dev libsqlite3-dev` will install all libraries needed to compile 
the core gdalcubes library. If you want to compile the command line interface, you will furthermore need `apt install libboost-program-options-dev libboost-system-dev`
and running gdalcubes as a server additionally requires `apt install libcpprest-dev`.


## Docker image
This repository includes a Docker image which you can use either to run the gdalcubes command line interface interactively
or to run gdalcubes_server as a service for distributed processing. The commands below demonstrate how to build the image and run a container.
 

```
git clone https://github.com/appelmar/gdalcubes && cd gdalcubes 
docker build -t appelmar/gdalcubes .
docker run -d -p 11111:1111 appelmar/gdalcubes # runs gdalcubes_server as a deamon 
docker run appelmar/gdalcubes /bin/bash # get a command line where you can run gdalcubes 
``` 

The Dockerfile at `demo/Dockerfile` furthermore runs [RStudio Server](https://www.rstudio.com/products/rstudio-server/) with the gdalcubes R package.


# R package
The R package is currently under development. You can install the current version using the `devtools` package as below.


```
library(devtools)
install_github("appelmar/gdalcubes", subdir="Rpkg/gdalcubes")
```


# Tutorial
Further documentation including a tutorial with different datasets can be found at https://appelmar.github.io/gdalcubes.





# Credits

gdalcubes uses the following great open source libraries:

**[GDAL](https://www.gdal.org/):  A translator library for raster and vector geospatial data formats**

**[json](https://github.com/nlohmann/json): JSON for Modern C++**

**[SQLite](https://www.sqlite.org/): A self-contained, high-reliability, embedded, full-featured, public-domain, SQL database engine**

**[CURL](https://curl.haxx.se/): Command line tool and library for transferring data with URLs**

**[ExprTk](http://www.partow.net/programming/exprtk/): A C++ Mathematical Expression Parsing and Evaluation Library**
 
**[netCDF](https://www.unidata.ucar.edu/software/netcdf): The Unidata network Common Data Form C library**
   
**[tiny-process-library](https://gitlab.com/eidheim/tiny-process-library): A small platform independent library making it simple to create and stop new processes in C++**

**[Catch](https://www.gdal.org/): A modern, C++-native, header-only, test framework for unit-tests, TDD and BDD**
       
**[Date](https://github.com/HowardHinnant/date): A date and time library based on the C++11/14/17 <chrono> header**   

**[cpprestsdk](https://github.com/Microsoft/cpprestsdk)**

**[Boost.Filesystem](https://www.boost.org/doc/libs/1_68_0/libs/filesystem/doc/index.htm)**

**[Boost.Program_options](https://www.boost.org/doc/libs/1_68_0/doc/html/program_options.html)**
