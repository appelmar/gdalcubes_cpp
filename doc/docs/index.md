# Earth observation data cubes from GDAL image collections

**gdalcubes** is a library and toolkit to represent collections of Earth Observation (EO) images
as on-demand data cubes (or _multidimensional arrays_). It presents a single model how multitemporal and multispectral 
imagery can be processed and streamed into external programs such as R or Python on local computers or distributed cloud environments. 
gdalcubes is not a database, i.e., it does not need to store additional copies of the imagery but instead
simply links to and indexes existing files / GDAL datasets. It is written in C++ and includes a command line interface as well as a package for R. A python  package is
planned for the future.

Core features:

- Creating image collections that link to and index existing files / cloud storage 
- Convert image collections to data cubes on-the-fly during processing with desired spatiotemporal resolution, extent, 
- Stream chunks of data cubes to external programs (R, python)
- Distributed processing with `gdalcubes_server` and deployment via Docker


gdalcubes alone does **not** provide a rich set of algorithms besides reducing data-cubes over time and filtering by space, time, and bands. However, 
streaming data into R or Python makes their power accessible with little amount of reimplemtation needed.



   