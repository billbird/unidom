/*  force_filters.cpp

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
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <cassert>
#include "unidom_common.hpp"

using std::string;
using std::vector;
using std::min;
using std::max;

using unidom::Solver;
using unidom::InputSource;
using unidom::ArgumentTokenizer;
using unidom::DominationInstance;
using unidom::PreprocessFilter;

class ForceInFilter: public PreprocessFilter{
    bool accept_argument(std::string arg, unidom::ArgumentTokenizer& parser){
        try{
            vertices.push_back(std::stoi(arg));
        }catch(std::invalid_argument e){
            //throw ArgumentParsingException("Expected an integer, not \""+arg+"\"");
            return false;
        }
        return true;
    }
    void process(DominationInstance& inst){
        int n = inst.G.n();

        for(auto v: vertices){
            if (v < 0 || v >= n){
                throw unidom::ConfigurableError("Vertex index " + std::to_string(v) + " is invalid.");
            }
            if (!inst.force_in.contains(v))
                inst.force_in.add( v );
        }		
    }
private:
    vector<VertIndex> vertices;
};

REGISTER_PREPROCESS_FILTER( ForceInFilter, "force_in", "Force some vertices to be included in the dominating set (specify vertex indices after '-F force_in').");



class ForceOutFilter: public PreprocessFilter{
    bool accept_argument(std::string arg, unidom::ArgumentTokenizer& parser){
        try{
            vertices.push_back(std::stoi(arg));
        }catch(std::invalid_argument e){
            //throw ArgumentParsingException("Expected an integer, not \""+arg+"\"");
            return false;
        }
        return true;
    }
    void process(DominationInstance& inst){
        int n = inst.G.n();

        for(auto v: vertices){
            if (v < 0 || v >= n){
                throw unidom::ConfigurableError("Vertex index " + std::to_string(v) + " is invalid.");
            }
            if (!inst.force_out.contains(v))
                inst.force_out.add( v );
        }		
    }
private:
    vector<VertIndex> vertices;
};
REGISTER_PREPROCESS_FILTER( ForceOutFilter, "force_out", "Force some vertices to be excluded from the dominating set (specify vertex indices after '-F force_out').");
