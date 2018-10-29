#' @export
gcbs_cube <- function(image_collection, view, chunking=c(16, 256, 256)) {

  stopifnot(is.gcbs_image_collection(image_collection))
  stopifnot(is.gcbs_view(view))
  x = libgdalcubes_create_image_collection_cube(image_collection$path, as_json(view))
  class(x) <- c("gcbs_image_collection_cube", "gcbs_cube", "list")
  return(x)
}

#' @export
is.gcbs_image_collection_cube <- function(obj) {
  return("gcbs_image_collection_cube" %in% class(obj))
}

#' @export
is.gcbs_cube <- function(obj) {
  return("gcbs_cube" %in% class(obj))
}

#' @export
print.gcbs_cube <- function(obj) {
  cat("A GDAL data cube proxy object of type", obj$type, "\n")
  cat("Dimensions:\n")
  print(obj$dimensions)
  cat("\n")
  cat("Bands:\n")
  print(obj$bands)
  cat("\n")

}


# size_bytes <- function(obj)
# projection <- function(obj)
