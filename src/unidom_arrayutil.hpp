/*  unidom_arrayutil.hpp

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
#include "graph.hpp"
#include "vertex_set.hpp"

namespace unidom{

    template<typename T>
    class bounded_range_iterable{
    public:
        bounded_range_iterable(T* start, T* end): start_ptr(start), end_ptr(end){}
        T* begin(){
            return start_ptr;
        }
        T* end(){
            return end_ptr;
        }
    private:
        T* start_ptr;
        T* end_ptr;
    };
    
    template<typename T, std::size_t size>
    bounded_range_iterable<T> array_range(std::array<T,size>& A, int start_idx, int end_idx){
        return bounded_range_iterable<T>(&A[start_idx],&A[end_idx]);
    }
    template<typename T, std::size_t size>
    bounded_range_iterable<T> array_range(std::array<T,size>& A, int end_idx){
        return bounded_range_iterable<T>(&A[0],&A[end_idx]);
    }
    template<typename T, std::size_t size>
    bounded_range_iterable<const T> array_range(const std::array<T,size>& A, int start_idx, int end_idx){
        return bounded_range_iterable<const T>(&A[start_idx],&A[end_idx]);
    }
    template<typename T, std::size_t size>
    bounded_range_iterable<const T> array_range(const std::array<T,size>& A, int end_idx){
        return bounded_range_iterable<const T>(&A[0],&A[end_idx]);
    }
    
    
    
    template<typename T>
    bounded_range_iterable<T> array_range(T A[], int start_idx, int end_idx){
        return bounded_range_iterable<T>(&A[start_idx],&A[end_idx]);
    }
    template<typename T>
    bounded_range_iterable<T> array_range(T A[], int end_idx){
        return bounded_range_iterable<T>(&A[0],&A[end_idx]);
    }
    
    
    
    template<typename T>
    class proxy_iterable{
    public:
        proxy_iterable(T begin_it, T end_it): begin_iterator(begin_it), end_iterator(end_it){}
        T begin(){
            return begin_iterator;
        }
        T end(){
            return end_iterator;
        }
    private:
        T begin_iterator;
        T end_iterator;
    };
    
    template<typename T>
    proxy_iterable<typename T::reverse_iterator> iterate_reverse(T& collection){
        return proxy_iterable<typename T::reverse_iterator>( collection.rbegin(), collection.rend() );
    }
    
    
};

#endif