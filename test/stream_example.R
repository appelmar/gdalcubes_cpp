library(gdalcubes)
x = read_stream_as_array()

#out = x[5,1,,,drop=FALSE]

#out <- apply(apply(x,c(2,3,4),FUN=function(x){return(sqrt(sum(x^2)/length(x)))}), c(2,3),median, na.rm = FALSE) ## median brightnes over time
#dim(out) <- c(1,1,dim(x)[3], dim(x)[4])


out = (x[8,,,,drop=FALSE] - x[4,,,,drop=FALSE]) / (x[8,,,,drop=FALSE] + x[4,,,,drop=FALSE])





write_stream_from_array(out)


