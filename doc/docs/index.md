# Earth observation data cubes from GDAL image collections

<p align="center">
 <img style="width:30%" src="gdalcubes_logo_1_small.png" alt="gdalcubes logo"/>
</p>

## Introduction

**gdalcubes** is a library to represent collections of Earth Observation (EO) images
as _on-demand_ data cubes (or _multidimensional arrays_). Users define data cubes by spatiotemporal extent, resolution, and 
spatial reference system and let gdalcubes read only relevant parts of the data and simultaneously apply reprojection, resampling, and cropping (using [gdalwarp](https://www.gdal.org/gdalwarp.html)).
Data cubes may be simply exported as NetCDF files or directly streamed chunk-wise into external software such as R or Python. The library furthermore
implements simple operations to reduce data cubes over time, to apply pixel-wise arithmetic expressions, and to filter by space, time, and bands.

gdalcubes is not a database, i.e., it does not need to store additional copies of the imagery but instead
simply links to and indexes existing files / GDAL datasets, i.e. it can also directly access
data in cloud environments with [GDAL virtual file systems](https://www.gdal.org/gdal_virtual_file_systems.html).  

The library is written in C++ and includes a command line interface as well as a package for R. A python package is
planned for the future. gdalcubes is licensed under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0).

## Features

- Create image collections that link to and index existing imagery from local files or cloud storage 
- Read multitemporal, multispectral image collections as on demand data cubes with desired spatiotemporal resolution, extent, and map projection
- Abstract from complexities in the data like different map projections for adjacent images and different resolutions for different spectral bands
- Stream chunks of data cubes to external programs (e.g. R, python)
- Scale computations on data cubes in distributed environments with `gdalcubes_server` and Docker (experimental)


## Limitations

- Currently, gdalcubes can only read GDAL datasets that have an affine transformation and a reference system. It does not work with e.g. Sentinel 1 data that have ground control points or with geolocation arrays (curvilinear grids). At the moment, you have to warp these datasets manually (e.g. with a VRT dataset) before they can be used in gdalcubes. It is planned to solve this by adding custom gdalwarp parameters to be used when images are read as data cubes.  
- There is no functionaliy to read NetCDF data that have been created from gdalcubes. Loading data is currently only possible with GDAL. This will be addressed in a future release.
- There is no Python package yet. 

## Warning
The library is still in an early development version. Major changes are possible to make gdalcubes more user-friendly, more stable, faster, and more robust.
The documentation is also preliminary and far from being complete.

   