# gdalcubes - Earth observation data cubes from GDAL image collections


gdalcubes is a library and toolkit to represent collections of Earth Observation (EO) images
as on-demand data cubes (or _multidimensional arrays_). It is written in C++ and includes interfaces
for R and (in the future) Python. gdalcubes is not a database, i.e., it does not need to store additional copies of the imagery but instead
simply links to and indexes existing files / GDAL datasets. 

Core features:

- Creating image collections that link to and index existing files / cloud storage 
- Convert image collections to data cubes on-the-fly during processing with desired spatiotemporal resolution, extent, 
- Stream chunks of data cubes to external programs (R, python)
- Distributed processing with `gdalcubes_server` and deployment via Docker


Basic workflow:

1. Create an image collection from existing imagery (local files, or cloud storage)
2. Define a _data cube view_ to specify spatiotemporal resolution, extent, and projection of the target data cube
3. Stream chunks of the data cube to your analysis in R / Python
4. If applicable, write results as multidimensional NetCDF or GeoTIFF image  


gdalcubes alone does **not** provide a rich set of algorithms besides reducing data-cubes over time and filtering by space, time, and bands. However, 
streaming data into R or Python makes their power accessible with little amount of reimplemtation.

Typical use cases, where gdalcubes helps:
- EO time series processing on large scale imagery
- Applying R / Python functions on chunks of the data (e.g. time series, spatial slices, moving windows), in distributed computing environments
- 


   