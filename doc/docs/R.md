# The gdalcubes R package
 
The [gdalcubes R package](https://github.com/appelmar/gdalcubes_R) is hosted at https://github.com/appelmar/gdalcubes_R.  
It is not available from CRAN and at the moment must be installed from sources.
 
 
## Installation

You can install the package directly from GitHub using the [`devtools` package](https://cran.r-project.org/web/packages/devtools/index.html).
However, since the package includes the gdalcubes C++ library as a git submodule in its `src` folder, you must install the [git command line client](https://git-scm.com/downloads) before.

On Windows, you will furthermore need [Rtools](https://cran.r-project.org/bin/windows/Rtools). Package building on Windows automatically downloads needed dependencies ([GDAL](https://github.com/OSGeo/gdal), [NetCDF](https://github.com/Unidata/netcdf-c), [SQLite](https://www.sqlite.org/index.html), [curl](https://github.com/curl/curl)) from [rwinlib](https://github.com/rwinlib). On Linux, please
install these libraries e.g. using your package manager.

The following R code will then install the gdalcubes R package.

```
library(devtools)
install_git("https://github.com/appelmar/gdalcubes_R", args="--recursive")
```

## Getting started

- The package includes a vignette that illustrates the basic concepts and funcitonality on a small (< 1 GB) MODIS dataset (see `vignettes/getting_started.Rmd`).
- Read the [tutorial](S2R.md) how to process Sentinel 2 images with the gdalcubes R package.