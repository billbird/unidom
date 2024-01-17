/*  bishop_graphs.cpp

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
#include <string>
#include "unidom_common.hpp"


using unidom::Solver;
using unidom::InputSource;
using unidom::ArgumentTokenizer;
using unidom::OutputProxy;
using unidom::DominationInstance;
using unidom::PreprocessFilter;

namespace{


class BishopGraphInputSource: public InputSource{
public:
    bool accept_argument(std::string arg, unidom::ArgumentTokenizer& parser){
        if (arg == "-n"){ //Specifying just '-n' will generate one graph.
            n_start = parser.get_next_unsigned_int();
            n_end = n_start;
        }else if (arg == "-start")
            n_start = parser.get_next_unsigned_int();
        else if (arg == "-end")
            n_end = parser.get_next_unsigned_int();
        else
            return unidom::InputSource::accept_argument(arg,parser);
        return true;
    }
    
    BishopGraphInputSource(): n_start(-1), n_end(-1), last_n(-1) {}
    bool read_next(DominationInstance& inst){
        if (n_start == -1 || n_end == -1)
            throw unidom::ConfigurableError("No size parameter (-n) specified for bishop generator.");
        if (n_start > n_end)
            return false;
        int n = n_start;
        last_n = n;
        
        n_start++;
        inst.force_in.reset_empty();
        inst.force_out.reset_empty();
        Graph& G = inst.G;
        
        int num_verts = n*n;
        G.reset(num_verts);
        
        for (int vi = 0; vi < n; vi++){
            for(int vj = 0; vj < n; vj++){
                VertIndex v = vi*n + vj;
                int ui, uj;
                //Add all cells on forward diagonal (not very elegant)
                for (int k = -n; k < n; k++){
                    ui = vi + k;
                    uj = vj + k;
                    if (ui < 0 || ui >= n || uj < 0 || uj >= n)
                        continue;
                    VertIndex u = ui*n + uj;
                    if (u == v)
                        continue;
                    G[v].neighbours().push_back(u);
                }
                //Add all cells on backward diagonal
                for (int k = -n; k < n; k++){
                    ui = vi + k;
                    uj = vj - k;
                    if (ui < 0 || ui >= n || uj < 0 || uj >= n)
                        continue;
                    VertIndex u = ui*n + uj;
                    if (u == v)
                        continue;
                    G[v].neighbours().push_back(u);
                }
                
            }
        }
        
        for(Graph::Vertex& v: G.V())
            if (v.deg() >= unidom::MAX_DEGREE)
                throw unidom::ConfigurableError("Degree of queen graph exceeds MAX_DEGREE");
        
        return true;
    }
public:	
    int get_last_n(){
        return last_n;
    }
protected:
    int n_start, n_end; //[n_start,n_end] is the range of graphs to generate

private:
    int last_n; //Value of n for the last generated graph
};

REGISTER_INPUT_SOURCE( BishopGraphInputSource, "bishop", "Generates a bishop graph (use -n to set board size)");





/* Border variants */






//Queen graphs restricted to dominating vertices on the border
class BorderBishopInputSource: public BishopGraphInputSource{
public:
    bool read_next(DominationInstance& inst){
        if (!BishopGraphInputSource::read_next(inst))
            return false;
        int n = get_last_n();
        
        //Add all of the cells in [1,n-2]X[1,n-2] to the force_out set.
        for(VertIndex vi = 1; vi < n-1; vi++)
            for(VertIndex vj = 1; vj < n-1; vj++){
                VertIndex v = vi*n + vj;
                inst.force_out.add(v);
            }
                
        return true;
    }
};
REGISTER_INPUT_SOURCE( BorderBishopInputSource, "border_bishop", "Generates a bishop graph (use -n to set board size) for the border bishop problem, with internal cells restricted.");

//An output filter which prints its results as a queen graph board.
class OutputProxyBishopBoard: public OutputProxy{
public:
    OutputProxyBishopBoard() {}
    bool accept_argument(std::string arg, unidom::ArgumentTokenizer& parser){
        return unidom::OutputProxy::accept_argument(arg,parser);
    }
    
    void initialize(DominationInstance& inst){
        best_set.reset_full(inst.G.n());
    }
    void process_set(DominationInstance& inst, VertexSet& dominating_set){
        best_set = dominating_set;
    }
    void finalize(DominationInstance& inst){
        //Verify that the input source was a BishopGraphInputSource
        unidom::SolverContext& C = get_solver_context();
        
        typedef std::shared_ptr<BishopGraphInputSource> QueenInputPtr;
        QueenInputPtr input_source = std::dynamic_pointer_cast<BishopGraphInputSource>(C.input_source);
        if (input_source == nullptr)
            throw unidom::ConfigurableError("bishop_board output proxy requires bishop graph input source.");
        
        int n = input_source->get_last_n();
        if (C.original_input_graph.n() != n*n)
            throw unidom::ConfigurableError("Input graph is not a bishop graph.");
        
        if (inst.G.n() != n*n)
            throw unidom::ConfigurableError("Input was modified after generation and is no longer recognized as a bishop graph.");
        
        std::vector< std::vector<bool> > board( n, std::vector<bool>(n, false) );
        for(VertIndex v: best_set){
            VertIndex real_index = inst.G[v].get_real_index();
            int r = v/n;
            int c = v%n;
            board[r][c] = true;
        }
        
        if (best_set.get_size() == inst.G.n()){
            unidom::log << "No dominating set found" << std::endl;
            return;
        }
        
        unidom::log << "Size: " << best_set.get_size() << std::endl;
        for(auto& row: board){
            for(auto entry: row)
                std::cout << (entry?"Q":"_") << " ";
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
private:
    VertexSet best_set;
    
};

REGISTER_OUTPUT_PROXY( OutputProxyBishopBoard, "bishop_board", "Output the best certificate as an n x n chess board (only works with bishop graph input sources).");




}