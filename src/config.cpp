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

#include "config.h"
#include "cube.h"

namespace gdalcubes {

config* config::_instance = nullptr;

config::config() : _chunk_processor(std::make_shared<chunk_processor_singlethread>()),
                   _progress_bar(std::make_shared<progress_none>()),
                   _error_handler(error_handler::default_error_handler),
                   _gdal_cache_max(1024 * 1024 * 256),         // 256 MiB
                   _server_chunkcache_max(1024 * 1024 * 512),  // 512 MiB
                   _server_worker_threads_max(1),
                   _swarm_curl_verbose(false),
                   _gdal_num_threads(1),
                   _streaming_dir(filesystem::get_tempdir()),
                   _collection_format_preset_dirs() {}

version_info config::get_version_info() {
    version_info v;
    v.VERSION_MAJOR = GDALCUBES_VERSION_MAJOR;
    v.VERSION_MINOR = GDALCUBES_VERSION_MINOR;
    v.VERSION_PATCH = GDALCUBES_VERSION_PATCH;
    v.BUILD_DATE = __DATE__;
    v.BUILD_TIME = __TIME__;
    v.GIT_COMMIT = GDALCUBES_GIT_COMMIT;
    v.GIT_DESC = GDALCUBES_GIT_DESC;
    return v;
}

}  // namespace gdalcubes
