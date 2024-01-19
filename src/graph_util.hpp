/*  graph_util.hpp

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
#ifndef GRAPH_UTIL_H
#define GRAPH_UTIL_H

#include <iostream>
#include <fstream>
#include "graph.hpp"
#include "unidom_common.hpp"

bool read_graph(std::istream& f, Graph& g);

void write_graph(std::ostream& f, Graph& g);

inline std::ostream& operator<<(std::ostream& f, Graph& g){
    write_graph(f,g);
    return f;
}
inline std::istream& operator>>(std::istream& f, Graph& g){
    if(!read_graph(f,g))
        f.setstate(std::ios_base::failbit);
    return f;
}

#endif