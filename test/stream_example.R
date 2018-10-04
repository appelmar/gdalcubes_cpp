library(gdalcubes)
x = read_stream_as_array()
sample_chunk = x
save(sample_chunk, file=paste(ceiling(runif(1,1,99999)), ".Rdata" , sep=""))

out <- reduce_time_multiband(x, function(x) {
    ndvi <- (x[8,]-x[4,])/(x[8,]+x[4,])
    if (all(is.na(x))) return(NA)
    xx = max(ndvi,na.rm=TRUE) - min(ndvi, na.rm=T)
    return(xx)
})

#out = (x[8,,,,drop=FALSE] - x[4,,,,drop=FALSE]) / (x[8,,,,drop=FALSE] + x[4,,,,drop=FALSE])





write_stream_from_array(out)


