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


#' @export
gcbs_get_projection <- function(obj) {
  stopifnot(is.gcbs_cube(obj))
  return(obj$proj)
}

#' @export
gcbs_get_cubesize <- function(obj, unit="MiB") {
  stopifnot(is.gcbs_cube(obj))
  size_bytes = prod(obj$size) * 8 # assuming everything is double
  return(switch(unit,
         B = size_bytes,
         KB = size_bytes / 1000,
         KiB = size_bytes / 1024,
         MB = size_bytes / (1000^2),
         MiB = size_bytes / (1024^2),
         MiB = size_bytes / (1024^2),
         GB = size_bytes / (1000^3),
         GiB = size_bytes / (1024^3),
         TB = size_bytes / (1000^4),
         TiB = size_bytes / (1024^4),
         PB = size_bytes / (1000^5),
         PiB = size_bytes / (1024^5)
  ))
}


#' @export
gcbs_nbands <- function(obj) {
  stopifnot(is.gcbs_cube(obj))
  return(obj$size[1])
}

#' @export
gcbs_nt <- function(obj) {
  stopifnot(is.gcbs_cube(obj))
  return(obj$size[2])
}

#' @export
gcbs_ny <- function(obj) {
  stopifnot(is.gcbs_cube(obj))
  return(obj$size[3])
}

#' @export
gcbs_nx <- function(obj) {
  stopifnot(is.gcbs_cube(obj))
  return(obj$size[4])
}


#' @export
gcbs_graph<- function(obj) {
  stopifnot(is.gcbs_cube(obj))
  return(obj$graph)
}
