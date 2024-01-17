/*  parse_arguments.cpp

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
#include <cstdlib>
#include <stdexcept>
#include <algorithm>
#include "unidom_common.hpp"

using std::string;
using std::vector;

void debug_maps();

class ArgumentParsingException{
public:
    string message;
    int argument_idx;
    ArgumentParsingException(string m, int idx): message(m), argument_idx(idx){}
};

class StackedArgumentTokenizer: public unidom::ArgumentTokenizer{
public:
    bool has_next(){
        return current_idx < args.size();
    }
    std::string get_next_string(){
        if (current_idx >= args.size())
            throw ArgumentParsingException("Expected string",current_idx+base_idx);
        return args[current_idx++];
    }
    int get_next_int(){
        if (current_idx >= args.size())
            throw ArgumentParsingException("Expected integer",current_idx+base_idx);
        string arg = args[current_idx++];
        try{
            return std::stoi(arg);
        }catch(std::invalid_argument e){
            throw ArgumentParsingException("Expected an integer, not \""+arg+"\"",current_idx+base_idx);
        }
    }
    unsigned int get_next_unsigned_int(){
        if (current_idx >= args.size())
            throw ArgumentParsingException("Expected positive integer",current_idx+base_idx);
        string arg = args[current_idx++];
        try{
            return (unsigned int)std::stoul(arg);
        }catch(std::invalid_argument e){
            throw ArgumentParsingException("Expected a positive integer, not \""+arg+"\"",current_idx+base_idx);
        }
    }
    double get_next_double(){
        if (current_idx >= args.size())
            throw ArgumentParsingException("Expected float",current_idx+base_idx);
        string arg = args[current_idx++];
        try{
            return std::stod(arg);
        }catch(std::invalid_argument e){
            throw ArgumentParsingException("Expected a float, not \""+arg+"\"",current_idx+base_idx);
        }
    }
    
    std::string peek_next_string(){
        if (current_idx >= args.size())
            throw ArgumentParsingException("Expected string",current_idx+base_idx);
        return args[current_idx];
    }
    
    int get_current_idx(){
        return current_idx;
    }
    int get_absolute_idx(){
        return current_idx + base_idx;
    }
    StackedArgumentTokenizer(vector<string>& arg_vector, int idx, int base_index): args(arg_vector), current_idx(idx), base_idx(base_index) {}
    
private:
    std::vector<string>& args;
    int current_idx;
    int base_idx;
};

bool is_root_argument(string s){
    string s2 = s.substr(0,2);
    return s == "-seed" || s == "-h" || s == "-help" || s2 == "-I" || s2 == "-S" || s2 == "-F" || s2 == "-O";
}

void stack_argument_parse(StackedArgumentTokenizer& S, unidom::Configurable& component){
    vector<string> sub_args;
    while(S.has_next() && !is_root_argument(S.peek_next_string()))
        sub_args.push_back(S.get_next_string());
    StackedArgumentTokenizer sub_tokenizer(sub_args,0, S.get_absolute_idx()-sub_args.size());
    if(!component.parse_arguments(sub_tokenizer)){
        int abs_index = std::max(0,sub_tokenizer.get_absolute_idx()-1);
        int sub_index = std::max(0,sub_tokenizer.get_current_idx()-1);
        throw ArgumentParsingException("Invalid argument \""+sub_args[sub_index]+"\"",abs_index);
    }
}


bool parse_arguments(unidom::SolverContext& C, std::vector<string> args){
    StackedArgumentTokenizer S(args,0,0);
    try{
        while(S.has_next()){
            string s = S.get_next_string();
            string s2 = s.substr(0,2);
            if (s == "-seed"){
                unsigned int seed = S.get_next_int();
                unidom::set_random_seed(seed);
            }else if (s == "-help" || s == "-h"){
                unidom::describe_components();
                return false;
            }else if (s2 == "-I"){
                string name = S.get_next_string();
                if (C.input_source != nullptr){
                    unidom::log << "Duplicate input source \""<<name<<"\""<<std::endl;
                    return false;
                }
                C.input_source = unidom::spawn_input_source(name);
                if (C.input_source == nullptr){
                    unidom::log << "Invalid input source \""<<name<<"\""<<std::endl;
                    return false;
                }
                stack_argument_parse(S, *C.input_source);
                C.input_source->set_solver_context(C);
            }else if (s2 == "-S"){
                string name = S.get_next_string();
                if (C.solver != nullptr){
                    unidom::log << "Duplicate solver \""<<name<<"\""<<std::endl;
                    return false;
                }
                C.solver = unidom::spawn_solver(name);
                if (C.solver == nullptr){
                    unidom::log << "Invalid solver \""<<name<<"\""<<std::endl;
                    return false;
                }
                stack_argument_parse(S, *C.solver);
                C.solver->set_solver_context(C);
            }else if (s2 == "-F"){
                string name = S.get_next_string();
                auto filter = unidom::spawn_preprocess_filter(name);
                if (filter == nullptr){
                    unidom::log << "Invalid preprocess filter \""<<name<<"\""<<std::endl;
                    return false;
                }
                C.preprocess_filters.push_back(filter);
                stack_argument_parse(S, *filter);
                filter->set_solver_context(C);
            }else if (s2 == "-O"){
                string name = S.get_next_string();
                if (C.output_proxy != nullptr){
                    unidom::log << "Duplicate output proxy \""<<name<<"\""<<std::endl;
                    return false;
                }
                C.output_proxy = unidom::spawn_output_proxy(name);
                if (C.output_proxy == nullptr){
                    unidom::log << "Invalid output proxy \""<<name<<"\""<<std::endl;
                    return false;
                }
                stack_argument_parse(S, *C.output_proxy);
                C.output_proxy->set_solver_context(C);
            }else{
                throw ArgumentParsingException("Invalid argument \""+s+"\"",S.get_absolute_idx());
            }
        }
    }catch(ArgumentParsingException e){
        int idx = e.argument_idx;
        if (idx >= args.size()){
            unidom::log << "Too few arguments: " << e.message << std::endl;
        }else{
            if (idx > 0)
                unidom::log << "Error parsing arguments (after \"" << args[idx-1] << "\"): " << e.message << std::endl;
            else
                unidom::log << "Error parsing arguments (first argument): " << e.message << std::endl;
        }
        return false;
    }
    if (C.input_source == nullptr)
        C.input_source = unidom::spawn_input_source(unidom::default_input_source);
    if (C.solver == nullptr)
        C.solver = unidom::spawn_solver(unidom::default_solver);
    if (C.output_proxy == nullptr)
        C.output_proxy = unidom::spawn_output_proxy(unidom::default_output_proxy);
    
    return true;
}


