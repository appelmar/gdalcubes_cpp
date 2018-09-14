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

#ifndef TIMER_H
#define TIMER_H

#include <chrono>

class timer {
   public:
    timer() { t = std::chrono::high_resolution_clock::now(); }

    void start() {
        t = std::chrono::high_resolution_clock::now();
    }

    double time() {
        std::chrono::high_resolution_clock::time_point tnow = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(
            tnow - t);
        return time_span.count();
    }

   private:
    std::chrono::high_resolution_clock::time_point t;
};

#endif  //GDAL_CUBES_TIMER_H
