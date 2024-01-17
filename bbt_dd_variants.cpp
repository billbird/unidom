/*  bbt_dd_variants.cpp

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
#include <array>
#include <vector>
#include <algorithm>
#include <cassert>
#include "unidom_common.hpp"
#include "unidom_arrayutil.hpp"
#include "bbt_framework.hpp"
#include "bbt_degreepq.hpp"
#include "graph_util.hpp"

using std::string;
using std::array;
using std::min;
using std::max;

using unidom::Solver;
using unidom::OutputProxy;
using unidom::DominationInstance;
using unidom::MAX_VERTS;
using unidom::MAX_DEGREE;
using unidom::array_range;
using unidom::iterate_reverse;



namespace{
    const int CHOOSE_VERTEX_MIN_CD = 0;
    const int CHOOSE_VERTEX_MAX_CD = 1;
    
    const int RANK_NEIGHBOURS_ASCENDING = 0;
    const int RANK_NEIGHBOURS_DESCENDING = 1;	
}

template<unsigned int CHOOSE_VERTEX_RULE, unsigned int RANK_NEIGHBOURS_RULE, bool FORCE_STOP_ON_TRAPPED_VERTEX, bool RECHECK_BOUNDS_IN_LOOP, bool GENERATE_ALL>
class BBTDDSolverVariant: public BBTFrameworkSolver{
    static_assert( CHOOSE_VERTEX_RULE <= CHOOSE_VERTEX_MAX_CD, "CHOOSE_VERTEX_RULE must be either CHOOSE_VERTEX_MIN_CD or CHOOSE_VERTEX_MAX_CD" );
    static_assert( RANK_NEIGHBOURS_RULE <= RANK_NEIGHBOURS_DESCENDING, "RANK_NEIGHBOURS_RULE must be either RANK_NEIGHBOURS_ASCENDING or RANK_NEIGHBOURS_DESCENDING" );
public:
    void solve(DominationInstance& inst, unidom::OutputProxy& output_proxy){
        dom_inst = &inst;
        Graph& G = inst.G;
        this->output_proxy = &output_proxy;
        
        
        add_loops(G);
        sort_neighbours_descending(G);
        
        
        int n = inst.G.n();
        D.reset();
        B.reset_full(n-1);
        
        if (!GENERATE_ALL && total_upper_bound < n)
            B.reset_full(total_upper_bound+1);
        
        covered.fill(0);
        fixed.fill(0);
        
        total_covered = 0;
        total_fixed = 0;
        
        DegreePQLight UD_DPQ(G);
        DegreePQHeavy C_DPQ(G);
        
        UndominatedDPQ = &UD_DPQ;
        CandidateDPQ = &C_DPQ;
        
        //Add all of the "force_in" vertices to the dominating set
        
        for(VertIndex v: inst.force_in){
            remove_candidate(G,v);
            D.add(v);
            for(VertIndex u: G[v].neighbours()){
                dominate(G,u);
            }
        }
        
        
        //Set all of the "force_out" vertices to be forbidden
        for(VertIndex v: inst.force_out){
            remove_candidate(G,v);
        }
        
        
        
        
        reset_depth_log();
        
        output_proxy.initialize(inst);
        FindDominatingSet<true>(G);
        output_proxy.finalize(inst);
        
        print_depth_log();
        
        UndominatedDPQ = nullptr;
        CandidateDPQ = nullptr;
        
    }
    
private:
    DominationInstance* dom_inst;
    OutputProxy* output_proxy;
    
    VertexSet D; //Current working set
    VertexSet B; //Best set found so far
    
    DegreePQLight* UndominatedDPQ; //Tracks the domination degree of each vertex
    DegreePQHeavy* CandidateDPQ; //Tracks the candidate degree of each vertex
    
    array<int,MAX_VERTS> covered, fixed;
    int total_covered, total_fixed;
        
    void sort_neighbours_descending(Graph& G){
        auto cmp = [&G](const VertIndex &a, const VertIndex &b){
            //Sort into descending order
            return a > b;
        };
        for(auto& v: G.V()){
            std::stable_sort(v.neighbours().begin(), v.neighbours().end(),cmp);
        }
    }
    
    void add_loops(Graph& G){
        for(auto& v: G.V())
            v.add_neighbour_simple(v.get_index());
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    void add_candidate(Graph& G, VertIndex v){
        assert(fixed[v]);
        fixed[v] = 0;
        total_fixed--;
        UndominatedDPQ->add_candidate(v);
        CandidateDPQ->add_candidate(v);
        for(VertIndex u: G[v].neighbours()) //Congruent to original, but should be reversed
            CandidateDPQ->increment(u);
        
    }
    bool remove_candidate(Graph& G, VertIndex v){ //Returns true if v must be in the dominating set.
        assert(!fixed[v]);
        fixed[v] = 1;
        total_fixed++;
        UndominatedDPQ->remove_candidate(v);
        CandidateDPQ->remove_candidate(v);
        bool forced = false;
        for(VertIndex u: G[v].neighbours())
            if (CandidateDPQ->decrement(u) == 0 && !covered[u]) //If u ends up with candidate degree 0 and isn't already dominated, then v must be in the set
                forced = true;
        return forced;
    }
    
    void dominate(Graph& G, VertIndex v){
        covered[v]++;
        if (covered[v] > 1)
            return; //The stuff below only applies if v is newly dominated
        total_covered++;
        
        UndominatedDPQ->dominate(v);
        CandidateDPQ->dominate(v);
        for(VertIndex u: G[v].neighbours())
            UndominatedDPQ->decrement(u);
    }
    void undominate(Graph& G, VertIndex v){
        covered[v]--;
        if (covered[v] > 0)
            return; //The stuff below only applies if v is no longer dominated
        total_covered--;
        
        UndominatedDPQ->undominate(v);
        CandidateDPQ->undominate(v);
        for(VertIndex u: G[v].neighbours()) //Congruent to original, but should be reversed
            UndominatedDPQ->increment(u);
            
    }
    
    template<bool check_resmod_depth>
    bool add_vertex_to_set(Graph& G, VertIndex j, int* fixed_list, int& num_fixed){
        
        bool forced = remove_candidate(G, j);
        fixed_list[num_fixed++] = j;
        
        D.add(j);

        for(VertIndex k: G[j].neighbours()){
            dominate(G, k);
        }
        FindDominatingSet<check_resmod_depth>(G);
        
        for(VertIndex k: iterate_reverse(G[j].neighbours())){
            undominate(G, k);
        }
                
        D.remove_pop(j);
        return forced;
    }
    
    
    
    
    
    bool bounds_satisfied(Graph& G){
        int n = G.n();
        
        VertIndex min_vertices_needed = UndominatedDPQ->count_minimum_to_dominate(n-total_covered);
        VertIndex min_total_size = D.get_size() + min_vertices_needed;
        
        if(GENERATE_ALL){
            if (min_total_size > total_upper_bound || n - total_fixed < min_vertices_needed)
                return false;
        }else{
            if (min_total_size >= B.get_size() || n - total_fixed < min_vertices_needed)
                return false;
        }
        return true;
    }
    
    void rank_neighbours(Graph& G, VertIndex v, VertIndex* neighbour_array, int& neighbour_count){
        //Copied almost verbatim from the C version
        //(Since it is a tough to transcribe highly optimized radix sort)
        struct NeighbourListNode{
            NeighbourListNode *next, *prev;
            VertIndex deg;
            VertIndex u;
        };
        
        int n = G.n();
        
        neighbour_count = 0;
        VertIndex v_deg = G[v].deg();
        
        NeighbourListNode all_nodes[v_deg];
        NeighbourListNode head_tail = {NULL,NULL,0,n};
        head_tail.next = head_tail.prev = &head_tail;
        
        NeighbourListNode* degrees[UndominatedDPQ->get_max_degree()+1];
        for(NeighbourListNode*& e: array_range(degrees,UndominatedDPQ->get_max_degree()+1))
            e = nullptr;			
        for(VertIndex u: G[v].neighbours()){
            if (fixed[u])
                continue;
            VertIndex uncovered_deg = UndominatedDPQ->ranked_degree(u);
            assert(uncovered_deg > 0);
            NeighbourListNode *node = &all_nodes[neighbour_count++];
            node->deg = uncovered_deg;
            node->u = u;
            NeighbourListNode *prevNode = degrees[uncovered_deg];
            if (!prevNode){
                prevNode = head_tail.prev;
                while(prevNode->deg > uncovered_deg)
                    prevNode = prevNode->prev;
            }
            node->next = prevNode->next;
            node->prev = prevNode;
            node->next->prev = node->prev->next = node;
            degrees[uncovered_deg] = node;
        }
        
        /*
        //High uncovered degree vertices first (best)
        for(node = head_tail.prev; node->deg != 0; node = node->prev)
            neighbourArray[i++] = node->u;
        //Low uncovered degree vertices first (terrible)
        for(node = head_tail.next; node->deg != 0; node = node->next)
            neighbourArray[i++] = node->u;
        */
        
        
        int i = 0;
        if (RANK_NEIGHBOURS_RULE == RANK_NEIGHBOURS_DESCENDING){
            //This is maxUCD (i.e. RANK_NEIGHBOURS_DESCENDING)
            for(NeighbourListNode *node = head_tail.prev; node->deg != 0; node = node->prev)
                neighbour_array[i++] = node->u;
        }else if (RANK_NEIGHBOURS_RULE == RANK_NEIGHBOURS_ASCENDING){
            //This is minUCD (i.e. RANK_NEIGHBOURS_ASCENDING)
            for(NeighbourListNode *node = head_tail.next; node->deg != 0; node = node->next)
                neighbour_array[i++] = node->u;
        }else{
            assert(0);
        }
        assert(i == neighbour_count);
    
    }
    
    
    template<bool check_resmod_depth>
    void FindDominatingSet(Graph& G){
        int resmod_check = report_node<check_resmod_depth>(D.get_size());
        if (resmod_check == 0)
            return;
        else if (check_resmod_depth && resmod_check == 1){
            unreport_node(D.get_size());
            FindDominatingSet<false>(G);
            return;
        }
        
        int n = G.n();
        
        if (total_covered == n){
            if (GENERATE_ALL){
                if (D.get_size() >= total_lower_bound && D.get_size() <= total_upper_bound)
                    output_proxy->process_set(*dom_inst,D);
            }else{
                if (D.get_size() >= total_lower_bound && D.get_size() < B.get_size()){
                    B = D;
                    output_proxy->process_set(*dom_inst,D);
                }
            }
            return;
        }
        
        VertIndex i = Graph::INVALID_VERTEX;
        if (CHOOSE_VERTEX_RULE == CHOOSE_VERTEX_MIN_CD){
            i = CandidateDPQ->get_min_undominated_vertex();
        } else if (CHOOSE_VERTEX_RULE == CHOOSE_VERTEX_MAX_CD){
            i = CandidateDPQ->get_max_undominated_vertex();
        }
        if (i == Graph::INVALID_VERTEX)
            return;
        //TODO remove
        assert(!covered[i] && i < n && G[i].deg() > 0);
        
        int i_deg = G[i].deg();
        
        if (!RECHECK_BOUNDS_IN_LOOP){
            if (!bounds_satisfied(G))
                return;
        }
        
        VertIndex neighbour_array[i_deg+1];
        int neighbour_count = 0;
        rank_neighbours(G,i,neighbour_array,neighbour_count);
    
        
        int fixed_list[i_deg+1]; //Standard C, but not standard C++
        int num_fixed = 0;
        
        bool end_branch = false;
        for(VertIndex j: array_range(neighbour_array,neighbour_count)){
            if (RECHECK_BOUNDS_IN_LOOP && !bounds_satisfied(G)){
                end_branch = true;
                break;
            }
            bool force_stop = add_vertex_to_set<check_resmod_depth>(G,j,fixed_list,num_fixed);
            if (FORCE_STOP_ON_TRAPPED_VERTEX && force_stop){
                end_branch = true;
                break;
            }	
        }
        
        /*
        for(int q = num_fixed - 1; q >= 0; q--){
            add_candidate(G, fixed_list[q]);
        }
        */
        
        //Duplicating an odd quirk of original implementation, we don't unstack fixed vertices
        //(so they're unstacked in the same order they were stacked)
        for(int q = 0; q < num_fixed; q++){
            add_candidate(G, fixed_list[q]);
        }
    }
    
    
    
    
    
    
    
    
    
    
};

