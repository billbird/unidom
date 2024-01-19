/*  bbt_fixed_order.cpp

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
#include <algorithm>
#include <cassert>
#include "unidom_common.hpp"
#include "bbt_framework.hpp"
#include "graph_util.hpp"

using std::array;
using std::min;
using std::max;

using unidom::Solver;
using unidom::OutputProxy;
using unidom::DominationInstance;
using unidom::MAX_VERTS;

template<bool GENERATE_ALL>
class BBTFixedOrderSolver: public BBTFrameworkSolver{
public:
    void solve(DominationInstance& inst, unidom::OutputProxy& output_proxy){
        /*
        Graph& G = inst.G;
        int n = G.n();
        VertexSet V(n);
        output_proxy.process_set(inst,V);
        output_proxy.finalize(inst);
        */
        dom_inst = &inst;
        Graph& G = inst.G;
        this->output_proxy = &output_proxy;
        
        
        add_loops(G);
        sort_neighbours_descending(G);
        
        
        int n = inst.G.n();
        D.reset();
        B.reset_full(n);
        
        if (!GENERATE_ALL && total_upper_bound < n)
            B.reset_full(total_upper_bound+1);
        
        compute_max_deg(inst.G);
        
        covered.fill(0);
        fixed.fill(0);
        
        total_covered = 0;
        total_fixed = 0;
        
        //Add all of the "force_in" vertices to the dominating set
        for(VertIndex v: inst.force_in){
            D.add(v);
            for(VertIndex u: G[v].neighbours()){
                if (covered[u] == 0)
                    total_covered++;
                covered[u]++;
            }
        }
        
        //Set all of the "force_out" vertices to be forbidden
        for(VertIndex v: inst.force_out){
            fixed[v] = 1;
            total_fixed++;
        }
        
        reset_depth_log();
        
        output_proxy.initialize(inst);
        FindDominatingSet<true>(G,0);
        output_proxy.finalize(inst);
        
        print_depth_log();
        
        
    }
    
private:
    DominationInstance* dom_inst;
    OutputProxy* output_proxy;
    
    VertexSet D; //Current working set
    VertexSet B; //Best set found so far
    int max_deg;
    
    array<int,MAX_VERTS> covered, fixed;
    int total_covered, total_fixed;
    
    void compute_max_deg(Graph& G){
        int n = G.n();
        
        for (int i = 0; i < n; i++){
            max_deg = max(max_deg,G[i].deg());
        }
    }
    
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
    
    
    
    
    
    
    
    template<bool check_resmod_depth>
    void add_vertex_to_set(Graph& G, int i, int j, int* fixed_list, int& num_fixed){
        
        fixed[j] = 1;
        fixed_list[num_fixed++] = j;
        total_fixed++;
        D.add(j);
        
        for(VertIndex k: G[j].neighbours()){
            if (covered[k] == 0)
                total_covered++;
            covered[k]++;
        }
        assert(covered[i]);
        FindDominatingSet<check_resmod_depth>(G,i+1);
        
        //This is congruent to the original, but both it and this one should really do this
        //in reverse order of the loop above...
        for(VertIndex k: G[j].neighbours()){
            covered[k]--;
            if (covered[k] == 0)
                total_covered--;
        }
                
        D.remove_pop(j);
    }
    
    
    template<bool check_resmod_depth>
    void FindDominatingSet(Graph& G, int i){
        int resmod_check = report_node<check_resmod_depth>(D.get_size());
        if (resmod_check == 0)
            return;
        else if (check_resmod_depth && resmod_check == 1){
            unreport_node(D.get_size());
            FindDominatingSet<false>(G,i);
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
        
        while(covered[i])
            i++;
        
        if(i >= n)
            throw unidom::ConfigurableError("Graph is not consistent");
        
        VertIndex min_vertices_needed = (n-total_covered + max_deg)/(max_deg+1);
        VertIndex min_total_size = D.get_size() + min_vertices_needed;
        
        if(GENERATE_ALL){
            if (min_total_size > total_upper_bound || n - total_fixed < min_vertices_needed)
                return;
        }else{
            if (min_total_size >= B.get_size() || n - total_fixed < min_vertices_needed)
                return;
        }
        
        int i_deg = G[i].deg();
        int fixed_list[i_deg+1]; //Standard C, but not standard C++
        int num_fixed = 0;
        
        int neighbour_array[i_deg+1];
        int neighbour_count = 0;
        
        //Populate the neighbour array with i, the uncovered neighbours of i, and the covered neighbours of i
        if (!fixed[i])
            neighbour_array[neighbour_count++] = i;
        for(VertIndex j: G[i].neighbours())
            if (!fixed[j] && !covered[j] && j != i)
                neighbour_array[neighbour_count++] = j;
        for(VertIndex j: G[i].neighbours())
            if (!fixed[j] && covered[j])
                neighbour_array[neighbour_count++] = j;

        for(int q = 0; q < neighbour_count; q++){
            VertIndex j = neighbour_array[q];
            add_vertex_to_set<check_resmod_depth>(G,i,j,fixed_list,num_fixed);
        }
        
        for(int q = num_fixed - 1; q >= 0; q--){
            fixed[fixed_list[q]] = 0;
            total_fixed--;
        }
        
    }
    
    
    
    
    
    
    
    
    
    
};

REGISTER_SOLVER( BBTFixedOrderSolver<0>, "fixed_order", "Fixed order solver (optimizing version) based on backtracking framework");
REGISTER_SOLVER( BBTFixedOrderSolver<1>, "fixed_order_all", "Fixed order solver (exhaustive generation version) based on backtracking framework");