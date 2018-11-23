#' @export
gcbs_cube <- function(image_collection, view, chunking=c(16, 256, 256)) {

  stopifnot(is.gcbs_image_collection(image_collection))
  
  x = NULL
  if (!missing(view)) {
    stopifnot(is.gcbs_view(view))
    x = libgdalcubes_create_image_collection_cube(image_collection, view)
  }
  else {
    x = libgdalcubes_create_image_collection_cube(image_collection)
  }
  class(x) <- c("gcbs_image_collection_cube", "gcbs_cube", "xptr")
  return(x)
}

#' @export
is.gcbs_image_collection_cube <- function(obj) {
  if(!("gcbs_image_collection_cube" %in% class(obj))) {
    return(FALSE)
  }
  if (libgdalcubes_is_null(obj)) {
    warning("GDAL data cube proxy object is invalid")
    return(FALSE)
  }
  return(TRUE)
}

#' @export
is.gcbs_cube <- function(obj) {
  if(!("gcbs_cube" %in% class(obj))) {
    return(FALSE)
  }
  if (libgdalcubes_is_null(obj)) {
    warning("GDAL data cube proxy object is invalid")
    return(FALSE)
  }
  return(TRUE)
}

#' @export
print.gcbs_cube <- function(obj) {
  if (libgdalcubes_is_null(obj)) {
    stop("GDAL data cube proxy object is invalid")
  }
  x = libgdalcubes_cube_info(obj)
  cat("A GDAL data cube proxy object\n")
  cat("Dimensions:\n")
  print(x$dimensions)
  cat("\n")
  cat("Bands:\n")
  print(x$bands)
  cat("\n")
}

#' @export
gcbs_size <- function(obj) {
  if (libgdalcubes_is_null(obj)) {
    stop("GDAL data cube proxy object is invalid")
  }
  x = libgdalcubes_cube_info(obj)
  return(x$size[2:4])
}

#' @export
dim.gcbs_cube <- function(obj) {
  return(gcbs_size(obj))
}

#' @export
names.gcbs_cube <- function(obj) {
  if (libgdalcubes_is_null(obj)) {
    stop("GDAL data cube proxy object is invalid")
  }
  x = libgdalcubes_cube_info(obj)
  return(as.character(x$bands$name))
}


#' @export
gcbs_dimensions <- function(obj) {
  if (libgdalcubes_is_null(obj)) {
    stop("GDAL data cube proxy object is invalid")
  }
  x = libgdalcubes_cube_info(obj)
  return(x$dimensions)
}

#' @export
gcbs_bands <- function(obj) {
  if (libgdalcubes_is_null(obj)) {
    stop("GDAL data cube proxy object is invalid")
  }
  x = libgdalcubes_cube_info(obj)
  return(x$bands)
}

#' @export
gcbs_get_projection <- function(obj) {
  stopifnot(is.gcbs_cube(obj))
  x = libgdalcubes_cube_info(obj)
  return(x$proj)
}

#' @export
gcbs_get_cubesize <- function(obj, unit="MiB") {
  stopifnot(is.gcbs_cube(obj))
  x = libgdalcubes_cube_info(obj)
  size_bytes = prod(x$size) * 8 # assuming everything is double
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
  x = libgdalcubes_cube_info(obj)
  return(x$size[1])
}

#' @export
gcbs_nt <- function(obj) {
  stopifnot(is.gcbs_cube(obj))
  x = libgdalcubes_cube_info(obj)
  return(x$size[2])
}

#' @export
gcbs_ny <- function(obj) {
  stopifnot(is.gcbs_cube(obj))
  x = libgdalcubes_cube_info(obj)
  return(x$size[3])
}

#' @export
gcbs_nx <- function(obj) {
  stopifnot(is.gcbs_cube(obj))
  x = libgdalcubes_cube_info(obj)
  return(x$size[4])
}


#' @export
gcbs_graph <- function(obj) {
  stopifnot(is.gcbs_cube(obj))
  x = libgdalcubes_cube_info(obj)
  return(x$graph)
}

#' @export
"gcbs_view<-" <-function(x,value) {
  stopifnot(is.gcbs_cube(x))
  stopifnot(is.gcbs_view(value))
  if (!is.gcbs_image_collection_cube)
    stop("x is no image_collection_cube, updating the data cube view is currently only implemented for image_collection_cube")
  libgdalcubes_update_cube_view(x,value)
  return(x)
}



#' @export
gcbs_eval <- function(x, fname = tempfile()) {
  stopifnot(is.gcbs_cube(x))
  libgdalcubes_eval_cube(x, fname)
}






