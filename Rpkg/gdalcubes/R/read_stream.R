
# .First<-function(){
#   library('devtools')
#   if(Sys.getenv("GDALCUBES_STREAMING") == "1") {
#     sink(tempfile(),type = "output")
#     sink(tempfile(),type="message")
#   }
# }



.onAttach = function(libname,pkgname)
{
  # if the package is used inside streaming, redirect stdout to stderr
  # in order to not disturb the (binary) communication with gdalcubes
  if(Sys.getenv("GDALCUBES_STREAMING") == "1") {
    sink(stderr())
  }
}

#' Test
#' @export
sample_array <- function() {
  d <- c(2,10,128,128)
  array(rnorm(d),dim=d)
}



#' Test
#' @export
read_stream_as_array <-function(with.dimnames=TRUE) {
  f <-file('stdin', 'rb');
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
# read_stream_as_vector <- function() {
#
#   f <-file('stdin', 'rb');
#   s <- readBin(f, integer(), n=4)
#   buf <- readBin(f, double(), n = prod(s))
#   close(f)
#   return(buf)
# }

# read_stream_as_df <- function() {
#   # TODO
# }


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
  s <- dim(v) # test
  writeBin(as.integer(s), f)
  writeBin(as.double(v), f)
  flush(f)
  close(f)
}

serialize_function <- function(f) {
  src <- attr(f,"srcref", exact = TRUE)
  if (is.null(src))
    stop("source for given function is not available")
  return(paste(as.character(src),collapse = "\n"))
}


