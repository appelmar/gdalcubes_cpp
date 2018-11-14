#' @export
gcbs_create_collection_format <- function() {
  
  format <- list(
    pattern = ".*",
    images = list(
      pattern = "(.*)"
    ),
    datetime = list(
      pattern = NULL,
      format = NULL
    ),
    bands = list(
      band01 = list(
        pattern=".*"
      )
    )
  )
  class(format)<- c("gcbs_collection_format","list")
  return(format)
}


#' @export
is.gcbs_collection_format <-function(obj) {
  return("gcbs_collection_format" %in% class(obj))
}