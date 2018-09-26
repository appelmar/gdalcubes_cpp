/*
   Copyright 2018 Marius Appel <marius.appel@uni-muenster.de>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "cube.h"

void cube::write_gtiff_directory(std::string dir) {

    namespace fs = boost::filesystem;
    fs::path op(dir);

    if (!fs::exists(dir)) {
        fs::create_directories(op);
    }

    if (!fs::is_directory(dir)) {
        throw std::string("ERROR in chunking::write_gtiff_directory(): output is not a directory.");
    }

    uint32_t nchunks = count_chunks();
    for (uint32_t i=0; i<nchunks; ++i) {

        GDALDriver *gtiff_driver = (GDALDriver *)GDALGetDriverByName("GTiff");
        if (gtiff_driver == NULL) {
            throw std::string("ERROR: cannot find GDAL driver for GTiff.");
        }

        CPLStringList out_co(NULL);
        //out_co.AddNameValue("TILED", "YES");
        //out_co.AddNameValue("BLOCKXSIZE", "256");
        // out_co.AddNameValue("BLOCKYSIZE", "256");

        bounds_st cextent = this->bounds_from_chunk(i);  // implemented in derived classes
        double affine[6];
        affine[0] = cextent.s.left;
        affine[3] = cextent.s.top;
        affine[1] = _st_ref->dx();
        affine[5] = -_st_ref->dy();
        affine[2] = 0.0;
        affine[4] = 0.0;

        std::shared_ptr<chunk_data> dat = read_chunk(i);

        for (uint16_t ib = 0; ib < dat->count_bands(); ++ib) {
            for (uint32_t it = 0; it < dat->size()[1]; ++it) {
                fs::path out_file = op / (std::to_string(i) + "_" + std::to_string(ib) + "_" + std::to_string(it) + ".tif");

                GDALDataset *gdal_out = gtiff_driver->Create(out_file.string().c_str(), dat->size()[3], dat->size()[2], 1, GDT_Float64, out_co.List());
                gdal_out->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, dat->size()[3], dat->size()[2], ((double*)dat->buf())  + (ib * dat->size()[1] * dat->size()[2] * dat->size()[3] + it * dat->size()[2] * dat->size()[3]), dat->size()[3], dat->size()[2], GDT_Float64, 0, 0, NULL);
                gdal_out->GetRasterBand(1)->SetNoDataValue(_bands.get(ib).no_data_value);
                char *wkt_out;
                OGRSpatialReference srs_out;
                srs_out.SetFromUserInput(_st_ref->proj().c_str());
                srs_out.exportToWkt(&wkt_out);

                GDALSetProjection(gdal_out, wkt_out);
                GDALSetGeoTransform(gdal_out, affine);

                GDALClose(gdal_out);
            }
        }
    }

}