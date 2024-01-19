/*  graph_util.cpp

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

#include <iostream>
#include <fstream>
#include "graph.hpp"
#include "graph_util.hpp"

using unidom::MAX_DEGREE;
using unidom::MAX_VERTS;


bool read_graph(std::istream& f, Graph& g){
    int n;
    if (!(f>>n) || n < 0 || n >= MAX_VERTS)
        return false;
    
    g.reset(n);
    
    for(int i = 0; i < n; i++){
        int deg;
        if (!(f>>deg) || deg < 0 || deg >= MAX_DEGREE)
            return false;
        for(int j = 0; j < deg; j++){
            VertIndex u;
            if (!(f>>u) || u < 0 || u >= MAX_DEGREE)
                return false;
            g[i].neighbours().push_back(u);
        }
    }
    
    return true;
}

void write_graph(std::ostream& f, Graph& g){
    f << g.n() << std::endl;
    for(auto v: g.V()){
        f << v.deg() << " ";
        for(auto u: v.neighbours())
            f << u << " ";
        f << std::endl;
    }
}