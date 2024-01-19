/*  unidom_common.hpp

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

#ifndef UNIDOM_COMMON_H
#define UNIDOM_COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <typeinfo>
#include <memory>
#include "unidom_constants.hpp"
#include "graph.hpp"
#include "vertex_set.hpp"

namespace unidom{	

    struct DominationInstance{
        Graph G;
        VertexSet force_in; //Set of vertices that must be in the set
        VertexSet force_out; //Set of vertices that must not be in the set
    };
    
    
    
    
    class ArgumentTokenizer{
    public:
        virtual std::string get_next_string() = 0;
        virtual int get_next_int() = 0;
        virtual unsigned int get_next_unsigned_int() = 0;
        virtual double get_next_double() = 0;
        virtual bool has_next() = 0;
    };
    
    class ConfigurableError{
    public:
        ConfigurableError(std::string s){
            message = s;
        }
        std::string what(){
            return message;
        }
    private:
        std::string message;
    };
    
    class ComponentNotFoundError{
    public:
        ComponentNotFoundError(std::string s){
            message = s;
        }
        std::string what(){
            return message;
        }
    private:
        std::string message;
    };
    
    class SolverContext;
    
    class Configurable{
    public:
        virtual std::string name() = 0;
        virtual std::string description() = 0;
        virtual bool parse_arguments(unidom::ArgumentTokenizer& parser){
            while(parser.has_next()){
                std::string arg = parser.get_next_string();
                if (!accept_argument(arg, parser))
                    return false;
            }
            return true;
        }
        virtual bool accept_argument(std::string argument, unidom::ArgumentTokenizer& parser){
            return false;
        }
        virtual void set_solver_context(unidom::SolverContext& c){
            solver_context = &c;
        }
    protected:
        SolverContext& get_solver_context(){
            return *solver_context;
        }
    private:
        SolverContext* solver_context;
    };
    
    class InputSource: public Configurable{
    public:
        virtual bool read_next(DominationInstance& inst) = 0;
    };
    
    class OutputProxy: public Configurable{
    public:
        //May be thrown during process_set. If thrown, then the solver should
        //terminate backtracking and finalize.
        class TerminateOutput{ };
        virtual void initialize(DominationInstance& inst){ }
        virtual void process_set(DominationInstance& inst, VertexSet& dominating_set) = 0;
        virtual void finalize(DominationInstance& inst){ }
    };
    
    class PreprocessFilter: public Configurable{
    public:
        virtual void process(DominationInstance& inst) = 0;
    };
    
    class Solver: public Configurable{
    public:
        virtual void solve(DominationInstance& inst, OutputProxy& output_proxy) = 0;
    };
    
    typedef std::shared_ptr<Solver> SolverPtr;
    typedef std::shared_ptr<InputSource> InputSourcePtr;
    typedef std::shared_ptr<OutputProxy> OutputProxyPtr;
    typedef std::shared_ptr<PreprocessFilter> PreprocessFilterPtr;
    
    
    class SolverContext{
    public:
        InputSourcePtr input_source;
        std::vector<PreprocessFilterPtr> preprocess_filters;
        SolverPtr solver;
        OutputProxyPtr output_proxy;
        
        Graph original_input_graph;
        
        SolverContext(): input_source(nullptr), solver(nullptr), output_proxy(nullptr){}
    };
    
    
    
    
    
    
    
    
    
    
    void set_random_seed(unsigned int seed);
    unsigned int random_in_range(unsigned int lower, unsigned int upper); //Range is inclusive
    
    void describe_components();
    
    /* Factory classes for automatic registration of components as they
       are added to the code */
    
    
    template<typename T>
    class ConfigurableFactoryBase;
    
    
    template<typename T>
    class ConfigurableFactoryBase{
    public:
        virtual std::shared_ptr<T> generate() = 0;
        virtual std::string name() = 0;
        virtual std::string description() = 0;
    };
    
    typedef ConfigurableFactoryBase<Solver> SolverFactory;
    typedef ConfigurableFactoryBase<InputSource> InputSourceFactory;
    typedef ConfigurableFactoryBase<OutputProxy> OutputProxyFactory;
    typedef ConfigurableFactoryBase<PreprocessFilter> PreprocessFilterFactory;
    
    void register_configurable_factory(SolverFactory& f);
    SolverPtr spawn_solver(std::string name);
    void register_configurable_factory(InputSourceFactory& f);
    InputSourcePtr spawn_input_source(std::string name);
    void register_configurable_factory(OutputProxyFactory& f);
    OutputProxyPtr spawn_output_proxy(std::string name);
    void register_configurable_factory(PreprocessFilterFactory& f);
    PreprocessFilterPtr spawn_preprocess_filter(std::string name);
    
    
    template<typename T, typename FactoryType>
    class RegisteredConfigurableFactory: public ConfigurableFactoryBase<FactoryType>{
    public:
        RegisteredConfigurableFactory(std::string component_name, std::string desc){
            this->desc = desc;
            this->component_name = component_name;
            register_configurable_factory(*this);
        }
        std::shared_ptr<FactoryType> generate(){
            return std::make_shared<T>();
        }
        std::string name(){
            return component_name;
        }
        std::string description(){
            return desc;
        }
    private:
        std::string component_name;
        std::string desc;
    };
    
    template<typename T, typename FactoryType>
    class ConfigurableManufacturingProxy: public T{
    public:
        static RegisteredConfigurableFactory< ConfigurableManufacturingProxy<T,FactoryType> , FactoryType> Factory;
        std::string name(){
            return Factory.name();
        }
        std::string description(){
            return Factory.description();
        }
    };
    
    
};

#define REGISTER_CONFIGURABLE( class_name, component_name, description, configurable_type ) \
    namespace{ \
        template<> \
        unidom::RegisteredConfigurableFactory< unidom::ConfigurableManufacturingProxy<class_name, configurable_type> , configurable_type> \
            unidom::ConfigurableManufacturingProxy<class_name, configurable_type>::Factory(component_name,description);\
    }

#define REGISTER_SOLVER( class_name, component_name, description ) REGISTER_CONFIGURABLE( class_name, component_name, description, Solver )
#define REGISTER_INPUT_SOURCE( class_name, component_name, description ) REGISTER_CONFIGURABLE( class_name, component_name, description, InputSource )
#define REGISTER_OUTPUT_PROXY( class_name, component_name, description ) REGISTER_CONFIGURABLE( class_name, component_name, description, OutputProxy )
#define REGISTER_PREPROCESS_FILTER( class_name, component_name, description ) REGISTER_CONFIGURABLE( class_name, component_name, description, PreprocessFilter )

#define REGISTER_SOLVER_ALIAS( class_name, alias_class_name, component_name, description ) \
    namespace{\
        class alias_class_name: public class_name{};\
    }\
    REGISTER_CONFIGURABLE( alias_class_name, component_name, description, Solver )
    
#endif