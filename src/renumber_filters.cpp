/*  renumber_filters.cpp

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

using unidom::ArgumentTokenizer;
using unidom::DominationInstance;
using unidom::PreprocessFilter;
using unidom::MAX_VERTS;
using unidom::MAX_DEGREE;

class RenumberFilter: public PreprocessFilter{
    void process(DominationInstance& inst){
        int n = inst.G.n();
        vector<VertIndex> permuted_numbering = compute_new_numbering(inst);
        
        decltype(permuted_numbering) inverse_perm(n);
        
        for(unsigned int i = 0; i < n; i++)
            inverse_perm[permuted_numbering[i]] = i;
        
        DominationInstance new_inst;
        inst.G.renumber(permuted_numbering,new_inst.G);
        
        for(VertIndex v: inst.force_in)
            new_inst.force_in.add( inverse_perm[v] );
        
        for(VertIndex v: inst.force_out)
            new_inst.force_out.add( inverse_perm[v] );
        inst = new_inst;
        
    }
    virtual vector<VertIndex> compute_new_numbering(DominationInstance& inst) = 0;
};
    
class RenumberMinDeg: public RenumberFilter{
public:
    vector<VertIndex> compute_new_numbering(DominationInstance& inst){
        Graph& G = inst.G;
        int n = G.n();
        vector<VertIndex> result(n);
        for(int i = 0; i < n; i++)
            result[i] = i;
        
        auto cmp = [&G](const VertIndex &a, const VertIndex &b){
            //Sort into ascending order
            return (G[a].deg() < G[b].deg());
        };
        std::stable_sort( std::begin(result), std::end(result), cmp);
        return result;
        
    }

};

REGISTER_PREPROCESS_FILTER( RenumberMinDeg, "renumber_mindeg", "Renumber vertices with low-degree vertices first");

    
class RenumberMaxDeg: public RenumberFilter{
public:
    vector<VertIndex> compute_new_numbering(DominationInstance& inst){
        Graph& G = inst.G;
        int n = G.n();
        vector<VertIndex> result(n);
        for(int i = 0; i < n; i++)
            result[i] = i;
        
        auto cmp = [&G](const VertIndex &a, const VertIndex &b){
            //Sort into descending order
            return G[a].deg() > G[b].deg();
        };
        std::stable_sort( std::begin(result), std::end(result), cmp);
        return result;	
    }

};

REGISTER_PREPROCESS_FILTER( RenumberMaxDeg, "renumber_maxdeg", "Renumber vertices with high-degree vertices first");


    
class RenumberBFS: public RenumberFilter{
public:
    RenumberBFS(): bfs_root(0){}
    bool accept_argument(std::string arg, unidom::ArgumentTokenizer& parser){
        if (arg == "-root"){
            bfs_root = parser.get_next_int();
            return true;
        }
        return RenumberFilter::accept_argument(arg,parser);
    }
    vector<VertIndex> compute_new_numbering(DominationInstance& inst){
        Graph& G = inst.G;
        int n = G.n();
        
        vector<bool> covered(n,false);
        vector<VertIndex> result;
        int start = bfs_root;
        result.push_back(bfs_root);
        covered[bfs_root] = true;
        while(start < result.size()){
            VertIndex v = result[start++];
            for(VertIndex u: G[v].neighbours()){
                if (covered[u])
                    continue;
                result.push_back(u);
                covered[u] = true;
            }
        }
        assert(result.size() == n);
        return result;	
    }
    
private:
    VertIndex bfs_root;

};

REGISTER_PREPROCESS_FILTER( RenumberBFS, "renumber_bfs", "Renumber vertices in BFS ordering rooted at vertex 0");


    
class RenumberRandom: public RenumberFilter{
public:
    bool accept_argument(std::string arg, unidom::ArgumentTokenizer& parser){
        if (arg == "-seed"){
            unidom::set_random_seed(parser.get_next_int());
            return true;
        }
        return RenumberFilter::accept_argument(arg,parser);
    }
    vector<VertIndex> compute_new_numbering(DominationInstance& inst){
        Graph& G = inst.G;
        int n = G.n();
        
        //Generate a random permutation with a Knuth shuffle
        vector<VertIndex> result(n);
        for(int i = 0; i < n; i++)
            result[i] = i;
        for(int i = 0; i < n; i++){
            int j = unidom::random_in_range(i,n-1);
            std::swap( result[i], result[j] );
        }
        
        return result;	
    }

};

REGISTER_PREPROCESS_FILTER( RenumberRandom, "renumber_random", "Randomly renumber the graph (use -seed to set seed)");
