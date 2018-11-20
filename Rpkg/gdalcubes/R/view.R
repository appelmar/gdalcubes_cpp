


#' 
#' Create a spatiotemporal data cube view, i.e. the spatiotemporal extent, size and projection
#' 
#' Views are used to convert image collections to data cubes. The data cube will filter images based on the view's
#' extent, read image data at the defined resolution, and warp to the output projection automatically.
#' 
#' 
#' @note All arguments are optional
#' 
#' @param view if provided, update the view object instead of creating a new data cube view
#' 
#' 
#' @example gcbs_view(l=-180,r=180,b = -50, t=50, t0="2018-01-01", t1="2018-12-31", dt="P1D")
#' @export
gcbs_view <- function(cube, view, proj, nx, ny, dx, dy, l, r, t, b, dt, nt, t0, t1, aggregation,resampling)  {
  if (!missing(cube)) {
    if (length(as.list(match.call())) > 2) {
      warning("Provided arguments except cube will be ignored")
    }
    stopifnot(is.gcbs_cube(cube))
    x = libgdalcubes_get_cube_view(cube)
    class(x) <- c("gcbs_view", class(x))
    return(x)
  }
  xx = NULL
  if (missing(view)) {
    xx = list(space =
                list(left = NULL,
                     right = NULL,
                     top = NULL,
                     bottom = NULL,
                     nx = NULL,
                     dx = NULL,
                     ny = NULL,
                     dy = NULL,
                     proj=NULL),
              time = list(
                t0 = NULL,
                t1 = NULL,
                dt = NULL,
                nt = NULL
              ),
              aggregation = NULL,
              resampling = NULL
              )
  }
  else {
    stopifnot(is.gcbs_view(view))
    xx = view
  }

  if (!(missing(l) && missing(r) && missing(t) && missing(b)) && missing(proj)) {
    warning("Spatial extent without coordinate system given, assuming WGS84")
    proj = "EPSG:4326"
  }
  
  if (!missing(proj)) xx$space$proj = proj
  if (!missing(nx)) xx$space$nx = as.integer(nx)
  if (!missing(ny)) xx$space$ny = as.integer(ny)
  if (!missing(dx)) xx$space$dx = as.double(dx)
  if (!missing(dy)) xx$space$dy = as.double(dy)
  if (!missing(l)) xx$space$left = as.double(l)
  if (!missing(r)) xx$space$right = as.double(r)
  if (!missing(b)) xx$space$bottom = as.double(b)
  if (!missing(t)) xx$space$top = as.double(t)
  if (!missing(t0)) {
    if (is.character(t0)) {
      xx$time$t0 = as.character(t0)
    }
    else if ("POSIXt" %in% class(t0)) {
      xx$time$t0 = format(t0, "%Y-%m-%dT%H:%M:%S")
    }
    else if ("Date" %in% class(t0)) {
      xx$time$t0 = format(t0, "%Y-%m-%d")
    }
    else {
      warning("Invalid type for t0, expected ISO 8601 character, POSIXt, or Date object, value will be ignored")
    }
  }
  if (!missing(t1)) {
    if (is.character(t1)) {
      xx$time$t1 = as.character(t1)
    }
    else if ("POSIXt" %in% class(t1)) {
      xx$time$t1 = format(t1, "%Y-%m-%dT%H:%M:%S")
    }
    else if ("Date" %in% class(t1)) {
      xx$time$t1 = format(t1, "%Y-%m-%d")
    }
    else {
      warning("Invalid type for t1, expected ISO 8601 character, POSIXt, or Date object, value will be ignored")
    }
  }
  if (!missing(dt)) {
    dt = toupper(as.character(dt))
    if (substr(dt[1],1,1) != "P") {
      dt[1] = paste("P", dt[1], sep="")
    }
    xx$time$dt = dt[1]
  }
  if (!missing(nt)) xx$time$nt = as.integer(nt)
  
  if (!missing(aggregation)) xx$aggregation = aggregation
  if (!missing(resampling)) xx$resampling = resampling
  
  class(xx) <- c("gcbs_view", class(xx))
  return(xx)
}

##' @export
#as_json <- function (x, ...) {
#  UseMethod("as_json", x)
#}

# #' @export
# as_json.gcbs_view <- function(obj) {
#   return(jsonlite::toJSON(obj, auto_unbox =  TRUE))
# }
# 
# 

 #' @export
 is.gcbs_view <- function(obj) {
   return("gcbs_view" %in% class(obj))
 }

