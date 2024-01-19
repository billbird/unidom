/*  unidom_common.cpp

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
#include <vector>
#include <algorithm>
#include <random>
#include "unidom_common.hpp"

using std::string;
using std::map;


using namespace unidom;



std::ostream& unidom::log = std::cerr;

//The maps which map component names to component factories may not be
//initialized in time if we just declare them as global variables, so 
//we have to use the following disgusting workaround.

namespace{
    template<typename T>
    map<string, ConfigurableFactoryBase<T>* >& get_component_map(){
        static map<string, ConfigurableFactoryBase<T>* > m;
        return m;
    }
    template<typename T>
    ConfigurableFactoryBase<T>& lookup_component(string name){
        return *get_component_map<T>().at(name);
    }
    template<typename T>
    bool component_exists(string name){
        auto& m = get_component_map<T>();
        return m.find(name) != m.end();
    }
    template<typename T>
    void add_component(ConfigurableFactoryBase<T>& component){
        get_component_map<T>()[component.name()] = &component;
    }
    
    map<string, SolverFactory*>& get_solver_map(){
        return get_component_map<Solver>();
    }
    map<string, InputSourceFactory*>& get_input_source_map(){
        return get_component_map<InputSource>();
    }
    map<string, OutputProxyFactory*>& get_output_proxy_map(){
        return get_component_map<OutputProxy>();
    }
    
    map<string, PreprocessFilterFactory*>& get_preprocess_filter_map(){
        return get_component_map<PreprocessFilter>();
    }

}


void unidom::register_configurable_factory(SolverFactory& f){
    add_component(f);
}
SolverPtr unidom::spawn_solver(std::string name){
    if (!component_exists<Solver>(name))
        return nullptr;
    SolverPtr result =lookup_component<Solver>(name).generate();
    if (name != result->name())
        unidom::log << "Warning: Tried to spawn solver \"" << name << "\" and got \"" << result->name() << "\"" << std::endl;
    return result;
}

void unidom::register_configurable_factory(InputSourceFactory& f){
    add_component(f);
}
InputSourcePtr unidom::spawn_input_source(std::string name){
    if (!component_exists<InputSource>(name))
        return nullptr;
    return lookup_component<InputSource>(name).generate();
}

void unidom::register_configurable_factory(OutputProxyFactory& f){
    add_component(f);
}
OutputProxyPtr unidom::spawn_output_proxy(std::string name){
    if (!component_exists<OutputProxy>(name))
        return nullptr;
    return lookup_component<OutputProxy>(name).generate();
}

void unidom::register_configurable_factory(PreprocessFilterFactory& f){
    add_component(f);
}
PreprocessFilterPtr unidom::spawn_preprocess_filter(std::string name){
    if (!component_exists<PreprocessFilter>(name))
        return nullptr;
    return lookup_component<PreprocessFilter>(name).generate();
}


void debug_maps(){
    using unidom::log;
    log << "Debugging maps" << std::endl;
    auto solvers = get_solver_map();
    for(auto P: solvers){
        auto& s = *P.second;
        log << "Solver: " << s.name() << " - " << s.description() << std::endl;
    }
    auto input_sources = get_input_source_map();
    for(auto P: input_sources){
        auto& s = *P.second;
        log << "Input Source: " << s.name() << " - " << s.description() << std::endl;
    }
    auto output_proxies = get_output_proxy_map();
    for(auto P: output_proxies){
        auto& s = *P.second;
        log << "Input Source: " << s.name() << " - " << s.description() << std::endl;
    }
}


namespace{
    template<typename T>
    std::vector<typename T::key_type> sorted_keys(T& source_map){
        std::vector<typename T::key_type> V;
        for (auto P: source_map){
            V.push_back(P.first);
        }
        std::sort(V.begin(), V.end());
    }
}

void unidom::describe_components(){
    log << "Available components:" << std::endl;
    
    log << "Input sources (-I) - Default: " << default_input_source << std::endl;
    auto input_sources = get_input_source_map();
    for(auto P: input_sources){
        auto& s = *P.second;
        log << "\t" << s.name() << ": " << s.description() << std::endl;
    }
    
    log << "Preprocessing Filters (-F)" << std::endl;
    auto filters = get_preprocess_filter_map();
    for(auto P: filters){
        auto& s = *P.second;
        log << "\t" << s.name() << ": " << s.description() << std::endl;
    }
    
    log << "Solvers (-S) - Default: " << default_solver << std::endl;
    auto solvers = get_solver_map();
    for(auto P: solvers){
        auto& s = *P.second;
        log << "\t" << s.name() << ": " << s.description() << std::endl;
    }
    
    log << "Output proxies (-O) - Default: " << default_output_proxy << std::endl;
    auto output_proxies = get_output_proxy_map();
    for(auto P: output_proxies){
        auto& s = *P.second;
        log << "\t" << s.name() << ": " << s.description() << std::endl;
    }
}


namespace{
    std::mt19937 random_generator(1);
}

void unidom::set_random_seed(unsigned int seed){
    random_generator.seed(seed);
}

unsigned int unidom::random_in_range(unsigned int lower, unsigned int upper){
    std::uniform_int_distribution<int> range(lower,upper);
    return range(random_generator);
}