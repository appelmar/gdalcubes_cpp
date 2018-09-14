# gdalcubes - Earth observation data cubes from GDAL image collections

**_This project is in initial development_**

This C++ library and toolkit presents a data model for Earth observation image collections from many 
two-dimensional GDAL datasets. Gdalcubes strongly build on GDAL but differs by
- adding date/time to images,
- formalizing how multiple GDALDatasets  correspond to multi-band images,
- and supporting images with different projections in single image collections.

gdalcubes includes both, a data model for image collections as well as tools to stream data as multidimensional arrays
into external tools such as R.  





## Command line interface

```
gdalcubes create_collection -f collection_format.json list.txt out.db 
gdalcubes create_collection -R -f collection_format.json /home/user/data/ out.db 
``` 


## Creating image collections: The JSON collection format description



### Example: Local Sentinel 2 imagery

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



# Ongoing work (without priority)
- Test cases with some imagery
- Examples for cloud storage
- CLI
- image collection filters on bands, bounding box, and datetime
- views
- processing example
- 