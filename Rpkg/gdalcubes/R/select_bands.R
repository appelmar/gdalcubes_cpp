#' @export
gcbs_select_bands <- function(cube, bands) {
  stopifnot(is.gcbs_cube(cube))
  
  x = libgdalcubes_create_select_bands_cube(cube, bands)
  class(x) <- c("gcbs_select_bands_cube", "gcbs_cube", "xptr")
  return(x)
}



#' @export
is.gcbs_select_bands_cube  <- function(obj) {
  if(!("gcbs_select_bands_cube" %in% class(obj))) {
    return(FALSE)
  }
  if (libgdalcubes_is_null(obj)) {
    warning("GDAL data cube proxy object is invalid")
    return(FALSE)
  }
  return(TRUE)
}




