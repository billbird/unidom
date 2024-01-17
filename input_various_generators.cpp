/*  input_various_generators.cpp

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
#include <vector>
#include <array>
#include <string>
#include <utility>
#include <algorithm>
#include <functional>
#include "unidom_common.hpp"
#include "vertex_set.hpp"

using std::string;
using std::map;
using std::min;
using std::max;
using std::vector;
using std::array;
using unidom::Solver;
using unidom::InputSource;
using unidom::ArgumentTokenizer;
using unidom::OutputProxy;
using unidom::DominationInstance;

namespace{
class SingleGraphGeneratorBase: public InputSource{
public:
    bool accept_argument(std::string arg, unidom::ArgumentTokenizer& parser){
        if (parameters.find(arg) != parameters.end()){
            set_parameter(arg,parser.get_next_unsigned_int());
        }else
            return unidom::InputSource::accept_argument(arg,parser);
        return true;
    }
    
    SingleGraphGeneratorBase(): already_generated(false){}
    
    
    bool read_next(DominationInstance& inst){
        for(auto P: parameters){
            if (!P.second.set)
                throw unidom::ConfigurableError("Parameter -n missing for generator \""+name()+"\".");
        }
        if (already_generated)
            return false;
        already_generated = true;
        inst.force_in.reset_empty();
        inst.force_out.reset_empty();
        
        generate(inst);
        
        return true;
    }
    
private:
    struct IntParameterProxy{
        IntParameterProxy(int& p, string name): parameter(&p), set(false), name(name) {}
        int* parameter;
        bool set;
        string name;
    };
    map<string, IntParameterProxy> parameters;
protected:
    void add_parameter(int& p, string name){
        parameters.insert(std::make_pair(name,IntParameterProxy(p,name)));
    }
    void set_parameter(string name, int value){
        auto& P = parameters.at(name);
        *P.parameter = value;
        P.set = true;
    }
    virtual void generate(DominationInstance& inst) = 0;
    
private:
    bool already_generated;
};

#define ADD_PARAMETER(p) add_parameter(p, "-"#p )
#define ADD_PARAMETER_DEFAULT(p, d) { add_parameter(p, "-"#p ); set_parameter("-"#p, d); }















/* Code Graph Generator */
class CodeGraphGenerator: public SingleGraphGeneratorBase{
public:
    CodeGraphGenerator(){
        ADD_PARAMETER(n);
        ADD_PARAMETER_DEFAULT(r,1);
        ADD_PARAMETER(base);
    }
protected:

    void generate(DominationInstance& inst){
        if (n == 0)
            throw unidom::ConfigurableError("Parameter -n for generator \""+name()+"\" must be at least 1.");
        Graph& G = inst.G;
        
        int num_verts = int_pow(base,n);
        
        
        AdjMatrix* Mstorage = new AdjMatrix(); //Can't allocate on stack due to size;
        
        AdjMatrix& M = *Mstorage;
        
        for(int i = 0; i < num_verts; i++){
            M[i].fill(unidom::MAX_VERTS);
            M[i][i] = 0;
        }
        
        for(int i = 0; i < num_verts; i++){
            vector<int> digits = get_digits(i);
            for(int j = 0; j < n; j++){
                int old_j = digits[j];
                for(int k = 0; k < base; k++){
                    digits[j] = k;
                    if (k == old_j)
                        continue;
                    int neighbour_idx = get_index(digits);
                    M[i][neighbour_idx] = 1;
                }				
                digits[j] = old_j;
            }
        }
        
        //Floyd warshall algorithm
        //To compute minimum distances between all pairs of vertices
        if (r > 1){
            for (int i = 0; i < num_verts; i++)
                for (int j = 0; j < num_verts; j++)
                    for (int k = 0; k < num_verts; k++)
                        M[i][j] = std::min(M[i][j],  M[i][k]+M[k][j]);
        }
        
        G.reset(num_verts);
        for(int i = 0; i < num_verts; i++){
            for(int j = 0; j < num_verts; j++){
                if (i != j && M[i][j] <= r)
                    G[i].neighbours().push_back(j);
            }
        }
                
        delete Mstorage;
        
        
    }
private:

    typedef array< array<int, unidom::MAX_VERTS>, unidom::MAX_VERTS> AdjMatrix;

    int base_mod(int x){
        while(x < 0)
            x += base;
        return x%base;
    }

    unsigned int int_pow(unsigned int base, unsigned int exp){
        unsigned int result = 1;
        for(int i = 0; i < exp; i++)
            result *= base;
        return result;
    }

