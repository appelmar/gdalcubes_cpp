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

#include "gdal_dataset_pool.h"


    gdal_dataset::gdal_dataset_pool gdal_dataset::_pool;

    gdal_dataset::gdal_dataset_registry* gdal_dataset::gdal_dataset_registry::_instance = NULL;
    uint32_t gdal_dataset::_count_open = 0;
    uint32_t gdal_dataset::_count_close = 0;


