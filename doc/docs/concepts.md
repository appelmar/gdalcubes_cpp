
# Concepts 





# Image collections


## GDAL dataset references

- local files, vs. fileserver, vs. cloud storage


## EO image collections are not data cubes

## Creating image collection files



---


## JSON collection format description

## Sharing image collections



# Data cube views

Data cube views (or simply _views_) convert an image collection to a data cube by defining the 
basic shape of the cube, i.e. how we look at the data. It includes the spatiotemporal resolution, extent, and projection.


## JSON format




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