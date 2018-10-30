

#' @export
gcbs_set_threads <- function(n=1) {
  stopifnot(n >= 1)
  stopifnot(n%%1==0)
  libgdalcubes_set_threads(n)
  invisible()
}

#' @export
gcbs_version <- function() {
  return(libgdalcubes_version())
}
