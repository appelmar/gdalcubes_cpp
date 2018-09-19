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

#include "chunking.h"




chunk_data default_chunking::read(chunkid id) {
    chunk_data out;
    if (id < 0 || id >= count_chunks())
        return out; // chunk is outside of the view, we don't need to read anything.




    // Access image collection and fetch band information


    // Derive how many pixels the chunk has (this varies for chunks at the boundary of the view)


    // Fill buffers accordingly


    // Find intersecting images from collection and iterate over these

    // For each image, call gdal_warp if projection is different than view or GDALTranslate if possible


    // For each band, call RasterIO to read the data
    // Copy data to the right position in the buffers






}