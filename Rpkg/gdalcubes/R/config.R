

#' @export
gcbs_set_threads <- function(n=1) {
  libgdalcubes_set_threads(n)
  invisible()
  # TODO: set option?!
}
