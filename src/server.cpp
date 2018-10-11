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
#include <cpprest/rawptrstream.h>
#include <condition_variable>
#include "build_info.h"
#include "cube_factory.h"
#include "image_collection.h"
#include "utils.h"
/**
GET  /version
POST /file (name query, body file)
POST /cube (json process descr), return cube_id
GET /cube/{cube_id}
POST /cube/{cube_id}/{chunk_id}/start
GET /cube/{cube_id}/{chunk_id}/status status= "notrequested", "submitted" "queued" "running" "canceled" "finished" "error"
GET /cube/{cube_id}/{chunk_id}/download

**/

server_chunk_cache* server_chunk_cache::_instance = nullptr;
std::mutex server_chunk_cache::_singleton_mutex;

void gdalcubes_server::handle_get(web::http::http_request req) {
    std::vector<std::string> path = web::uri::split_path(web::uri::decode(req.relative_uri().path()));
    std::map<std::string, std::string> query_pars = web::uri::split_query(web::uri::decode(req.relative_uri().query()));
    //    std::for_each(path.begin(), path.end(), [](std::string s) { std::cout << s << std::endl; });
    //    std::for_each(query_pars.begin(), query_pars.end(), [](std::pair<std::string, std::string> s) { std::cout << s.first << ":" << s.second << std::endl; });
    if (!path.empty()) {
        if (path[0] == "version") {
            std::string version = "gdalcubes_server " + std::to_string(VERSION_MAJOR) + "." + std::to_string(VERSION_MINOR) + "." + std::to_string(VERSION_PATCH);
            req.reply(web::http::status_codes::OK, version.c_str(), "text/plain");
        } else if (path[0] == "cube") {
            if (path.size() == 2) {
                uint32_t cube_id = std::stoi(path[1]);
                if (_cubestore.find(cube_id) == _cubestore.end()) {
                    req.reply(web::http::status_codes::NotFound, "ERROR in /GET /cube/{cube_id}: cube with given id is not available.", "text/plain");
                } else {
                    req.reply(web::http::status_codes::OK, _cubestore[cube_id]->make_constructible_json().dump(2).c_str(),
                              "application/json");
                }
            } else if (path.size() == 4) {
                uint32_t cube_id = std::stoi(path[1]);
                uint32_t chunk_id = std::stoi(path[2]);
                std::string cmd = path[3];
                if (cmd == "download") {
                    if (_cubestore.find(cube_id) == _cubestore.end()) {
                        req.reply(web::http::status_codes::NotFound, "ERROR in /GET /cube/{cube_id}/{chunk_id}/download: cube is not available", "text/plain");
                    } else if (chunk_id >= _cubestore[cube_id]->count_chunks()) {
                        req.reply(web::http::status_codes::NotFound, "ERROR in /GET /cube/{cube_id}/{chunk_id}/download: invalid chunk_id given", "text/plain");
                    }
                    // if not in queue, executing, or finished, return 404
                    else if (!server_chunk_cache::instance()->has(std::make_pair(cube_id, chunk_id)) &&
                             _chunk_read_requests_set.find(std::make_pair(cube_id, chunk_id)) == _chunk_read_requests_set.end() &&
                             _chunk_read_executing.find(std::make_pair(cube_id, chunk_id)) == _chunk_read_executing.end()) {
                        req.reply(web::http::status_codes::NotFound, "ERROR in /GET /cube/{cube_id}/{chunk_id}/download: chunk read has not been requested yet", "text/plain");
                    } else {
                        while (!server_chunk_cache::instance()->has(std::make_pair(cube_id, chunk_id))) {
                            // req.reply(web::http::status_codes::NotFound, "ERROR in /GET /cube/{cube_id}/{chunk_id}/download: chunk is not available.", "text/plain");

                            std::unique_lock<std::mutex> lck(_chunk_cond[std::make_pair(cube_id, chunk_id)].second);
                            _chunk_cond[std::make_pair(cube_id, chunk_id)].first.wait(lck);
                        }
                        std::shared_ptr<chunk_data> dat = server_chunk_cache::instance()->get(std::make_pair(cube_id, chunk_id));
                        std::vector<unsigned char> rawdata;
                        rawdata.resize(4 * sizeof(uint32_t) + dat->total_size_bytes());
                        std::copy((unsigned char*)(dat->size().data()), (unsigned char*)(dat->size().data()) + (sizeof(uint32_t) * 4), (unsigned char*)(rawdata.data()));
                        std::copy((unsigned char*)(dat->buf()), (unsigned char*)(dat->buf() + dat->total_size_bytes()), rawdata.begin() + (sizeof(uint32_t) * 4));

                        if (query_pars.find("clean") != query_pars.end() && query_pars["clean"] == "true") {
                            server_chunk_cache::instance()->remove(std::make_pair(cube_id, chunk_id));
                        }
                        web::http::http_response res;
                        res.set_body(rawdata);
                        res.set_status_code(web::http::status_codes::OK);
                        req.reply(res);
                    }

                } else if (cmd == "status") {
                    if (_cubestore.find(cube_id) == _cubestore.end()) {
                        req.reply(web::http::status_codes::NotFound, "ERROR in /GET /cube/{cube_id}/{chunk_id}/status: cube is not available", "text/plain");
                    } else if (chunk_id >= _cubestore[cube_id]->count_chunks()) {
                        req.reply(web::http::status_codes::NotFound, "ERROR in /GET /cube/{cube_id}/{chunk_id}/status: invalid chunk_id given", "text/plain");
                    }

                    // TODO: /GET /cube/{cube_id}/{chunk_id}/status
                    if (server_chunk_cache::instance()->has(std::make_pair(cube_id, chunk_id))) {
                        req.reply(web::http::status_codes::OK, "finished", "text/plain");
                    } else if (_chunk_read_executing.find(std::make_pair(cube_id, chunk_id)) !=
                               _chunk_read_executing.end()) {
                        req.reply(web::http::status_codes::OK, "running", "text/plain");
                    } else if (_chunk_read_requests_set.find(std::make_pair(cube_id, chunk_id)) != _chunk_read_requests_set.end()) {
                        req.reply(web::http::status_codes::OK, "queued", "text/plain");
                    } else {
                        req.reply(web::http::status_codes::OK, "notrequested", "text/plain");
                    }

                } else {
                    req.reply(web::http::status_codes::NotFound);
                }
            } else {
                req.reply(web::http::status_codes::NotFound);
            }
        }

        else {
            req.reply(web::http::status_codes::NotFound);
        }
    } else {
        req.reply(web::http::status_codes::NotFound);
    }
}

