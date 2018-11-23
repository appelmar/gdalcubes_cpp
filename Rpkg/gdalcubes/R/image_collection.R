#' @example gcbs_image_collection("/home/marius/github/gdalcubes/cmake-build-debug/src/test.db")
#' @export
gcbs_image_collection <- function(filename) {
  stopifnot(file.exists(filename))
  xptr <- libgdalcubes_open_image_collection(filename)
  class(xptr) <- c("gcbs_image_collection", "xptr")
  return(xptr)
}


#' @export
is.gcbs_image_collection <- function(obj) {
  if(!("gcbs_image_collection" %in% class(obj))) {
    return(FALSE)
  }
  if (libgdalcubes_is_null(obj)) {
    warning("Image collection proxy object is invalid")
    return(FALSE)
  }
  return(TRUE)
}


#' @export
print.gcbs_image_collection <- function(obj) {
  stopifnot(is.gcbs_image_collection(obj))
  info <- libgdalcubes_image_collection_info(obj)
  cat(paste("A GDAL image collection object, referencing",nrow(info$images), "images with", nrow(info$bands), " bands\n"))
  cat("Images:\n")
  print(info$images)
  cat("\n")
  cat("Bands:\n")
  print(info$bands)
  cat("\n")
}




# files <- list.files("/home/marius/eodata/Sentinel2/", pattern="*.jp2", recursive = TRUE, full.names = TRUE) 
#
#' @export
gcbs_create_image_collection <-function(files, format_file, out_file=tempfile(fileext = ".sqlite"))
{
  libgdalcubes_create_image_collection(files, format_file, out_file)
  return(gcbs_image_collection(out_file))
}
