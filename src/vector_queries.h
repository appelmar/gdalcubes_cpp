/*
    MIT License

    Copyright (c) 2019 Marius Appel <marius.appel@uni-muenster.de>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#ifndef VECTOR_H
#define VECTOR_H

#include "cube.h"

namespace gdalcubes {

class vector_queries {
   public:
    /**
         * Query values of a data cube at irregular spatiotmeporal points.
         *
         * @brief This function extracts values of data cube cells at provided spatiotemporal points.
         * The result will contain data from all bands of the data cube, but will not contain the input point coordinates.
         *
         * @param cube data cube
         * @param x x coordinates of query points
         * @param y y coordinates of query points
         * @param t date/time of query points
         * @param srs spatial reference system of spatial point coordinates
         * @return data frame (vector of double vectors) where first vector represents columns (bands)
         */
    static std::vector<std::vector<double>> query_points(std::shared_ptr<cube> cube, std::vector<double> x, std::vector<double> y, std::vector<std::string> t, std::string srs);

    /**
     * Query summary statistics of a data cube over spatial polygons
     *
     * As a result, the function creates a geopackage file with layers for all time steps. Each layer contains the geometries and selected combinations of aggregation functions and bands
     * as attributes. Available aggregation functions currently include "min", "max", "mean", "median", "sum", "prod", and "count". "var" and "sd" are currently NOT implemented.
     *
     * @note THIS FUNCTION IS NOT YET IMPLEMENTED
     *
     *
     * @param cube input data cube
     * @param ogr_dataset input OGR dataset identifier with polygon geometries
     * @param agg_band_functions vector of aggregation functions, band pairs representing combinations of available summary statistics functions (first element) and data cube bands (second element), e.g. {"min", "band1"}.
     * @param out_dir output directory
     * @param out_prefix prefix for output filenames
     * @param ogr_layer defines from which layer geometries are taken, if the ogr_dataset has multiple layers
     */
    static void zonal_statistics(std::shared_ptr<cube> cube, std::string ogr_dataset, std::vector<std::pair<std::string, std::string>> agg_band_functions, std::string out_dir, std::string out_prefix = "", std::string ogr_layer = "");
};

}  // namespace gdalcubes

#endif  //VECTOR_H
