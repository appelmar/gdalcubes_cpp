

#' Test
#' @export
sample_array <- function() {
  d <- c(2,10,128,128)
  array(rnorm(d),dim=d)
}



#' Test
#' @export
read_stream_as_array <-function(with.dimnames=TRUE) {
  f <-file('stdin', 'rb')
  on.exit(close(f))
  s <- readBin(f, integer(), n=4)
  if (prod(s) == 0) {
    warning("gdalcubes::read_stream_as_array(): received empty chunk.")
    return(NULL)
  }
  bandnames <- character(s[1])
  for (i in 1:s[1]) {
    nchars= readBin(f, integer(), n=1)
    bandnames[i] = readChar(f, nchars = nchars)
  }
  dims <- readBin(f, double(), n=sum(s[2:4]))
  proj.length = readBin(f, integer(), n=1)
  proj = readChar(f, nchars = proj.length)
  buf <- readBin(f, double(), n = prod(s))

  # row major -> column major
  x <- array(buf, dim=s)
  dim(x) <- rev(dim(x))
  if (with.dimnames) {
    dnames <- list(band=bandnames,
                   datetime=dims[1:s[2]],
                   y = dims[(s[2]+1):(s[2]+s[3])],
                   x = dims[(s[2]+s[3]+1):(s[2]+s[3]+s[4])])
    dimnames(x) <- rev(dnames)
  }
  return(aperm(x,4:1))
}

read_stream_as_df <- function() {
  require(reshape2)
  return(dcast(melt(read_stream_as_array()),formula = datetime + y + x ~ band))
}



# write_stream_from_vector <- function(v=NULL) {
#   #f <-file('stdout', 'w+b')
#   f <- pipe("cat", "wb")
#   s <- c(13,1,256,256) # test
#   print(s)
#   buf <- rnorm(prod(s))
#   writeBin(as.integer(s), f)
#   writeBin(as.double(buf), f)
#   flush(f)
#   close(f)
# }

#' Test
#' @export
write_stream_from_array <- function(v) {
  v = aperm(v,c(4,3,2,1))
  dim(v) <- rev(dim(v))
  stopifnot(length(dim(v)) == 4)
  f <- pipe("cat", "wb")
  on.exit(close(f))
  s <- dim(v) # test
  writeBin(as.integer(s), f)
  writeBin(as.double(v), f)
  flush(f)
}

serialize_function <- function(f) {
  src <- attr(f,"srcref", exact = TRUE)
  if (is.null(src))
    stop("source for given function is not available")
  return(paste(as.character(src),collapse = "\n"))
}


#' ASSUMPTION: FUN RECEIVES MULTIBAND TIMESERIES AS INPUT AND PRODUCES
#' A SCALAR OR VECTOR AS OUTPUT, WHICH ELEMENTS ARE INTERPRETED AS NEW BANDS
#' OF THE RESULT
#' @examples
#' load("sample_chunk.Rdata")
#' reduce_time_multiband(sample_chunk, function(x) {
#'   ndvi <- (x[8,]-x[4,])/(x[8,]+x[4,])
#'   return(c(min(ndvi, na.rm=T),max(ndvi, na.rm=T)))
#' })
#' @export
reduce_time_multiband <- function(x, FUN, ...) {
  stopifnot(is.array(x))
  stopifnot(length(dim(x))==4)
  res <- apply(x, c(3,4), FUN, ...)
  if (length(dim(res)) == 2) {
    dim(res) <- c(1,1,dim(res)[1],dim(res)[2])

  }
  else if (length(dim(res)) == 3) {
    dim(res) <- c(dim(res)[1],1,dim(res)[2],dim(res)[3]) # if a vector of length n is returned, elements are interpreted as new bands of the output
  }
  return(res)
}

#' FUN receives the time series of one band as input
#' @examples
#' load("sample_chunk.Rdata")
#' y = apply_pixel(sample_chunk, function(x) {
#'  ndvi <- (x[8]-x[4])/(x[8]+x[4])
#'  return(c(ndvi=(x[8]-x[4])/(x[8]+x[4]), nir=x[8]))
#' })
#' @export
apply_pixel <- function(x, FUN, ...) {
  stopifnot(is.array(x))
  stopifnot(length(dim(x))==4)
  res <- apply(x, c(2,3,4), FUN, ...)
  dim(res) <- c(rep(1, 4-length(dim(res))), dim(res))
  return(res)
}


#' FUN receives one spatial slice of all bands (3d array) as input
#' @examples
#' load("sample_chunk.Rdata")
#' y = reduce_space_multiband(sample_chunk, function(x) {
#'  ndvi <- (x[8,,]-x[4,,])/(x[8,,]+x[4,,])
#'  return(c(min(ndvi, na.rm=T),max(ndvi, na.rm=T)))
#' })
#' @export
reduce_space_multiband <- function(x, FUN, ...) {
  stopifnot(is.array(x))
  stopifnot(length(dim(x))==4)
  res <- apply(x, c(2), FUN, ...)
  dim(res) <- c(dim(res)[1],dim(res)[2],1,1) # if a vector of length n is returned, elements are interpreted as new bands of the output
  return(res)
}



