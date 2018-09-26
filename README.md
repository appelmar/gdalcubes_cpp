# gdalcubes - Earth observation data cubes from GDAL image collections

**_This project is in initial development_**

This C++ library and toolkit presents a data model for Earth observation image collections from many 
two-dimensional GDAL datasets. Gdalcubes strongly build on GDAL but differs by
- adding date/time to images,
- formalizing how multiple GDALDatasets  correspond to multi-band images,
- and supporting images with different projections in single image collections.

gdalcubes includes both, a data model for image collections as well as tools to stream data as multidimensional arrays
into external tools such as R.  


## gdalcubes in a nutshell

What is a _gdalcube_? Image collection + data view

What is an _image collection_? A bunch of GDAL datasets (e.g. image files) and their relation with regard to datetime and raster bands.

What is a _data view_? A spatial and temporal window + resolution + projection how we look at the data. 



# Objectives

- Proposing a simple image collection file format to reference many images on local disk, remote servers, or in the cloud. 
- Extent GDAL to work with multi-temporal, multi-spectral imagery.
- Read image collection data as on demand data cubes and stream data chunks into analysis tools such as R or Python. 
- Provide simple possibilities to scale analysis to several machines  (e.g. by using Docker for deployment).
- Demonstrate how to use gdalcubes for typical use cases. 


# Command line utilities

```
# Creating image collections
gdalcubes create_collection -f collection_format.json list.txt out.db 
gdalcubes create_collection -R -f collection_format.json /home/user/data/ out.db 


gdalcubes info in.db

gdalcubes join in1.db in2.db out.db 

gdalcubes stream -v view.json -t reduce_time -o result.db > Rscript timeseries_stats.R
gdalcubes stream -v view.json -t apply_pixel > Rscript timeseries_stats.R

gdalcubes reduce -r average -v view.json --gdal-of="Gtiff" --gdal-co="TILED=YES COMPRESS=JPEG PHOTOMETRIC=YCBCR" in.db out.tif 
``` 


# Creating image collections: The JSON collection format description



## Example: Local Sentinel 2 imagery

Consider the local archive of a few Sentinel images each in on of their directories

```
ls
> S2A_MSIL1C_20161213T093402_N0204_R136_T34UFD_20161213T093403.SAFE
> S2A_MSIL1C_20170101T082332_N0204_R121_T34KGD_20170101T084243.SAFE
> S2A_MSIL1C_20170111T082311_N0204_R121_T34KGD_20170111T084257.SAFE
> S2A_MSIL1C_20171002T094031_N0205_R036_T34UFD_20171002T094035.SAFE
> S2A_MSIL1C_20170131T082151_N0204_R121_T34KGD_20170131T084118.SAFE
> S2A_MSIL1C_20170829T081601_N0205_R121_T34KGD_20170829T083601.SAFE
> S2A_MSIL1C_20161222T082342_N0204_R121_T34KGD_20161222T083840.SAFE
> S2A_MSIL1C_20170411T081601_N0204_R121_T34KGD_20170411T083418.SAFE
> S2B_MSIL1C_20170726T125259_N0205_R138_T27WWM_20170726T125301.SAFE
> S2A_MSIL1C_20170815T102021_N0205_R065_T32TMR_20170815T102513.SAFE
> S2A_MSIL1C_20161212T082332_N0204_R121_T34KGD_20161212T084403.SAFE
```

Each of these directories stores the complete Sentinel 2 subfolder structure (see https://sentinel.esa.int/web/sentinel/user-guides/sentinel-2-msi/data-formats).
To automatically derive an image collection from our archive, we need to specify the structure as a collection format JSON file.


```
{
  "pattern" : ".+/IMG_DATA/.+\\.jp2",
  "images" : {
    "pattern" : ".*/(.+)\\.SAFE.*"
  },
  "datetime" : {
    "pattern" : ".*MSIL1C_(.+?)_.*",
    "format" : "%Y%m%dT%H%M%S"
  },
  "bands" : {
    "band1" : {
      "pattern" : ".+_B01\\.jp2"
    },
    "band2" : {
      "pattern" : ".+_B02\\.jp2"
    },
    "band3" : {
      "pattern" : ".+_B03\\.jp2"
    },
    "band4" : {
      "pattern" : ".+_B04\\.jp2"
    },
    "band5" : {
      "pattern" : ".+_B05\\.jp2"
    },
    "band6" : {
      "pattern" : ".+_B06\\.jp2"
    },
    "band7" : {
      "pattern" : ".+_B07\\.jp2"
    },
    "band8" : {
      "pattern" : ".+_B08\\.jp2"
    },
    "band8A" : {
      "pattern" : ".+_B8A\\.jp2"
    },
    "band9" : {
      "pattern" : ".+_B09\\.jp2"
    },
    "band10" : {
      "pattern" : ".+_B10\\.jp2"
    },
    "band11" : {
      "pattern" : ".+_B11\\.jp2"
    },
    "band12" : {
      "pattern" : ".+_B12\\.jp2"
    }
  }
}
```

This file involves a few regular expressions to extract datetime, bands from individual .jp2 files.




# Data Views

Data views define how we look at the EO imagery and how it is transformed to a data image_collection_cube (or multidimensional array). It includes an area of interest, the projection, and the temporal and spatial resolutions.
Views are serialized as JSON documents as in the example below:
```
{
  "space" :
  {
    "left" : 6.9,
    "right" : 7.1,
    "top" : 52.0,
    "bottom" : 51.8,
    "proj" : "EPSG:4326",
    "nx" : 100,
    "ny" : 100
  },
  "time" :
  {
    "t0" : "2016-01-01",
    "t1" : "2018-09-01",
    "dt" : "P6M"
  }
}
```


# Ongoing work (without priority)
- Test cases with some imagery
- Examples for cloud storage
- Unit tests for datetime
- gdalcubes info CLI
- processing example
- Docker image for distributed processing