namespace{
    typedef BBTDDSolverVariant<CHOOSE_VERTEX_MIN_CD,RANK_NEIGHBOURS_ASCENDING,false,false,false> DD_minCD_asc;
    typedef BBTDDSolverVariant<CHOOSE_VERTEX_MIN_CD,RANK_NEIGHBOURS_ASCENDING,false,false,true> DD_minCD_asc_all;
    typedef BBTDDSolverVariant<CHOOSE_VERTEX_MIN_CD,RANK_NEIGHBOURS_DESCENDING,false,false,false> DD_minCD_desc;
    typedef BBTDDSolverVariant<CHOOSE_VERTEX_MIN_CD,RANK_NEIGHBOURS_DESCENDING,false,false,true> DD_minCD_desc_all;
}
REGISTER_SOLVER( DD_minCD_asc, "DD_minCD_asc", "DD_minCD_asc");
REGISTER_SOLVER( DD_minCD_asc_all, "DD_minCD_asc_all", "DD_minCD_asc_all");

REGISTER_SOLVER( DD_minCD_desc, "DD_minCD_desc", "DD_minCD_desc");
REGISTER_SOLVER( DD_minCD_desc_all, "DD_minCD_desc_all", "DD_minCD_desc_all");


REGISTER_SOLVER_ALIAS( DD_minCD_asc, DD_basic, "DD", "DD Bounding Solver (optimization)");
REGISTER_SOLVER_ALIAS( DD_minCD_asc_all, DD_basic_all, "DD_all", "DD Bounding Solver (generation)");

