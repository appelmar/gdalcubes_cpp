
# Concepts 

This page describes the basic concepts and terms used by gdalcubes. Some of the definitions below may vary from
other libraries. 

## Image collections

An _image collection_ is simply a set of images where all images contain identical variables (_bands_). 
Images have a spatial footprint and a recording date/time. Values of the bands from one image may be stored in a single 
or in several files and bands may also differ in their spatial resolution. Two different images in a collection may have different map projections.  
  
 The figure below illustrates the basic structure: an image collection has $n$ images, images each have a spatial extent, recording date/time, and a map projection and 9 bands, where the band data come from three files per image.
 ![](ic_structure.png)



## GDAL dataset references
In gdalcubes, anything that is readable by GDAL may contain actual band data of an image. Typically, this will be local files, 
but actually can be anything that [the `gdalinfo` command](https://www.gdal.org/gdalinfo.html) understands, including
cloud storage, databases, and archive files (see below) through [GDAL virtual file systems](https://www.gdal.org/gdal_virtual_file_systems.html).
Instead of _files_ we therefore often use the terms _GDAL dataset reference_ or _GDAL dataset descriptor_.

```
gdalinfo /vsizip/my.zip/my.tif  # file in a .zip archive
gdalinfo test.tif               # local file
gdalinfo /vsicurl/https://download.osgeo.org/geotiff/samples/spot/chicago/UTM2GTIF.TIF # file on a HTTP server
``` 

## Image collections are not data cubes




## Creating image collection files
gdalcubes has its own file format to store an index including links to GDAL dataset descriptors and define how 
they relate images and bands in a collection. Image collection files are simple [SQLite](https://www.sqlite.org/index.html)
databases with three important tables `bands`, `images`, and `gdalrefs`. 




---


## JSON collection format description

## Sharing image collections



# Data cube views

Data cube views (or simply _views_) convert an image collection to a data cube by defining the 
basic shape of the cube, i.e. how we look at the data. It includes
 
- the spatiotemporal extent, 
- the spatial reference / map projection, 
- the spatiotemporal resolution either by number of or by the size of cells,
- a resampling algorithm used in [`gdalwarp`](https://www.gdal.org/gdalwarp.html), and 
- a temporal aggregation algorithm that combines values of several image if they are located in the same cell of the target cube 

Views can be serialized as simple JSON object as in the example below.

example_view.json
```
{
  "aggregation" : "none",
  "resampling" : "near",
  "space" :
  {
    "left" : 22.9,
    "right" : 23.1,
    "top" : -18.9,
    "bottom" : -19.1,
    "proj" : "EPSG:4326",
    "nx" : 500,
    "ny" : 500
  },
  "time" :
  {
    "t0" : "2017-01-01",
    "t1" : "2018-01-01",
    "dt" : "P1M"
  }
}
```



---

# Streaming data cube chunks to external programs (R, Python)







---

# Distributed image collection processing

## Running gdalcubes as a server

- command line interface
- Docker image 

## Defining a swarm



## Execution context



## API details

Internally, gdalcubes_server provides a REST-like API to cubes