# Earth observation data cubes from GDAL image collections


![](gdalcubes_logo_1_medium.png)

## Introduction

**gdalcubes** is a library to represent collections of Earth Observation (EO) images
as on-demand data cubes (or _multidimensional arrays_). It presents a model how multitemporal and multispectral 
imagery can be processed and streamed into external programs such as R or Python on local computers or distributed cloud environments. 
gdalcubes is not a database, i.e., it does not need to store additional copies of the imagery but instead
simply links to and indexes existing files / GDAL datasets. It is written in C++ and includes a command line interface as well as a package for R. A python package is
planned for the future. gdalcubes is licensed under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0).

Core features:

- Create image collections that link to and index existing imagery from local files or cloud storage 
- Read multitemporal, multispectral image collections as on demand data cubes with desired spatiotemporal resolution, extent, and projection
- Stream chunks of data cubes to external programs (R, python)
- Scale computations on data cubes in distributed environments with `gdalcubes_server` and Docker

gdalcubes alone does **not** provide a rich set of algorithms besides reducing data-cubes over time and filtering by space, time, and bands. 
However, streaming chunks of cubes into external programs allows to apply e.g. arbitrary R or Python code on the data with little amount of reimplementations needed.     


## Warning
The library is still in an early development version. Major changes are possible to make gdalcubes more user-friendly, more stable, faster, and more robust.
The documentation is also preliminary and far from being complete.

   