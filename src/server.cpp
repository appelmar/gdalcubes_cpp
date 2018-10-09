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

#include "server.h"
#include <cpprest/filestream.h>
#include "build_info.h"
#include "image_collection.h"
#include "utils.h"
#include "cube_factory.h"
#include <pplx/threadpool.h>
#include <cpprest/rawptrstream.h>
/**
GET  /version
POST /file (name query, body file)
POST /cube (json process descr), return cube_id
GET  /read (cube_id, chunk_id)
**/

void gdalcubes_server::handle_get(web::http::http_request req) {
    std::vector<std::string> path = web::uri::split_path(web::uri::decode(req.relative_uri().path()));
    std::map<std::string, std::string> query_pars = web::uri::split_query(web::uri::decode(req.relative_uri().query()));
    std::for_each(path.begin(), path.end(), [](std::string s) { std::cout << s << std::endl; });
    std::for_each(query_pars.begin(), query_pars.end(), [](std::pair<std::string, std::string> s) { std::cout << s.first << ":" << s.second << std::endl; });
    if (!path.empty()) {
        if (path[0] == "version") {
            std::string version = "gdalcubes_server " + std::to_string(VERSION_MAJOR) + "." + std::to_string(VERSION_MINOR) + "." + std::to_string(VERSION_PATCH);
            req.reply(web::http::status_codes::OK, version.c_str(), "text/plain");
        }
        else if (path[0] == "read") {
            if (query_pars.find("cube_id") == query_pars.end() || query_pars.find("chunk_id") == query_pars.end()) {
                req.reply(web::http::status_codes::BadRequest, "GET /read expects cube_id and chunk_id query parameters.", "text/plain");
            }
            else {
                if (_cubestore.find(std::stoi(query_pars["cube_id"])) == _cubestore.end()) {
                    req.reply(web::http::status_codes::BadRequest, "GET /read cannot find cube with given id.", "text/plain");
                }
                else {
                    std::shared_ptr<chunk_data> dat = _cubestore[std::stoi(query_pars["cube_id"])]->read_chunk(std::stoi(query_pars["chunk_id"]));
                    std::vector<unsigned char> rawdata;
                    rawdata.resize(4*sizeof(uint32_t) + dat->total_size_bytes());
                    std::copy(dat->size().begin(), dat->size().end(), rawdata.begin());
                    std::copy((unsigned char*)(dat->buf()), (unsigned char*)(dat->buf() + dat->total_size_bytes()), rawdata.begin() + (sizeof(uint32_t)*4));

                    web::http::http_response res;
                    res.set_body(rawdata);
                    res.set_status_code(web::http::status_codes::OK);

                    req.reply(res);
                }
            }

        }
        else {
            req.reply(web::http::status_codes::NotFound);
        }
    } else {
        req.reply(web::http::status_codes::NotFound);
    }

    // std::cout << req.to_string();
    //req.reply(web::http::status_codes::OK,is,"text/plain");
}

void gdalcubes_server::handle_post(web::http::http_request req) {
    std::vector<std::string> path = web::uri::split_path(web::uri::decode(req.relative_uri().path()));
    std::map<std::string, std::string> query_pars = web::uri::split_query(web::uri::decode(req.relative_uri().query()));
    std::for_each(path.begin(), path.end(), [](std::string s) { std::cout << s << std::endl; });
    std::for_each(query_pars.begin(), query_pars.end(), [](std::pair<std::string, std::string> s) { std::cout << s.first << ":" << s.second << std::endl; });

    if (!path.empty()) {
        if (path[0] == "file") {
            std::string fname;
            if (query_pars.find("name") != query_pars.end()) {
                fname = query_pars["name"];
            } else {
                fname = utils::generate_unique_filename();
            }
            fname = (_workdir / fname).string();

            if (boost::filesystem::exists(fname)) {
                req.reply(web::http::status_codes::Conflict);
            } else {
                auto fstream = std::make_shared<concurrency::streams::ostream>();
                pplx::task<void> t = concurrency::streams::fstream::open_ostream(fname).then(
                    [fstream, &req](concurrency::streams::ostream outFile) {
                        *fstream = outFile;
                        concurrency::streams::istream indata = req.body();
                        uint32_t maxbytes = 1024 * 1024 * 8;  // Read up to 8MiB
                        while (!indata.is_eof()) {
                            indata.read(fstream->streambuf(), maxbytes).wait();
                        }
                        fstream->close().wait();
                    });
                t.wait();
                req.reply(web::http::status_codes::OK, fname.c_str(), "text/plain");
            }
        }
        else if (path[0] == "cube") {
            // we do not use cpprest JSON library here
            uint32_t id;
            req.extract_string(true).then([&id, this](std::string s) {
                std::shared_ptr<cube> c = cube_factory::create_from_json(nlohmann::json::parse(s));
                id = get_unique_id();
                _mutex_cubestore.lock();
                _cubestore.insert(std::make_pair(id, c));
                _mutex_cubestore.unlock();
            }).wait();
            req.reply(web::http::status_codes::OK, std::to_string(id), "text/plain");
        }
        else {
            req.reply(web::http::status_codes::NotFound);
        }
    } else {
        req.reply(web::http::status_codes::NotFound);
    }
}

void gdalcubes_server::handle_put(web::http::http_request req) {
    req.reply(web::http::status_codes::MethodNotAllowed);
}




std::unique_ptr<gdalcubes_server> server;
int main(int argc, char* argv[]) {


    GDALAllRegister();
    GDALSetCacheMax(1024 * 1024 * 256);            // 256 MiB
    CPLSetConfigOption("GDAL_PAM_ENABLED", "NO");  // avoid aux files for PNG tiles

    srand(time(NULL));


    std::unique_ptr<gdalcubes_server> srv = std::unique_ptr<gdalcubes_server>(new gdalcubes_server("localhost"));
    srv->open().wait();

    std::cout << "Press ENTER to exit" << std::endl;
    std::string line;
    std::getline(std::cin, line);

    srv->close().wait();

    return 0;
}