void gdalcubes_server::handle_post(web::http::http_request req) {
    std::vector<std::string> path = web::uri::split_path(web::uri::decode(req.relative_uri().path()));
    std::map<std::string, std::string> query_pars = web::uri::split_query(web::uri::decode(req.relative_uri().query()));
    //    std::for_each(path.begin(), path.end(), [](std::string s) { std::cout << s << std::endl; });
    //    std::for_each(query_pars.begin(), query_pars.end(), [](std::pair<std::string, std::string> s) { std::cout << s.first << ":" << s.second << std::endl; });

    if (!path.empty()) {
        if (path[0] == "file") {
            // TODO: check if file with same path and size exists, if yes do not upload again
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
                boost::filesystem::create_directories(boost::filesystem::path(fname).parent_path());
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
        } else if (path[0] == "cube") {
            if (path.size() == 1) {
                // we do not use cpprest JSON library here
                uint32_t id;
                req.extract_string(true).then([&id, this](std::string s) {
                                            std::shared_ptr<cube> c = cube_factory::create_from_json(nlohmann::json::parse(s));
                                            id = get_unique_id();
                                            _mutex_cubestore.lock();
                                            _cubestore.insert(std::make_pair(id, c));
                                            _mutex_cubestore.unlock();
                                        })
                    .wait();
                req.reply(web::http::status_codes::OK, std::to_string(id), "text/plain");
            } else if (path.size() == 4) {
                uint32_t cube_id = std::stoi(path[1]);
                uint32_t chunk_id = std::stoi(path[2]);

                std::string cmd = path[3];
                if (cmd == "start") {
                    if (_cubestore.find(cube_id) == _cubestore.end()) {
                        req.reply(web::http::status_codes::NotFound, "ERROR in /POST /cube/{cube_id}/{chunk_id}/start: cube is not available", "text/plain");
                    } else if (chunk_id >= _cubestore[cube_id]->count_chunks()) {
                        req.reply(web::http::status_codes::NotFound, "ERROR in /POST /cube/{cube_id}/{chunk_id}/start: invalid chunk_id given", "text/plain");
                    }

                    // if already in queue, executing, or finished, do not compute again
                    else if (server_chunk_cache::instance()->has(std::make_pair(cube_id, chunk_id)) ||
                             _chunk_read_requests_set.find(std::make_pair(cube_id, chunk_id)) != _chunk_read_requests_set.end() ||
                             _chunk_read_executing.find(std::make_pair(cube_id, chunk_id)) != _chunk_read_executing.end()) {
                        req.reply(web::http::status_codes::OK);
                    } else {
                        _mutex_worker_threads.lock();
                        if (_worker_threads.size() < _worker_thread_max) {
                            // immediately start a thread

                            _worker_threads.push_back(std::thread([this, cube_id, chunk_id]() {
                                _mutex_chunk_read_requests.lock();
                                _chunk_read_requests.push_back(std::make_pair(cube_id, chunk_id));
                                _chunk_read_requests_set.insert(std::make_pair(cube_id, chunk_id));
                                _mutex_chunk_read_requests.unlock();
                                while (1) {
                                    _mutex_chunk_read_requests.lock();
                                    while (!_chunk_read_requests.empty()) {
                                        _mutex_chunk_read_executing.lock();
                                        uint32_t xcube_id = _chunk_read_requests.front().first;
                                        uint32_t xchunk_id = _chunk_read_requests.front().second;
                                        _chunk_read_executing.insert(_chunk_read_requests.front());
                                        _chunk_read_requests_set.erase(_chunk_read_requests.front());
                                        _chunk_read_requests.pop_front();
                                        _mutex_chunk_read_requests.unlock();
                                        _mutex_chunk_read_executing.unlock();

                                        _mutex_cubestore.lock();
                                        std::shared_ptr<cube> c = _cubestore[xcube_id];
                                        _mutex_cubestore.unlock();

                                        std::shared_ptr<chunk_data> dat = c->read_chunk(xchunk_id);

                                        server_chunk_cache::instance()->add(std::make_pair(xcube_id, xchunk_id), dat);

                                        _mutex_chunk_read_executing.lock();
                                        _chunk_read_executing.erase(std::make_pair(xcube_id, xchunk_id));
                                        _mutex_chunk_read_executing.unlock();

                                        // notify waiting download requests
                                        if (_chunk_cond.find(std::make_pair(xcube_id, xchunk_id)) != _chunk_cond.end()) {
                                            _chunk_cond[std::make_pair(xcube_id, xchunk_id)].first.notify_all();
                                        }

                                        _mutex_chunk_read_requests.lock();
                                    }
                                    _mutex_chunk_read_requests.unlock();

                                    std::unique_lock<std::mutex> lock(_mutex_worker_cond);
                                    _worker_cond.wait(lock);
                                }
                            }));

                            _mutex_worker_threads.unlock();
                            req.reply(web::http::status_codes::OK);
                        }

                        else {
                            _mutex_worker_threads.unlock();
                            std::lock_guard<std::mutex> lock(_mutex_worker_cond);
                            // add to queue
                            _mutex_chunk_read_requests.lock();
                            _chunk_read_requests.push_back(std::make_pair(cube_id, chunk_id));
                            _mutex_chunk_read_requests.unlock();
                            _worker_cond.notify_all();
                            req.reply(web::http::status_codes::OK);
                        }
                    }
                } else if (cmd == "cancel") {
                    //TODO: POST /cube/{cube_id}/{chunk_id}/cancel
                } else {
                    req.reply(web::http::status_codes::NotFound);
                }
            } else {
                req.reply(web::http::status_codes::NotFound);
            }

        } else {
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

    std::unique_ptr<gdalcubes_server> srv = std::unique_ptr<gdalcubes_server>(new gdalcubes_server("0.0.0.0"));
    srv->open().wait();

    std::cout << "Press ENTER to exit" << std::endl;
    std::string line;
    std::getline(std::cin, line);

    srv->close().wait();

    return 0;
}