#' @export
gcbs_apply_pixel <- function(cube, expr, names=NULL) {
  stopifnot(is.gcbs_cube(cube))
  
  if (is.null(names)) {
    names <- paste("band", 1:length(expr), sep="")
  }
  
  x = libgdalcubes_create_apply_pixel_cube(cube, expr, names)
  class(x) <- c("gcbs_apply_pixel_cube", "gcbs_cube", "xptr")
  return(x)
}



#' @export
is.gcbs_apply_pixel_cube  <- function(obj) {
  if(!("gcbs_apply_pixel_cube" %in% class(obj))) {
    return(FALSE)
  }
  if (libgdalcubes_is_null(obj)) {
    warning("GDAL data cube proxy object is invalid")
    return(FALSE)
  }
  return(TRUE)
}




