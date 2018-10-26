library(gdalcubes)

x = gcbs_open_image_collection("/home/marius/github/gdalcubes/cmake-build-debug/src/test.db")
v <- gcbs_view(nx = 500, ny=500, t0 = "2017-01-01", t1="2018-01-01", dt="P1M", l=22, r=24,t=-18,b=-20, aggregation = "min")
xcube <- gcbs_cube(x,v)
xcube

x_red_cube <- gcbs_reduce(xcube,"median")
x_red_cube

gcbs_eval(x_red_cube, "test.tif", "GTiff")

f <- function() {
  x = read_stream_as_array()
  out <- reduce_time_multiband(x, function(x) {
    ndvi <- (x[8,]-x[4,])/(x[8,]+x[4,])
    if (all(is.na(x))) return(NA)
    xx = max(ndvi,na.rm=TRUE) - min(ndvi, na.rm=T)
    return(xx)
  })
  write_stream_from_array(out)
}

scube <- gcbs_stream(xcube, f,c(16,256,256))
