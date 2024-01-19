/*  bbt_mdd_variants.cpp

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
#include <array>
#include <vector>
#include <algorithm>
#include <cassert>
#include "unidom_common.hpp"
#include "unidom_arrayutil.hpp"
#include "bbt_framework.hpp"
#include "bbt_degreepq.hpp"
#include "bbt_mddstack.hpp"
#include "graph_util.hpp"

using std::array;
using std::vector;
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
    const int CHOOSE_VERTEX_MIN_MDD = 0;
    const int CHOOSE_VERTEX_MAX_MDD = 1;
    const int CHOOSE_VERTEX_MIN_CD = 2;
    const int CHOOSE_VERTEX_MAX_CD = 3;
    
    const int RANK_NEIGHBOURS_ASCENDING = 0;
    const int RANK_NEIGHBOURS_DESCENDING = 1;	
}


template<unsigned int CHOOSE_VERTEX_RULE, unsigned int RANK_NEIGHBOURS_RULE, bool FORCE_STOP_ON_TRAPPED_VERTEX, bool RECHECK_BOUNDS_IN_LOOP, bool GENERATE_ALL>
class BBTMDDSolverVariant: public BBTFrameworkSolver{
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
        
        
        UndominatedSet.reset_full(n);
        CandidateNeighbours.clear();
        CandidateNeighbours.resize(n); //The newly created sets will be reset_empty by default
        for(VertIndex v = 0; v < n; v++){
            VertexSet& v_neighbours = CandidateNeighbours[v];
            for(VertIndex u: G[v].neighbours())
                v_neighbours.add(u);
        }
        
        //DegreePQs initialize with full degree (i.e. ranked_degree(v) = deg(v))
        DegreePQLight UD_DPQ(G);
        
        UndominatedDPQ = &UD_DPQ;
        
        
        //The MDD_Stack is so huge, it will break the stack size limit
        mdd_stack = new MDDStack(G,CandidateNeighbours,UndominatedSet,UD_DPQ);
        
        //Add all of the "force_in" vertices to the dominating set
        
        
        for(VertIndex v: inst.force_in){
            remove_candidate(G,v);
            D.add(v);
            for(VertIndex u: G[v].neighbours()){
                dominate(G,u);
            }
            mdd_stack->add_dominator(v);
        }	
        
        //Set all of the "force_out" vertices to be forbidden
        for(VertIndex v: inst.force_out){
            remove_candidate(G,v);
            mdd_stack->exclude_dominator(v);
        }
        
        
        
        
        
        reset_depth_log();
        
        output_proxy.initialize(inst);
        FindDominatingSet<true>(G);
        output_proxy.finalize(inst);
        
        print_depth_log();
        
        UndominatedDPQ = nullptr;
        delete mdd_stack;
        mdd_stack = nullptr;
    }
    
private:
    DominationInstance* dom_inst;
    OutputProxy* output_proxy;
    
    VertexSet D; //Current working set
    VertexSet B; //Best set found so far
    
    DegreePQLight* UndominatedDPQ; //Tracks the domination degree of each vertex
    vector<VertexSet> CandidateNeighbours;
    VertexSet UndominatedSet;
    MDDStack* mdd_stack;
    
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
        
        for(VertIndex u: G[v].neighbours()) //Congruent to original, but should be reversed
            CandidateNeighbours[u].add(v);		
    }
    bool remove_candidate(Graph& G, VertIndex v){ //Returns true if v must be in the dominating set.
        assert(!fixed[v]);
        fixed[v] = 1;
        total_fixed++;
        UndominatedDPQ->remove_candidate(v);
        
        bool forced = false;
        for(VertIndex u: G[v].neighbours()){
            CandidateNeighbours[u].remove(v);
            if (CandidateNeighbours[u].get_size() == 0 && !covered[u]) //If u ends up with candidate degree 0 and isn't already dominated, then v must be in the set
                forced = true;
        }
        return forced;
    }
    
    void dominate(Graph& G, VertIndex v){
        covered[v]++;
        if (covered[v] > 1)
            return; //The stuff below only applies if v is newly dominated
        total_covered++;
        
        UndominatedDPQ->dominate(v);
        UndominatedSet.remove(v);
        for(VertIndex u: G[v].neighbours())
            UndominatedDPQ->decrement(u);
    }
    void undominate(Graph& G, VertIndex v){
        covered[v]--;
        if (covered[v] > 0)
            return; //The stuff below only applies if v is no longer dominated
        total_covered--;
        
        UndominatedDPQ->undominate(v);
        UndominatedSet.add(v);
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
        
        mdd_stack->add_dominator(j);
        //FindDominatingSets returns 0 if it bounds out fatally, in which case
        //we have to force stop (since the bound will continue to be violated
        //until we unwind).
        //FindDominatingSets returns -1 if it bounds out non-fatally (i.e. if the bound is not
        //necessarily going to be violated until we unwind).
        if (FindDominatingSet<check_resmod_depth>(G) == 0){
            forced = true;
        }
        
        mdd_stack->remove_dominator(j);
        
        for(VertIndex k: iterate_reverse(G[j].neighbours())){
            undominate(G, k);
        }
                
        D.remove_pop(j);
        
        mdd_stack->exclude_dominator(j);
        
        return forced;
    }
    
    
    
    
    
    void rank_neighbours(Graph& G, VertIndex v, VertIndex* neighbour_array, int& neighbour_count){
        //Copied almost verbatim from the C version
        //This version is from the UBBT code (it is distinct from the version in the CBBT code
        //used in the DD solver).
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
        for(VertIndex u: CandidateNeighbours[v]){
            VertIndex uncovered_deg = UndominatedDPQ->ranked_degree(u);
            //assert(uncovered_deg > 0);
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
        
        //TODO add template parameter to decide between minUCD and maxUCD
        
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
    
    

    
    int evaluate_bounds(Graph& G){
        int n = G.n();
        
        VertIndex min_vertices_needed = mdd_stack->min_vertices_needed();
        if (min_vertices_needed >= unidom::MAX_VERTS)
            return 0;
        VertIndex min_total_size = D.get_size() + min_vertices_needed;
        
        if(GENERATE_ALL){
            if (n - total_fixed + 1 < min_vertices_needed)
                return 0; //Fatal
            if (n - total_fixed + 1 == min_vertices_needed)
                return -1;//Potentially not fatal (might be caused by the current vertex)
            if (min_total_size > total_upper_bound)
                return -1;
        }else{
            if (n - total_fixed + 1 < min_vertices_needed)
                return 0; //Fatal
            if (n - total_fixed + 1 == min_vertices_needed)
                return -1;//Potentially not fatal (might be caused by the current vertex)
            if (min_total_size >= B.get_size())
                return -1;
        }
        return 1;
    }
    
    VertIndex choose_next_vertex(Graph& G){
        if (CHOOSE_VERTEX_RULE == CHOOSE_VERTEX_MIN_MDD){
            //Find a vertex with maximum MDD
            VertIndex min_mdd_vertex = mdd_stack->get_min_mdd_vertex();
            return min_mdd_vertex;
        } else if (CHOOSE_VERTEX_RULE == CHOOSE_VERTEX_MAX_MDD){
            //Find a vertex with maximum MDD
            VertIndex max_mdd_vertex = mdd_stack->get_max_mdd_vertex();
            return max_mdd_vertex;
        } else if (CHOOSE_VERTEX_RULE == CHOOSE_VERTEX_MIN_CD){
            VertIndex min_cd_vertex = Graph::INVALID_VERTEX;
            int min_cd = unidom::MAX_VERTS;
            for(VertIndex v: UndominatedSet){
                if (CandidateNeighbours[v].get_size() < min_cd){
                    min_cd = CandidateNeighbours[v].get_size();
                    min_cd_vertex = v;
                }
            }
            assert(min_cd_vertex != Graph::INVALID_VERTEX);
            return min_cd_vertex;
        } else if (CHOOSE_VERTEX_RULE == CHOOSE_VERTEX_MAX_CD){
            VertIndex max_cd_vertex = Graph::INVALID_VERTEX;
            int max_cd = 0;
            for(VertIndex v: UndominatedSet){
                if (CandidateNeighbours[v].get_size() > max_cd){
                    max_cd = CandidateNeighbours[v].get_size();
                    max_cd_vertex = v;
                }
            }
            assert(max_cd_vertex != Graph::INVALID_VERTEX);
            return max_cd_vertex;
        } else
            assert(0);
        return Graph::INVALID_VERTEX;
    }
    
    
    template<bool check_resmod_depth>
    int FindDominatingSet(Graph& G){
        int resmod_check = report_node<check_resmod_depth>(D.get_size());
        if (resmod_check == 0)
            return 1;
        else if (check_resmod_depth && resmod_check == 1){
            unreport_node(D.get_size());
            return FindDominatingSet<false>(G);
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
            return 1;
        }
        
        int bound_result = evaluate_bounds(G);
        if (bound_result != 1)
            return bound_result;
        
        VertIndex i = choose_next_vertex(G);
        if (i == Graph::INVALID_VERTEX)
            assert(0);
        //TODO remove
        //assert(!covered[i] && i < n && G[i].deg() > 0);
        
        int i_deg = G[i].deg();
        
        VertIndex neighbour_array[i_deg+1];
        int neighbour_count = 0;
        rank_neighbours(G,i,neighbour_array,neighbour_count);
    
        
        int fixed_list[i_deg+1]; //Standard C, but not standard C++
        int num_fixed = 0;
        
        for(VertIndex j: array_range(neighbour_array,neighbour_count)){
            bool force_stop = add_vertex_to_set<check_resmod_depth>(G,j,fixed_list,num_fixed);
            if (FORCE_STOP_ON_TRAPPED_VERTEX && force_stop){
                break;
            }	
            if (RECHECK_BOUNDS_IN_LOOP && evaluate_bounds(G) != 1)
                break;
        }
        
        
        for(int q = num_fixed - 1; q >= 0; q--){
            mdd_stack->unexclude_dominator(fixed_list[q]);
            add_candidate(G, fixed_list[q]);
        }
        return 1;
    }
    
    
    
    
    
    
    
    
    
    
};

namespace{
    typedef BBTMDDSolverVariant<CHOOSE_VERTEX_MIN_CD,RANK_NEIGHBOURS_DESCENDING,false,true,false> MDD_minCD_desc;
    typedef BBTMDDSolverVariant<CHOOSE_VERTEX_MIN_CD,RANK_NEIGHBOURS_DESCENDING,false,true,true> MDD_minCD_desc_all;
    typedef BBTMDDSolverVariant<CHOOSE_VERTEX_MIN_CD,RANK_NEIGHBOURS_ASCENDING,false,true,false> MDD_minCD_asc;
    typedef BBTMDDSolverVariant<CHOOSE_VERTEX_MIN_CD,RANK_NEIGHBOURS_ASCENDING,false,true,true> MDD_minCD_asc_all;
    typedef BBTMDDSolverVariant<CHOOSE_VERTEX_MIN_MDD,RANK_NEIGHBOURS_DESCENDING,false,true,false> MDD_minMDD_desc;
    typedef BBTMDDSolverVariant<CHOOSE_VERTEX_MIN_MDD,RANK_NEIGHBOURS_DESCENDING,false,true,true> MDD_minMDD_desc_all;
    typedef BBTMDDSolverVariant<CHOOSE_VERTEX_MAX_MDD,RANK_NEIGHBOURS_DESCENDING,false,true,false> MDD_maxMDD_desc;
    typedef BBTMDDSolverVariant<CHOOSE_VERTEX_MAX_MDD,RANK_NEIGHBOURS_DESCENDING,false,true,true> MDD_maxMDD_desc_all;
}

REGISTER_SOLVER( MDD_minCD_desc, "MDD_minCD_desc", "MDD_minCD_desc");
REGISTER_SOLVER( MDD_minCD_desc_all, "MDD_minCD_desc_all", "MDD_minCD_desc_all");
REGISTER_SOLVER( MDD_minCD_asc, "MDD_minCD_asc", "MDD_minCD_asc");
REGISTER_SOLVER( MDD_minCD_asc_all, "MDD_minCD_asc_all", "MDD_minCD_asc_all");
REGISTER_SOLVER( MDD_minMDD_desc, "MDD_minMDD_desc", "MDD_minMDD_desc");
REGISTER_SOLVER( MDD_minMDD_desc_all, "MDD_minMDD_desc_all", "MDD_minMDD_desc_all");
REGISTER_SOLVER( MDD_maxMDD_desc, "MDD_maxMDD_desc", "MDD_maxMDD_desc");
REGISTER_SOLVER( MDD_maxMDD_desc_all, "MDD_maxMDD_desc_all", "MDD_maxMDD_desc_all");



REGISTER_SOLVER_ALIAS( MDD_minCD_desc, MDD_basic, "MDD", "MDD Bounding Solver (optimization)");
REGISTER_SOLVER_ALIAS( MDD_minCD_desc_all, MDD_basic_all, "MDD_all", "MDD Bounding Solver (generation)");