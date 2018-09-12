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

#ifndef GDAL_DATASET_POOL_H_
#define GDAL_DATASET_POOL_H_

#include <gdal_priv.h>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <queue>
#include <sstream>


    class gdal_dataset;






    // A wrapper for GDAL datasets, which counts open files and opens / closes files
    class gdal_dataset {

        //friend class gdal_dataset_registry;

    public:


        class gdal_dataset_pool {
        public:
            gdal_dataset_pool() : _capacity(256) {
                CPLSetConfigOption("GDAL_MAX_DATASET_POOL_SIZE", std::to_string(_capacity).c_str());
            }
            gdal_dataset_pool(uint32_t capacity) : _capacity(capacity) {
                CPLSetConfigOption("GDAL_MAX_DATASET_POOL_SIZE", std::to_string(_capacity).c_str());
            }

            void push(gdal_dataset* obj) {
                _mutex.lock();
                while (_current_size >= capacity()) {
                    pop();
                }
                std::string key = obj->path();

                _datasets[key] = obj;
                _current_size++;


                uint32_t p = inc_prio();


                _prio_forward[key] = p;
                _prio_backward[p] = key;
                _mutex.unlock();
            }


            // closes GDAL dataset with lowest priority
            void pop() {
                _mutex.lock();
                // select dataset with the lowest priority and close
                auto it_x = _prio_backward.lower_bound(0);  // lowest value greater than or equal to 0
                uint32_t p = it_x->first;
                _datasets[it_x->second]->close(); // automatically calls remove(...)
                _mutex.unlock();
            }

            void remove(std::string key) {
               _mutex.lock();
                auto it = _datasets.find(key);
                if (it != _datasets.end()) {
                    _current_size--;
                    _datasets.erase(it);
                }
                auto it_2 = _prio_forward.find(key);
                uint32_t p = it_2->second;
                if (it_2 != _prio_forward.end()) {
                    _prio_forward.erase(it_2);
                }
                auto it_3 = _prio_backward.find(p);
                if (it_3 != _prio_backward.end()) {
                    _prio_backward.erase(it_3);
                }
                _mutex.unlock();

            }

            std::string print_status() {
                std::stringstream ss;
                ss << "-----------------------------------------------------------"
                   << std::endl;
                ss << "GDAL pool status: (" << _current_size << "/" << _capacity << ") open";
                ss << "-----------------------------------------------------------"
                   << std::endl;
                return ss.str();
            }



            inline uint32_t count() {return _current_size;}
            inline uint32_t capacity() {return _capacity;}


        private:

            const uint32_t& inc_prio() {

                _mutex.lock();
                // if pool has been accessed too often, such that count variables
                // might be overflowed, rebuild priority index structure
                if (cur_priority == std::numeric_limits<uint32_t>::max()) {
                    uint32_t newp = 0;

                    for (auto it = _prio_forward.begin(); it != _prio_forward.end(); ++it) {
                        it->second = newp++;
                        _prio_backward.insert(std::make_pair(it->second, it->first));
                    }
                    cur_priority = newp;

                } else {

                    ++cur_priority;

                }
                _mutex.unlock();
                return cur_priority;
            }

            uint32_t _capacity;

            std::map<std::string, gdal_dataset*> _datasets;
            std::map<std::string, uint32_t> _prio_forward;
            std::map<uint32_t, std::string> _prio_backward;

            uint32_t cur_priority;

            uint16_t _current_size;


            std::recursive_mutex _mutex;
        };










        // This class makes sure that there is only one gdal_dataset for the same path
        // TODO: upper size limit?
        class gdal_dataset_registry {
        public:
            static gdal_dataset_registry* instance() {
                if (_instance == NULL) {
                    _instance = new gdal_dataset_registry();
                }
                return _instance;
            }

            gdal_dataset* get(std::string path) {
                if (_datasets.find(path) !=  _datasets.end()) {
                    return _datasets[path];
                }
                gdal_dataset* g = new gdal_dataset(path);
                _datasets[path] = g; // TODO: read write?
                return g;
            }

            std::string to_string() {
                std::stringstream out;
                out << "REGISTERED GDAL DATASETS:" << std::endl;
                auto it = _datasets.begin();
                uint32_t i=1;
                while (it != _datasets.end()) {
                    out <<  "\t[" << i << "] " << it->first << std::endl;
                    ++it;
                    ++i;
                }
                return out.str();
            }


        private:
            static gdal_dataset_registry* _instance;
            gdal_dataset_registry () : _datasets() { }
            gdal_dataset_registry ( const gdal_dataset_registry& );
            ~gdal_dataset_registry () {
                auto it = _datasets.begin();
                while (it != _datasets.end()) {
                    delete it->second;
                    ++it;
                }
            }
        private:
            std::map<std::string, gdal_dataset*> _datasets;
        };





    public:
        static gdal_dataset* get(std::string path) {

            return gdal_dataset_registry::instance()->get(path);
        }

    private:


        gdal_dataset(std::string path) : _path(path), _dataset(NULL), _write(false) {}
        gdal_dataset(std::string path, bool write) : _path(path), _dataset(NULL), _write(write) {}

        // Disable copying
        gdal_dataset(const gdal_dataset& x);
        gdal_dataset(const gdal_dataset&& x);


//        gdal_dataset(const gdalcubes::gdal_dataset& x):  _path(""), _dataset(NULL), _write(false)
//        {
//            _path = x._path;
//            _write = x._write;
//            _dataset = NULL; // TODO: make sure that the same file is opened only once internally
//        }
//
//        gdal_dataset(const gdalcubes::gdal_dataset&& x):  _path(""), _dataset(NULL), _write(false)
//        {
//            _path = x._path;
//            _write = x._write;
//            _dataset = x._dataset;
//        }


       // static gdal_dataset open_read(std::string path) {return gdal_dataset(path);}
       // static gdal_dataset open_write(std::string path) {return gdal_dataset(path, true);}



        ~gdal_dataset() {
            if (is_open()) {
                close(); // also make sure that the pool will remove the dataset
            }
        }





    public:

        inline bool is_open() const { return _dataset != NULL; }

        /**
         * Returns a GDALDataset pointer. PLEASE DO NOT CALL GDALClose() on this pointer.
         * @return
         */
        inline GDALDataset* get() {
            if (!is_open()) {
                open();
            }
            return _dataset;
        }

        // syntactic sugar: -> returns GDALDataset pointer
        inline GDALDataset* operator ->() {return get();}
        inline GDALDataset*& operator *() {
            get();
            return _dataset;
        }



        inline std::string path() const { return _path; }





    private:
        std::string _path;
        GDALDataset* _dataset;
        bool _write;
        std::recursive_mutex _mutex;


    private:

        void open() {
            // check whether there is capacity in the pool
            // if not, close another gdal_dataset (e.g. oldest)
            // call GDALOpen and store pointer in _dataset
            // add this to list to dataset pool

            if (is_open()) return;


            while (_pool.count() >= _pool.capacity()) {
                _pool.pop();
            }
            _mutex.lock();
            _dataset = write? (GDALDataset *)GDALOpen(_path.c_str(), GA_Update) : (GDALDataset *)GDALOpen(_path.c_str(), GA_ReadOnly);
            ++_count_open;
            _mutex.unlock();
            if (_dataset == NULL) {
                throw std::string("GDALOpen failed for '" + _path  + "'");
            }
            _pool.push(this);

        }
        void close() {
            if (is_open()) {
                _mutex.lock();
                GDALClose(_dataset);
                ++_count_close;
                _dataset = NULL;
                _mutex.unlock();
                _pool.remove(_path); // remove from pool
            }
        }

        static gdal_dataset_pool _pool;
        static uint32_t _count_open;
        static uint32_t _count_close;


    public:
        static uint32_t count_open() {return _count_open;}
        static uint32_t count_close() {return _count_close;}
    };











#endif  //GDAL_DATASET_POOL_H_
