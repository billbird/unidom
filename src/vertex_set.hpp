/*  vertex_set.hpp

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
#ifndef VERTEX_SET_H
#define VERTEX_SET_H

#include <vector>
#include <array>
#include <cassert>
#include "unidom_constants.hpp"

class VertexSet{
public:
    typedef VertIndex* iterator;
    typedef const VertIndex* const_iterator;
    
    VertexSet(){
        reset_empty();
    }
    VertexSet(int n){
        reset_full(n);
    }
    
    void reset(){
        reset_empty();
    }
    
    void reset_empty(){
        size = 0;
        for(auto& x: set_indices)
            x = unidom::MAX_VERTS;		
    }
    void reset_full(int n){
        size = n;
        for(unsigned int i = 0; i < n; i++){
            set_elements[i] = i;
            set_indices[i] = i;
        }
    }
    
    bool contains(VertIndex v){
        return set_indices[(unsigned int)v] < size;
    }	
    
    bool add(VertIndex v){
        assert(!contains(v));
        set_indices[v] = size;
        set_elements[size] = v;
        size++;
        return false;
    }
    
    void remove_pop(VertIndex v){
        int idx = set_indices[v];
        assert(set_indices[v] == size-1);
        set_indices[v] = unidom::MAX_VERTS+1;
        --size;
    }
    
    bool remove(VertIndex v){
        assert(contains(v));
        int idx = set_indices[v];
        int u = set_elements[--size];
        set_elements[idx] = u;
        set_indices[u] = idx;
        set_elements[size] = v;
        set_indices[v] = unidom::MAX_VERTS+1;
        return true;
    }
    
    const_iterator begin() const {
        return &set_elements[0];
    }
    const_iterator end() const{
        return &set_elements[size];
    }
    int get_size() const{
        return size;
    }
protected:
    int size;
    std::array<VertIndex,unidom::MAX_VERTS> set_elements;
    std::array<int,unidom::MAX_VERTS> set_indices;
    
};


#endif