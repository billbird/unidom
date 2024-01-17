/*  basic_io.cpp

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
#include <map>
#include <string>
#include "unidom_common.hpp"
#include "graph_util.hpp"


using unidom::Solver;
using unidom::InputSource;
using unidom::ArgumentTokenizer;
using unidom::OutputProxy;
using unidom::DominationInstance;
using unidom::PreprocessFilter;

class SimpleGraphInputSource: public InputSource{
public:
    bool read_next(DominationInstance& inst){
        inst.force_in.reset_empty();
        inst.force_out.reset_empty();
        if (read_graph(std::cin,inst.G))
            return true;
        return false;
    }
};

REGISTER_INPUT_SOURCE( SimpleGraphInputSource, "basic_input", "Read adjacency lists from standard input");

class OutputProxyOutputAll: public OutputProxy{
public:
    OutputProxyOutputAll(): total_solutions(0), print_stats(true) {}
    bool accept_argument(std::string arg, unidom::ArgumentTokenizer& parser){
        if (arg == "-stats")
            print_stats = true;
        else if (arg == "-nostats")
            print_stats = false;
        else
            return unidom::OutputProxy::accept_argument(arg,parser);
        return true;
    }
    
    void initialize(DominationInstance& inst){
        total_solutions = 0;
    }
    void process_set(DominationInstance& inst, VertexSet& dominating_set){
        total_solutions++;
        std::cout << dominating_set.get_size() << " ";
        for(VertIndex i: dominating_set)
            std::cout << inst.G[i].get_real_index() << " ";
        std::cout << std::endl;
    }
    void finalize(DominationInstance& inst){
        std::cout << -1 << std::endl;
        unidom::log << "Total Solutions Generated: " << total_solutions << std::endl;
    }
private:
    bool print_stats;
    int total_solutions;
    
};

REGISTER_OUTPUT_PROXY( OutputProxyOutputAll, "output_all", "Output each certificate on its own line, followed by -1");



class OutputProxyOutputBest: public OutputProxy{
public:
    OutputProxyOutputBest(): print_graph(false), print_stats(true), size_only(false) {}
    bool accept_argument(std::string arg, unidom::ArgumentTokenizer& parser){
        if (arg == "-stats")
            print_stats = true;
        else if (arg == "-nostats")
            print_stats = false;
        else if (arg == "-gamma")
            size_only = true;
        else if (arg == "-size_only")
            size_only = true;
        else if (arg == "-size-only")
            size_only = true;
        else if (arg == "-graph")
            print_graph = true;
        else
            return unidom::OutputProxy::accept_argument(arg,parser);
        return true;
    }
    
    void initialize(DominationInstance& inst){
        best_set.reset_full(inst.G.n());
    }
    void process_set(DominationInstance& inst, VertexSet& dominating_set){
        best_set = dominating_set;
    }
    void finalize(DominationInstance& inst){
        if (print_graph){
            //std::cout << inst.G << std::endl;
            std::cout << get_solver_context().original_input_graph << std::endl;
        }
        
        std::cout << best_set.get_size() << " ";
        if (!size_only){
            for(VertIndex i: best_set)
                std::cout << inst.G[i].get_real_index() << " ";
        }
        std::cout << std::endl;
    }
private:
    VertexSet best_set;
    bool print_stats;
    bool print_graph;
    bool size_only;
    
};

REGISTER_OUTPUT_PROXY( OutputProxyOutputBest, "output_best", "Output the last certificate only. Use -graph flag to output the graph before the certificate.");



class OutputProxyOutputGraphOnly: public OutputProxy{
public:
    void initialize(DominationInstance& inst){
    }
    void process_set(DominationInstance& inst, VertexSet& dominating_set){
    }
    void finalize(DominationInstance& inst){
        std::cout << inst.G << std::endl;
    }
    
};

REGISTER_OUTPUT_PROXY( OutputProxyOutputGraphOnly, "graph_only", "Output the graph only (ignore all dominating sets).");




class PrintGraphFilter: public PreprocessFilter{
public:
    void process(DominationInstance& inst){
        unidom::log << inst.G << std::endl;
    }

};

REGISTER_PREPROCESS_FILTER( PrintGraphFilter, "print_graph_stderr", "Print the graph to stderr.");

