# gdalcubes - Earth observation data cubes from GDAL image collections

**gdalcubes** is a library and toolkit to represent collections of Earth Observation (EO) images
as on-demand data cubes (or _multidimensional arrays_). It presents a single model how multitemporal and multispectral 
imagery can be processed and streamed into external programs such as R or Python on local computers or distributed cloud environments. 
gdalcubes is not a database, i.e., it does not need to store additional copies of the imagery but instead
simply links to and indexes existing files / GDAL datasets. It is written in C++ and includes a command line interface as well as a package for R. A python package is
planned for the future. gdalcubes is licensed under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0).

Core features:

- Create image collections that link to and index existing files / cloud storage 
- Process multitemporal, multispectral image collections as on demand data cubes with desired spatiotemporal resolution, extent, and projection.
- Stream chunks of data cubes to external programs (R, python)
- Scale processing in distributed environments with `gdalcubes_server` and Docker

gdalcubes alone does **not** provide a rich set of algorithms besides reducing data-cubes over time and filtering by space, time, and bands. 
However, streaming chunks of cubes into external programs allows to apply e.g. arbitrary R or Python code on the data with little amount of reimplementations needed.     



# Objectives

- Proposing a simple image collection file format to reference many images on local disk, remote servers, or in the cloud. 
- Extend GDAL to work with multi-temporal, multi-spectral imagery.
- Read image collection data as on demand data cubes and stream data chunks into common analysis tools such as R or Python. 
- Provide simple possibilities to scale analysis to several machines  (e.g. by using Docker for deployment).



# Installation


## Installation from sources

gdalcubes uses CMake and can be compiled with a typical CMake workflow sa listed below.

```
git clone https://github.com/appelmar/gdalcubes && cd gdalcubes
mkdir -p build 
cd build 
cmake -DCMAKE_BUILD_TYPE=Release ../ 
make 
make install
```

## Docker image
This repository includes a Docker image which you can use either to run the gdalcubes command line interface interactively
or to run gdalcubes_server as a service for distribute processing. The commands below demonstrate how to build the image and run a container.
 

```
git clone https://github.com/appelmar/gdalcubes && cd gdalcubes 
docker build -t appelmar/gdalcubes .
docker run -d -p 11111:1111 appelmar/gdalcubes # runs gdalcubes_server as a deamon 
docker run appelmar/gdalcubes /bin/bash # get a command line where you can run gdalcubes 
``` 


# Command line interface
The CLI can be used to create image collections (`gdalcubes create_collection`), to print information about existing 
image collections ('gdalcubes info`), to stream chunks of image collection data cubes to external tools (gdalcubes stream), and to
reduce image collection data cubes over time. More functionality, e.g. to update existing image collections, will be added in the future. 


# R package
The R package is currently under development. You can install the current version using the `devtools` package as below.

```
library(devtools)
install_github("appelmar/gdalcubes", subdir="Rpkg/gdalcubes")
```


# Tutorial
Further documentation including a tutorial with different datasets can be found at http://appelmar.github.io/gdalcubes.
