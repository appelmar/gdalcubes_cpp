library(gdalcubes)

gcbs_collection_formats()

x = gcbs_image_collection("/home/marius/Desktop/file7329263c65.sqlite")
x
x = gcbs_create_image_collection(list.files("/home/marius/eodata/Sentinel2/",recursive = TRUE,pattern=".jp2$",full.names = TRUE), format = "Sentinel2_L1C_local")



x = gcbs_image_collection("/home/marius/github/gdalcubes/cmake-build-debug/src/test.db")
x
#v <- gcbs_view(nx = 500, ny=500, t0 = "2017-01-01", t1="2018-01-01", dt="P1M", l=22, r=24,t=-18,b=-20, proj="EPSG:4326", aggregation = "min")
v <- gcbs_view(nx = 500, ny=500, t0 = "2017-01-01", t1="2018-01-01", dt="P1M", l=23, r=24,t=-19,b=-20, proj="EPSG:4326", aggregation = "min")


xcube <- gcbs_cube(x, v, chunking = c(16,128,128))
xcube

plot(gcbs_apply_pixel(gcbs_select_bands(xcube, c("B04","B08")), c("(B04 - B08)/10000", "(B04 + B08)/10000")), t= c(1,4,8), key.pos=1)



plot(xcube, bands = 4)
plot(gcbs_select_bands(xcube, c("B04","B08")), t=c(4,8))
cat(gcbs_graph(gcbs_select_bands(xcube, c("B04","B08"))))

plot(xcube, rgb=4:2, t=c(4,8), zlim=c(100,2000))
plot(xcube, rgb=4:2, t=8)
plot(xcube, rgb=4:2)

plot(xcube, rgb=4:2, t=8)
plot(xcube, key.pos=1, t=c(1,4,8))
plot(xcube, key.pos=1, t=c(1,4,8))

plot(xcube)

x_red_cube <- gcbs_reduce(xcube,"median")
plot(x_red_cube, key.pos = 1, bands=1)

#gcbs_eval(x_red_cube, "/home/marius/Desktop/test2.tif", "GTiff")

require(magrittr)
plot(gcbs_cube(x, v)  %>% gcbs_reduce("median") %>% gcbs_select_bands(c("B04_median","B08_median")) )
plot(gcbs_cube(x, v) %>% gcbs_select_bands(c("B04","B8A"))  %>% gcbs_reduce("median")  )

plot(gcbs_cube(x, v) %>% gcbs_select_bands(c("B04","B08", "B8A"))  %>% gcbs_apply_pixel(c("((B08-B04)/(B08+B04))", "B08-B8A")) %>%  gcbs_reduce("median"), key.pos=1)
plot(gcbs_cube(x, v) %>% gcbs_select_bands(c("B04","B08", "B8A"))  %>% gcbs_apply_pixel(c("B08 < 2000")) %>%  gcbs_reduce("mean"), key.pos=1)

# median NDVI
plot(gcbs_cube(x, v) %>% gcbs_select_bands(c("B04","B08"))  %>% gcbs_apply_pixel(c("((B08-B04)/(B08+B04))")) %>%  gcbs_reduce("median"), key.pos=1)

plot(gcbs_cube(x, v) %>% gcbs_select_bands(c("B02","B03","B04"))  %>%  gcbs_reduce("min"), rgb=3:1)


plot(gcbs_cube(x, v) %>% gcbs_select_bands(c("B04","B08"))  %>% gcbs_apply_pixel(c("((B08-B04)/(B08+B04))")), key.pos=1, t=c(4,8))


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

cr <- gcbs_stream(xcube, f,c(16,256,256))
plot(cr)

xstrm_red <- gcbs_reduce(gcbs_stream(xcube, f,c(16,256,256)),reducer = "min")
gcbs_view(xstrm_red) <- gcbs_view(l=23, r=24)
cat(gcbs_graph(xstrm_red))
gcbs_eval(xstrm_red, "/home/marius/Desktop/xxx.tif", "GTiff")










#####
library(gdalcubes)
setwd("/home/marius/Desktop/CHIRPS/")


x = gcbs_create_image_collection(list.files(pattern=".tif",recursive = T), format="collection_format.json")

x = gcbs_image_collection("/home/marius/Desktop/CHIRPS/CHIRPS.db")
x
v <- gcbs_view(nx = 360*2, ny=2*100, t0 = "1981-01-01", t1="1981-05-31", dt="P1D", l=-180, r=180,t=50,b=-50)
xcube <- gcbs_cube(x, v)
xcube


# count numer of days wiht precipitation larger than 40
plot(gcbs_cube(x, v)  %>% gcbs_apply_pixel(c("precipitation > 50", "precipitation < 0.001")) %>%  gcbs_reduce("sum"), key.pos=1, col=heat.colors)


x_red_cube <- gcbs_reduce(xcube,"median")
x_red_cube
 
plot(x_red_cube, rgb=4:2)


gcbs_eval(x_red_cube, "/home/marius/Desktop/test_chirps.nc")
gcbs_set_threads(8)

# minimum 30 day precipitation sum
f <- function() {
  require(zoo)
  x = read_stream_as_array()
  out <- reduce_time_multiband(x, function(x) {
    a = min(rollsum(x[1,], 30), na.rm = TRUE)
    if (!is.finite(a)) a <- NA
    a
  })
  write_stream_from_array(out)
}

xstrm <- gcbs_stream(xcube, f,c(gcbs_nt(xcube),128,128))
plot(xstrm)





#####
library(gdalcubes)
gcbs_set_threads(8)
setwd("/home/marius/Desktop/MODIS/MOD13A3.A2018/")
x = gcbs_image_collection("MOD13A3.db")
x

v <- gcbs_view(proj="EPSG:4326", nx = 500, ny=500, t0 = "2018-01-01", t1="2018-09-30", dt="P3M", l=-20, r=20,t=60,b=40, aggregation = "first")
xcube <- gcbs_cube(x, v)
xcube

plot(xcube, col=heat.colors, key.pos = 1, t = 1)


plot(gcbs_reduce(xcube, reducer="median"), key.pos=1)

f <- function() {
  x = read_stream_as_array()
  out <- reduce_time_multiband(x, function(x) {
    y = x[1,]
    #max(abs(y[2:(length(y))] - y[1:(length(y)-1)]), na.rm=TRUE)
    mean(y, na.rm=T)
  })
  write_stream_from_array(out)
}

xstrm <- gcbs_stream(xcube, f,c(gcbs_nt(xcube),128,128))
plot(xstrm,key.pos=1, breaks=seq(0, 3000, length.out=11), col=heat.colors)
cat(gcbs_graph(xstrm))
xstrm_red <- gcbs_reduce(xstrm,reducer = "min")
cat(gcbs_graph(xstrm_red))

# TODO: return stars object if no file is provided
gcbs_eval(xstrm_red, "/home/marius/Desktop/change_ndvi_mod.tif", "GTiff")












