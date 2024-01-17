/*  graph.hpp

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
#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <string>
#include <array>
#include <algorithm>

#include "unidom_constants.hpp"

typedef int VertIndex;

class GraphError{
public:
    GraphError(std::string message){
        this->message = message;
    }
    std::string what(){
        return message;
    }
private:
    std::string message;
};

class Graph{
public:
    static const int INVALID_VERTEX = (VertIndex)(0x7fffffff);
    typedef std::vector<VertIndex> neighbour_list;
    class Vertex{
    friend class Graph;
    public:
        int deg() const{
            return neighbours().size();
        }
        neighbour_list& neighbours(){
            return _neighbours;
        }
        const neighbour_list& neighbours() const{
            return _neighbours;
        }
        VertIndex neighbours(int idx){
            return _neighbours[idx];
        }
        int get_real_index() const{
            return real_index;
        }
        int get_index() const{
            return index;
        }
        bool add_neighbour_simple(int neighbour){
            if (std::find(std::begin(_neighbours), std::end(_neighbours), neighbour) != std::end(_neighbours)){
                return false;
            }else{
                _neighbours.push_back(neighbour);
                return true;
            }
        }
    private:
        neighbour_list _neighbours;
        int real_index;
        int index;
    };
    
    Graph(int n){
        reset(n);
    }
    Graph(){
    }
    
    void reset(int new_size){
        if (new_size >= unidom::MAX_VERTS)
            throw GraphError("Graph with too many vertices ("+std::to_string(new_size)+")");
        vertices.clear();		
        vertices.resize(new_size);
        for(unsigned int i = 0; i < new_size; i++)
            vertices[i].real_index = vertices[i].index = i;
    }
    int n(){
        return vertices.size();
    }
    int nVerts(){
        return n();
    }
    Vertex& operator[](int v){
        return vertices[(unsigned int)v];
    }
    std::vector<Vertex>& V(){
        return vertices;
    }
    void renumber( std::vector<VertIndex> permutation, Graph& result ){
        decltype(permutation) inverse_perm(n());
        for(unsigned int i = 0; i < n(); i++)
            inverse_perm[permutation.at(i)] = i;
        result.reset(n());
        for(unsigned int i = 0; i < n(); i++){
            Vertex& v_out = result[i];
            Vertex& v_in = (*this)[permutation[i]];
            v_out.real_index = v_in.real_index;
            for( VertIndex neighbour: v_in.neighbours() )
                v_out.neighbours().push_back( inverse_perm[neighbour] );
        }
    }
    
    void add_edge_simple(VertIndex i, VertIndex j){
        Vertex& u = vertices[i];
        Vertex& v = vertices[j];
        u.add_neighbour_simple(j);
        v.add_neighbour_simple(i);
    }
private:
    std::vector< Vertex > vertices;
};


#endif