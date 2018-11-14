#' @example gcbs_open_image_collection("/home/marius/github/gdalcubes/cmake-build-debug/src/test.db")
#' @export
gcbs_open_image_collection <- function(filename) {
  stopifnot(file.exists(filename))

  db <- DBI::dbConnect(RSQLite::SQLite(), dbname = filename)

  out <- list(
    name = basename(filename),
    path = filename,
    bands = dbReadTable(db, "bands"),
    images = dbReadTable(db, "images"),
    gdalrefs = dbReadTable(db, "gdalrefs")
  )

  DBI::dbDisconnect(db)


  # xptr <- gdalcubes:::libgdalcubes_open_image_collection(filename)
  # out <- list(
  #   name = basename(filename),
  #   path = filename,
  #   xptr = xptr
  # )

  class(out) <- c("gcbs_image_collection", "list")
  return(out)
}


#' @export
is.gcbs_image_collection <- function(obj) {
  return("gcbs_image_collection" %in% class(obj))
}


#' @export
summary.gcbs_image_collection <- function(obj) {
  print("xxxx")
}

# 
# get_image_id <- function(path) {
#   p <- unlist(strsplit(path, "/"))
#   p[length(p)-4]
#   return(gsub(".SAFE", "",p[length(p)-4]))
# }
# 
# get_datetime <- function(path) {
#   return(substr(get_image_id(path), 12, 26))
# }
# 
# get_band <- function(path) {
#   return(substr(get_image_id(path), 12, 26))
# }

#' @export
gcbs_create_image_collection <-function(files, get_band, get_datetime, get_image_id, ignore_pattern=NULL)
{
  files <- list.files("/home/marius/eodata/Sentinel2/", pattern="*.jp2", recursive = TRUE, full.names = TRUE)
  sapply(files, function(f) {
    
  })
  
}



