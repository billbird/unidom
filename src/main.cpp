/*  main.cpp

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


#include <vector>
#include <string>
#include "graph.hpp"
#include "graph_util.hpp"
#include "unidom_common.hpp"
#include "unidom_util.hpp"

using std::string;

bool parse_arguments(unidom::SolverContext& C, std::vector<string> args);

int main(int argc, char** argv){
    
    //debug_maps();
    std::vector<string> args;
    for (unsigned int i = 1; i < argc; i++)
        args.push_back(argv[i]);
    
    if (args.size() == 0)
        unidom::log << "Use the -h flag for a list of components" << std::endl;
    
    unidom::SolverContext C;
    
    if(!parse_arguments(C,args))
        return 0;
    

    unidom::log << "Input source: " << C.input_source->name() << " ";
    if (C.preprocess_filters.size() > 0){
        unidom::log << " Filter" << ((C.preprocess_filters.size() == 1)? "":"s") << ": ";
        for(auto F: C.preprocess_filters)
            unidom::log << F->name() << " ";
    }
    unidom::log << "Solver: " << C.solver->name() << " ";
    unidom::log << "Output: " << C.output_proxy->name();
    unidom::log << std::endl;
    
    
    unidom::Timer solver_timer;
    
    while(1){
        unidom::DominationInstance inst;
        if (!C.input_source->read_next(inst))
            break;
        C.original_input_graph = inst.G;
        for(auto F: C.preprocess_filters)
            F->process(inst);
        //TODO add a consistency check for the graph and the force_in/force_out sets
        solver_timer.start();
        C.solver->solve(inst, *C.output_proxy);
        solver_timer.stop();
        unidom::log << "Total Solver Time: " << solver_timer.elapsed_seconds() << std::endl;
    }
    
    return 0;
}