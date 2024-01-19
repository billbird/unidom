/*  unidom_util.hpp

    unidom: A modular domination solver
    Copyright (C) 2016 - 2024 Bill Bird

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef UNIDOM_ARRAYUTIL_H
#define UNIDOM_ARRAYUTIL_H

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <typeinfo>
#include <memory>
#include <array>
#include <chrono>

namespace unidom{
    
    class Timer{
    public:
        Timer(){
            started = false;
            stopped = false;
        }
        void start(){
            start_time = std::chrono::high_resolution_clock::now();
            started = true;
            stopped = false;
        }
        void stop(){
            end_time = std::chrono::high_resolution_clock::now();
            stopped = true;
            started = false;
        }
        double elapsed_seconds(){
            if (!stopped && !started)
                throw std::runtime_error("Can't read time of invalid timer");
            return std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count()/1000000.0;
        }
    private:
        std::chrono::high_resolution_clock::time_point start_time, end_time;
        bool started,stopped;

    };
    
    
};

#endif