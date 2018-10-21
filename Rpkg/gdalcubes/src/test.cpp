#include <Rcpp.h>
#include <gdalcubes/gdalcubes.h>
using namespace Rcpp;



// [[Rcpp::export]]
void gdalcubes_version() {
  Rcout << "gdalcubes " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH;
}

