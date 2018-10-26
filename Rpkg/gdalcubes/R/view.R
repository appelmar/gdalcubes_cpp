
#' @example gcbs_view(l=-180,r=180,b = -50, t=50, t0="2018-01-01", t1="2018-12-31", dt="P1D")
#' @export
gcbs_view <- function(proj="EPSG:4326", image_collection, nx, ny, dx, dy, l, r, t, b, dt, t0, t1, aggregation=c("none"),resampling=c("near"))  {

  xx = list(space =
              list(left = NULL,
                   right = NULL,
                   top = NULL,
                   bottom = NULL,
                   nx = NULL,
                   ny = NULL,
                   proj=NULL),
            time = list(
              t0 = NULL,
              t1 = NULL,
              dt = NULL
            ),
            aggregation = aggregation,
            resampling = resampling
            )

  xx$space$proj = proj

  # 1. step simply write input args to output list
  if (!missing(nx)) xx$space$nx = as.integer(nx)
  if (!missing(ny)) xx$space$ny = as.integer(ny)
  if (!missing(l)) xx$space$left = as.double(l)
  if (!missing(r)) xx$space$right = as.double(r)
  if (!missing(b)) xx$space$bottom = as.double(b)
  if (!missing(t)) xx$space$top = as.double(t)
  if (!missing(t0)) xx$time$t0 = as.character(t0)
  if (!missing(t1)) xx$time$t1 = as.character(t1)
  if (!missing(dt)) xx$time$dt = as.character(dt)



  # 2. step derive values based on others

  if (!missing(dx) && !missing(l) && !missing(r) && missing(nx)) {
    xx$space$nx = as.integer(ceiling((r-l)/dx))
    expandx <- (xx$space$nx * dx) - (r-l)
    xx$space$left = as.double(l) - expandx / 2
    xx$space$right = as.double(r) + expandx / 2
  }
  else if (!missing(nx) && !missing(dx) && !missing(l) && missing(r)) {
    xx$space$nx = as.integer(nx)
    xx$space$left = as.double(l)
    xx$space$right = as.double(l + nx*dx)
  }
  else if (!missing(nx) && !missing(dx) && !missing(r) && missing(l)) {
    xx$space$nx = as.integer(nx)
    xx$space$left = as.double(r - nx*dx)
    xx$space$right = as.double(r)
  }

  # y
  if (!missing(dy) && !missing(b) && !missing(t) && missing(ny)) {
    xx$space$ny = as.integer(ceiling((t-b)/dy))
    expandy <- (xx$space$ny * dy) - (t-b)
    xx$space$bottom = as.double(b) - expandy / 2
    xx$space$top = as.double(t) + expandy / 2
  }
  else if (!missing(ny) && !missing(dy) && !missing(b) && missing(t)) {
    xx$space$ny = as.integer(ny)
    xx$space$bottom = as.double(b)
    xx$space$top = as.double(b + ny*dy)
  }
  else if (!missing(ny) && !missing(dy) && !missing(t)  && missing(b)) {
    xx$space$ny = as.integer(ny)
    xx$space$bottom = as.double(t - ny*dy)
    xx$space$top = as.double(t)
  }

  # t
  # TODO: suport nt


  # step 3 try to derive missing values automatically
  if (is.null(xx$space$nx) && is.null(xx$space$ny)) {
    xx$space$nx = dev.size("px")[2]
    xx$space$ny = dev.size("px")[2]
  }



  if (!missing(image_collection)) {
    if (is.gcbs_image_collection(image_collection)) {
      # TODO: derive spatial and temporal extent from image collection
    }
  }


  stopifnot(!is.null(xx$time$t0))
  stopifnot(!is.null(xx$time$t1))
  stopifnot(!is.null(xx$time$dt))
  stopifnot(!is.null(xx$space$nx))
  stopifnot(!is.null(xx$space$ny))
  stopifnot(!is.null(xx$space$left))
  stopifnot(!is.null(xx$space$right))
  stopifnot(!is.null(xx$space$bottom))
  stopifnot(!is.null(xx$space$top))


  class(xx) <- c("gcbs_view", class(xx))
  return(xx)
}

#' @export
as_json <- function (x, ...) {
  UseMethod("as_json", x)
}

#' @export
as_json.gcbs_view <- function(obj) {
  return(jsonlite::toJSON(obj, auto_unbox =  TRUE))
}


#' @export
is.gcbs_view <- function(obj) {
  return("gcbs_view" %in% class(obj))
}

