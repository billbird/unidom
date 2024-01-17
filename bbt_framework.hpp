/*  bbt_framework.hpp

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

#ifndef BBT_FRAMEWORK_H
#define BBT_FRAMEWORK_H

#include <iostream>
#include <iomanip>
#include <string>
#include <array>
#include "unidom_common.hpp"


class BBTFrameworkSolver: public unidom::Solver{
public:
    
    BBTFrameworkSolver(){
        reset_depth_log();
        resmod_mod = 1;
        resmod_res = 0;
        resmod_depth = INVALID_DEPTH;
        total_upper_bound = unidom::MAX_VERTS;
        total_lower_bound = 0;
        verbose = false;
    }
    
    void duplicate_settings_only(BBTFrameworkSolver& other){
        reset_depth_log();
        resmod_mod = other.resmod_mod;
        resmod_res = other.resmod_res;
        resmod_depth = other.resmod_depth;
        total_upper_bound = other.total_upper_bound;
        total_lower_bound = other.total_lower_bound;
    }
    
    bool accept_argument(std::string arg, unidom::ArgumentTokenizer& parser){
        if (arg == "-res")
            resmod_res = parser.get_next_unsigned_int();
        else if(arg == "-mod")
            resmod_mod = parser.get_next_unsigned_int();
        else if(arg == "-resmod_depth")
            resmod_depth = parser.get_next_unsigned_int();
        else if(arg == "-u" || arg == "-max")
            total_upper_bound = parser.get_next_unsigned_int();
        else if(arg == "-l" || arg == "-min")
            total_lower_bound = parser.get_next_unsigned_int();
        else if(arg == "-quiet")
            verbose = false;
        else if(arg == "-verbose")
            verbose = true;
        else
            return unidom::Solver::accept_argument(arg,parser);
        return true;
    }
    
    
protected:
    
    
    void reset_depth_log(){
        depth_log.fill(0);
    }
    //Returns 0 if the current branch should be terminated for violating
    //the res/mod conditions, -1 if the current branch should continue but
    //may eventually violate the res/mod conditions, and 1 if the current branch
    //should continue and can avoid checking the conditions ever again.
    template<bool check_resmod_depth>
    int report_node(int depth){
        depth_log[(unsigned int)depth]++;
        if (check_resmod_depth){
            if(depth == resmod_depth){
                if((depth_log[(unsigned int)depth]-1)%resmod_mod == resmod_res)
                    return 1;
                else
                    return 0;
            }else{
                assert(depth < resmod_depth);
                return -1;
            }
        }else{
            return 1;
        }
    }
    void unreport_node(int depth){
        depth_log[(unsigned int)depth]--;
    }
    
    void print_depth_log(){
        using unidom::log;
        if (!verbose)
            return;
        log << "Depth Log:" << std::endl;
        int max_depth = 0;
        for (int i = 0; i < unidom::MAX_VERTS; i++)
            if (depth_log[i] > 0)
                max_depth = i;
        unsigned long long int total_count = 0;
        for(int i = 0; i <= max_depth; i++){
            log << std::setw(2) << i << ": " << depth_log[i] << std::endl;
            total_count += depth_log[i];
        }
        log<< "End Depth Log"<<std::endl;
        log<<"Total Logged Calls: "<<total_count<<std::endl;
    }
    
    const unsigned int INVALID_DEPTH = (unsigned int)(-1);
    
    unsigned int resmod_mod;
    unsigned int resmod_res;
    unsigned int resmod_depth;
    
    unsigned int total_lower_bound; //Only sets with at least this size will be generated
    unsigned int total_upper_bound; //No sets with size larger than this will be generated
    
    std::array<unsigned long long int, unidom::MAX_VERTS> depth_log;
    
    bool verbose;
    
    
};


#endif