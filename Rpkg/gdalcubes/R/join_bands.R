#' @export
gcbs_join_bands <- function(A, B) {
  stopifnot(is.gcbs_cube(A))
  stopifnot(is.gcbs_cube(B))
  
  x = libgdalcubes_create_join_bands_cube(cube, reducer)
  class(x) <- c("gcbs_join_bands_cube", "gcbs_cube", "xptr")
  return(x)
}


#' @export
is.gcbs_join_bands_cube  <- function(obj) {
  if(!("gcbs_join_bands_cube" %in% class(obj))) {
    return(FALSE)
  }
  if (libgdalcubes_is_null(obj)) {
    warning("GDAL data cube proxy object is invalid")
    return(FALSE)
  }
  return(TRUE)
}




