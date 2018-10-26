library(gdalcubes)

x = gcbs_open_image_collection("/home/marius/github/gdalcubes/cmake-build-debug/src/test.db")
v <- gcbs_view(nx = 500, ny=500, t0 = "2017-01-01", t1="2018-01-01", dt="P1M", l=22, r=24,t=-18,b=-20, aggregation = "min")
xcube <- gcbs_cube(x,v)
xcube

x_red_cube <- gcbs_reduce(xcube,"median")
x_red_cube

gcbs_eval(x_red_cube, "test.tif", "GTiff")


