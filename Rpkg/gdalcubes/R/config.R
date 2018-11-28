

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


#' @export
gcbs_enable_debug_output <- function() {
  libgdalcubes_debug_output(TRUE)
  invisible()
}

#' @export
gcbs_disable_debug_output <- function() {
  libgdalcubes_debug_output(FALSE)
  invisible()
}
