
#' @export
serialize_function <- function(f) {
  src <- attr(f,"srcref", exact = TRUE)
  if (is.null(src))
    stop("source for given function is not available")
  return(paste(as.character(src),collapse = "\n"))
}


#' @export
gcbs_stream <- function(cube, f, chunk_size) {
  stopifnot(is.gcbs_image_collection_cube(cube))
  srcfile =  tempfile(".stream_", tmpdir = getwd(),fileext = ".R")
  cat(serialize_function(f), file = srcfile, append = FALSE)
  cmd <- paste("Rscript ", "--vanilla ", "-e ", "\"require(gdalcubes)\" ", "-e ", "\"do.call(eval(parse('", srcfile ,"')), args=list())\"", sep="")
  x = libgdalcubes_create_stream_cube(cube, cmd, chunk_size)
  class(x) <- c("gcbs_stream_cube", "gcbs_cube", "list")
  return(x)
}