    vector<int> get_digits(int index){
        vector<int> result(n,0);
        for(int j = 0; j < n; j++){
            result[n-j-1] = index%base;
            index /= base;
        }
        return result;
    }
    int get_index(const vector<int>& digits){
        int k = 0;
        for(auto j: digits)
            k = k*base + j;
        return k;
    }
    

    int n;
    int r;
    int base;

};
REGISTER_INPUT_SOURCE( CodeGraphGenerator, "code_graph", "Generates a covering code graph: -n sets dimension, -base sets base, -r sets radius (default 1).");











/* Kneser Graph Generator */
class KneserGenerator: public SingleGraphGeneratorBase{
public:
    KneserGenerator(){
        ADD_PARAMETER(n);
        ADD_PARAMETER(k);
    }
protected:

    void generate(DominationInstance& inst){
        if (n == 0)
            throw unidom::ConfigurableError("Parameter -n for generator \""+name()+"\" must be at least 1.");
        if (n > 31)
            throw unidom::ConfigurableError("Parameter -n for generator \""+name()+"\" must be at most 31.");
        
        Graph& G = inst.G;

        vector<int> vertices;
        auto callback = [&vertices](int i){
            vertices.push_back(i);
        };
        generate_by_pop_count(n,k,callback);
        
        
        int num_verts = vertices.size();
        G.reset(num_verts);
        for(unsigned int i = 0; i < num_verts; i++){
            for(unsigned int j = 0; j < num_verts; j++){
                if ((vertices[i] & vertices[j]) == 0)
                    G[i].neighbours().push_back(j);
            }
        }
        
        
    }
private:

    string int_to_binary(int i, int width){
        if (width == 0)
            return "";
        return int_to_binary(i>>1, width-1) + std::to_string(i&1);
    }

    void generate_by_pop_count(int n, int count, std::function< void(int) > callback, int prefix = 0){
        if (count == 0){
            callback(prefix<<n);
            return;
        }
        if (n == 1){
            assert(count == 1); //count == 0 was handled above
            callback((prefix<<1)|1);
            return;
        }
        if (count < n) //Case where bit is zero
            generate_by_pop_count(n-1,count,callback, prefix<<1);
        generate_by_pop_count(n-1,count-1,callback,(prefix<<1)|1);
    }

    int n;
    int k;

};
REGISTER_INPUT_SOURCE( KneserGenerator, "kneser", "Generates a Kneser graph: -n sets dimension, -k sets subset size.");































/* TODO: Move the TG and HR generators to a separate file */

class TriangleBoardGraphGenerator: public SingleGraphGeneratorBase{
public:
    TriangleBoardGraphGenerator(){
        ADD_PARAMETER(n);
    }
    virtual int get_index(int row, int col){
        //Row 0 has 1 vertex, row 1 has 2 vertices, ..., row n-1 has n vertices.
        //There are r*(r+1)/2 vertices above row r
        return (row*(row+1))/2 + col;
    }
    int get_last_n(){
        return n;
    }
protected:
    int n;
};


/* Triangle Grid Graph Generator */
/* Vertex numbering is consistent with Wagon (2014), so the last row of TG(n) has n vertices */
class TrigridGenerator: public TriangleBoardGraphGenerator{
    
protected:
    void generate(DominationInstance& inst){
        if (n == 0)
            throw unidom::ConfigurableError("Parameter -n for generator \""+name()+"\" must be at least 1.");
        Graph& G = inst.G;
        
        int total_verts = get_index(n,0);
        G.reset(total_verts);
        
        for(int i = 0; i < n; i++){
            for (int j = 0; j <= i; j++){ //Row i contains i+1 vertices
                VertIndex v = get_index(i,j);
                auto& neighbours = G[v].neighbours();
                if (i > 0){
                    if (j > 0)
                        neighbours.push_back(get_index(i-1,j-1)); //Northwest neighbour
                    if (j < i)
                        neighbours.push_back(get_index(i-1,j)); //Northeast neighbour
                }
                if (j > 0)
                    neighbours.push_back(get_index(i,j-1)); //West neighbour
                if (j < i)
                    neighbours.push_back(get_index(i,j+1)); //East neighbour
                if (i < n-1){
                    neighbours.push_back(get_index(i+1,j)); //Southwest neighbour
                    neighbours.push_back(get_index(i+1,j+1)); //Southeast neighbour
                }
            }
        }
    }

};
REGISTER_INPUT_SOURCE( TrigridGenerator, "TG", "Generates a Triangular Grid Graph (use -n to set the order).");




