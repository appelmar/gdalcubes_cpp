#' @export
gcbs_reduce <- function(cube, reducer=c("mean","median","min","max")) {
  stopifnot(is.gcbs_cube(cube))

  x = libgdalcubes_create_reduce_cube(cube, reducer)
  class(x) <- c("gcbs_reduce_cube", "gcbs_cube") # hide Xptr class
  return(x)
}





#' @export
is.gcbs_reduce_cube  <- function(obj) {
  return("gcbs_reduce_cube" %in% class(obj))
}

#' @export
gcbs_eval.gcbs_reduce_cube <- function(x, outfile="reduce.tif", of="GTiff") {
  stopifnot(is.gcbs_reduce_cube(x))
  libgdalcubes_eval_reduce_cube(x, outfile, of)
}


