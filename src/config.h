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

#ifndef CONFIG_H
#define CONFIG_H

#include <curl/curl.h>
#include <gdal_priv.h>
#include <memory>
#include "build_info.h"
#include "error.h"
#include "progress.h"

// forward declarations
class chunk_processor;
class chunk_processor_singlethread;

/**
 * A simple structure to store
 * version information of the library
 */
struct version_info {
    uint16_t VERSION_MAJOR;
    uint16_t VERSION_MINOR;
    uint16_t VERSION_PATCH;
    std::string BUILD_DATE;
    std::string BUILD_TIME;
    std::string GIT_DESC;
    std::string GIT_COMMIT;
};

/**
 * @brief A singleton class to manage global configuration options
 */
class config {
   public:
    /**
     * Return the singleton instance
     */
    static config* instance() {
        static GC g;
        if (!_instance) {
            _instance = new config();
        }
        return _instance;
    }

    inline std::shared_ptr<chunk_processor> get_default_chunk_processor() {
        return _chunk_processor;
    }
    inline void set_default_chunk_processor(std::shared_ptr<chunk_processor> p) {
        _chunk_processor = p;
    }

    inline void set_default_progress_bar(std::shared_ptr<progress> p) {
        _progress_bar = p;
    }

    inline std::shared_ptr<progress> get_default_progress_bar() {
        return _progress_bar;
    }

    inline void set_gdal_cache_max(uint32_t size_bytes) {
        GDALSetCacheMax(size_bytes);
        _gdal_cache_max = size_bytes;
    }

    inline void set_server_chunkcache_max(uint32_t size_bytes) {
        _server_chunkcache_max = size_bytes;
    }

    inline uint32_t get_server_chunkcache_max() {
        return _server_chunkcache_max;
    }

    inline void set_server_worker_threads_max(uint16_t max_threads) {
        _server_worker_threads_max = max_threads;
    }

    inline uint16_t get_server_worker_threads_max() {
        return _server_worker_threads_max;
    }

    inline bool get_swarm_curl_verbose() { return _swarm_curl_verbose; }
    inline void set_swarm_curl_verbose(bool verbose) { _swarm_curl_verbose = verbose; }

    inline bool get_gdal_debug() { return _gdal_debug; }
    inline void set_gdal_debug(bool debug) {
        _gdal_debug = debug;
        if (debug)
            CPLSetConfigOption("CPL_DEBUG", "ON");
        else
            CPLSetConfigOption("CPL_DEBUG", "OFF");
    }

    inline void set_gdal_num_threads(uint16_t threads) {
        _gdal_num_threads = threads;
        CPLSetConfigOption("GDAL_NUM_THREADS", std::to_string(_gdal_num_threads).c_str());
    }

    inline void set_verbose(bool v) { _verbose = v; }
    inline bool get_verbose(void) { return _verbose; }

    inline uint16_t get_gdal_num_threads() { return _gdal_num_threads; }

    /**
     * @brief Global gdalcubes library initialization function
     */
    void gdalcubes_init() {
        curl_global_init(CURL_GLOBAL_ALL);
        GDALAllRegister();
        GDALSetCacheMax(_gdal_cache_max);
        CPLSetConfigOption("GDAL_PAM_ENABLED", "NO");  // avoid aux files for PNG tiles
        curl_global_init(CURL_GLOBAL_ALL);
        CPLSetConfigOption("GDAL_NUM_THREADS", std::to_string(_gdal_num_threads).c_str());
        srand(time(NULL));
        CPLSetErrorHandler(CPLQuietErrorHandler);
    }

    /**
   * @brief Global gdalcubes library cleanup function
   */
    void gdalcubes_cleanup() {
        curl_global_cleanup();
    }

    /**
   * @brief Global gdalcubes version information function
   */
    version_info get_version_info();

    inline void set_error_handler(error_action f) {
        _error_handler = f;
    }

    inline error_action get_error_handler() {
        return _error_handler;
    }

   private:
    std::shared_ptr<chunk_processor> _chunk_processor;
    std::shared_ptr<progress> _progress_bar;
    error_action _error_handler;
    uint32_t _gdal_cache_max;
    uint32_t _server_chunkcache_max;
    uint16_t _server_worker_threads_max;  // number of threads for parallel chunk reads
    bool _swarm_curl_verbose;
    uint16_t _gdal_num_threads;
    bool _verbose;
    bool _gdal_debug;

   private:
    config();
    ~config() {}
    config(const config&) = delete;
    static config* _instance;

    class GC {
       public:
        ~GC() {
            if (config::_instance) {
                delete config::_instance;
                config::_instance = nullptr;
            }
        }
    };
};

#endif  //CONFIG_H