/* Hex Rook Graph Generator */
/* Vertex numbering is consistent with Wagon (2014), so the last row of HR(n) has n vertices, and there is no HR(0) */
class HexrookGenerator: public TriangleBoardGraphGenerator{
protected:
    void generate(DominationInstance& inst){
        if (n == 0)
            throw unidom::ConfigurableError("Parameter -n for generator \""+name()+"\" must be at least 1.");
        Graph& G = inst.G;
        
        int total_verts = get_index(n,0);
        G.reset(total_verts);
        
        for(int i = 0; i < n; i++){
            for (int j = 0; j <= i; j++){ //Row i contains i+1 vertices
                VertIndex v = get_index(i,j);
                auto& neighbours = G[v].neighbours();
                //Horizontal neighbours
                for(int k = 0; k <= i; k++)
                    if (k != j)
                        neighbours.push_back(get_index(i,k));
                //Vertical neighbours (60 degrees)
                for(int k = j; k < n; k++)
                    if (k != i)
                        neighbours.push_back(get_index(k,j));
                //Vertical neighbours (120 degrees) (not very elegant)
                for(int k = -n; k < n; k++){
                    int ni = i+k;
                    int nj = j+k;
                    if (ni < 0 || ni >= n || nj < 0 || nj >= n)
                        continue;
                    if (ni == i && nj == j)
                        continue;
                    neighbours.push_back(get_index(ni,nj));
                }
            }
        }
    }
};
REGISTER_INPUT_SOURCE( HexrookGenerator, "hexrook", "Generates a Hex Rook Graph (use -n to set the order).");



//An output filter which prints its results as a triangle graph board (for HR and TG graphs only)
class OutputProxyTriangleBoard: public OutputProxy{
public:
    OutputProxyTriangleBoard(): output_all(false) {}
    bool accept_argument(std::string arg, unidom::ArgumentTokenizer& parser){
        if (arg == "-all"){
            output_all = true;
            return true;
        }
        return unidom::OutputProxy::accept_argument(arg,parser);
    }
    
    void initialize(DominationInstance& inst){
        best_set.reset_full(inst.G.n());
    }
    void process_set(DominationInstance& inst, VertexSet& dominating_set){
        if (output_all)
            output(inst,dominating_set);
        else
            best_set = dominating_set;	
    }
    void output(DominationInstance& inst, VertexSet &s){
        unidom::SolverContext& C = get_solver_context();
        typedef std::shared_ptr<TriangleBoardGraphGenerator> TriangleInputPtr;
        TriangleInputPtr input_source = std::dynamic_pointer_cast<TriangleBoardGraphGenerator>(C.input_source);
        
        if (input_source == nullptr)
            throw unidom::ConfigurableError("triangle_board output proxy requires either hexrook or trigrid input source.");
        
        int n = input_source->get_last_n();
        int total_verts = input_source->get_index(n,0);
        if (C.original_input_graph.n() != total_verts )
            throw unidom::ConfigurableError("Input graph is not a triangle graph.");
        
        if (inst.G.n() != total_verts )
            throw unidom::ConfigurableError("Input was modified after generation and is no longer recognized as a triangle graph.");
        
        
        std::vector< bool > real_set(total_verts,false);
        
        for(VertIndex v: s){
            VertIndex real_index = inst.G[v].get_real_index();
            real_set[real_index] = true;
        }
        
        std::vector< std::vector<bool> > board( n, std::vector<bool>(n, false) );
        
        for(int i = 0; i < n; i++){
            for (int j = 0; j <= i; j++){ //Row i contains i+1 vertices
                int idx = input_source->get_index(i,j);
                board[i][j] = real_set[idx];
            }
        }

        
        if (s.get_size() == inst.G.n()){
            unidom::log << "No dominating set found" << std::endl;
            return;
        }
        
        unidom::log << "Size: " << s.get_size() << std::endl;

        for(int i = 0; i < n; i++){
            for (int j = 0; j <= i; j++)
                std::cout << (board[i][j]?"X":"_") << " ";
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
    void finalize(DominationInstance& inst){
        if (!output_all)
            output(inst,best_set);
    }
private:
    VertexSet best_set;
    bool output_all;
    
};

REGISTER_OUTPUT_PROXY( OutputProxyTriangleBoard, "triangle_board", "Output the best certificate as an n x n triangular board (only works with hexrook/trigrid input sources).");

}