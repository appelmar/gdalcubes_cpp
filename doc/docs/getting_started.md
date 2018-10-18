# Getting started


## Installation




## Sample data





## Creating an image collection





## Deriving a cloud-free image



## Time-series analysis with R




## Distributed processing












## Example

```
gdalcubes create_collection -f collection_format_s2_zip.json jp2_list.txt s2.db
> IMAGE COLLECTION 's2.db' has 144 images with 12 bands from 1724 GDAL dataset references
```




```
gdalcubes info s2.db
> IMAGE COLLECTION 's2.db' has 144 images with 12 bands from 1724 GDAL dataset references
> date range: 2018-06-02T10:40:19 to 2018-09-30T10:40:19
> x / lat range: 5.87022 to 9.14329
> y / lon range: 51.2789 to 52.3504
> BAND:(type,offset,scale,unit)
> B01:(uint16,0,1,)
> B02:(uint16,0,1,)
> B03:(uint16,0,1,)
> B04:(uint16,0,1,)
> B05:(uint16,0,1,)
> B06:(uint16,0,1,)
> B07:(uint16,0,1,)
> B08:(uint16,0,1,)
> B09:(uint16,0,1,)
> B11:(uint16,0,1,)
> B12:(uint16,0,1,)
> B8A:(uint16,0,1,)
```



```
view.json
{
  "aggregation" : "median",
  "resampling" : "near",
  "space" :
  {
    "left" : 5.87022,
    "right" : 9.14329,
    "top" : 52.3504,
    "bottom" : 51.2789,
    "proj" : "EPSG:4326",
    "nx" : 500,
    "ny" : 500
  },
  "time" :
  {
    "t0" : "2017-06-02",
    "t1" : "2018-09-30",
    "dt" : "P7D"
  }
}

```



```
gdalcubes reduce -r median -v view.json s2.db mosaic_1.tif
```

