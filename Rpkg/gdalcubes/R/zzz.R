.pkgenv <- new.env(parent=emptyenv())

.onLoad <- function(libname,pkgname) {
  op <- options()
  op.gdalcubes <- list(
    gdalcubes.gdal_cache_max = 1024 * 1024 * 256,
    gdalcubes.gdal_num_threads = 1,
    gdalcubes.verbose = FALSE
  )
  toset <- !(names(op.gdalcubes) %in% names(op))
  if(any(toset)) options(op.gdalcubes[toset])

  # call gdalcubes_init()
  if(!Sys.getenv("GDALCUBES_STREAMING") == "1") {
    libgdalcubes_init()
  }
  invisible()
}


.is_streaming <-function() {
  return(Sys.getenv("GDALCUBES_STREAMING") == "1")
}

.onAttach <- function(libname,pkgname)
{
  # if the package is used inside streaming, redirect stdout to stderr
  # in order to not disturb the (binary) communication with gdalcubes
  if(Sys.getenv("GDALCUBES_STREAMING") == "1") {
    sink(stderr())
  }
  else {
    libgdalcubes_cleanup()
  }
}
