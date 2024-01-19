/*  bbt_mddstack.hpp

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

#ifndef BBT_MDDSTACK_H
#define BBT_MDDSTACK_H

#include <array>
#include <vector>
#include <algorithm>
#include "unidom_common.hpp"
#include "unidom_arrayutil.hpp"
#include "vertex_set.hpp"
#include "bbt_degreepq.hpp"


class MDDStack{
public:
    static const int INVALID_MDD = 0x7fffffff;
    int get_mdd(VertIndex v){
        return mdd_values[v];
    }
    
    int get_max_mdd(){
        return max_mdd;
    }
    
    VertIndex get_max_mdd_vertex(){
        VertIndex result = Graph::INVALID_VERTEX;
        for(VertIndex v: UndominatedSet){
            if (get_mdd(v) == max_mdd){
                result = v;
                break;
            }
        }
        assert(result != Graph::INVALID_VERTEX);
        return result;
    }
    VertIndex get_min_mdd_vertex(){
        VertIndex result = Graph::INVALID_VERTEX;
        int min_mdd = unidom::MAX_VERTS;
        for(VertIndex v: UndominatedSet){
            if (get_mdd(v) < min_mdd){
                min_mdd = get_mdd(v);
                result = v;
            }
        }
        assert(result != Graph::INVALID_VERTEX);
        return result;
    }

    void add_dominator(VertIndex v){
        //This function should be called as v is being added to the set, after all of
        //v's neighbours have been marked as covered.
        StackRow& row = new_row(v);
        
        //Clear the MDD of each of v's neighbours out of the system
        for(VertIndex u: G[v].neighbours()){
            int old_mdd = get_mdd(u);
            if ( old_mdd == INVALID_MDD )
                continue;
            int new_mdd = INVALID_MDD;
            
            row.new_entry(u,old_mdd);
            mdd_values[u] = new_mdd;
            mdd_counts[old_mdd]--;
        }
        //For each remaining undominated vertex u, completely recompute the umbrage
        //of u. Vertices affected by the addition of v to the dominating set can be
        //as far away as four steps from v, so there's really no more convenient way
        //to do this (since the uncovered set is likely to be much smaller than
        //the set of vertices up to four steps away from v)
        for(VertIndex u: UndominatedSet){
            int old_mdd = get_mdd(u);
            assert(old_mdd != INVALID_MDD);
            int new_mdd = recompute_mdd(u);
            if (old_mdd == new_mdd)
                continue;
            assert(new_mdd < old_mdd);
            row.new_entry(u,old_mdd);
            mdd_values[u] = new_mdd;
            mdd_counts[old_mdd]--;
            mdd_counts[new_mdd]++;
        }
        
        while(mdd_counts[max_mdd] == 0 && max_mdd > 0)
            max_mdd--;
        
    }
    void remove_dominator(VertIndex v){
        //This function should be called as v is being removed, before any neighbours of v have been
        //marked uncovered.
        StackRow& row = pop_row(v);
        int highest_new_mdd = 0;
        while(!row.is_empty()){
            StackEntry& entry = row.pop();
            VertIndex u = entry.vertex;
            int old_mdd = mdd_values[u];
            int new_mdd = entry.old_mdd;
            mdd_values[u] = new_mdd;
            if (old_mdd != INVALID_MDD)
                mdd_counts[old_mdd]--;
            mdd_counts[new_mdd]++;
            highest_new_mdd = std::max(highest_new_mdd,new_mdd);
        }
        if (highest_new_mdd > max_mdd)
            max_mdd = highest_new_mdd;
    }
    
    
    void exclude_dominator(VertIndex v){
        //This function is called when a vertex v (which is NOT in the dominating set) is excluded
        //from ever being in the dominating set.
        //The function should be called just after the vertex v has been marked as fixed (i.e. removed as a candidate).
        StackRow& row = new_row(v);
        
        for(VertIndex u: G[v].neighbours()){
            if (!UndominatedSet.contains(u))
                continue;
            int old_mdd = mdd_values[u];
            int new_mdd = recompute_mdd(u);
            if (new_mdd != old_mdd){
                assert(new_mdd < old_mdd);
                row.new_entry(u,old_mdd);
                mdd_values[u] = new_mdd;
                mdd_counts[old_mdd]--;
                mdd_counts[new_mdd]++;
            }
        }
        while(mdd_counts[max_mdd] == 0)
            max_mdd--;
    }
    void unexclude_dominator(VertIndex v){
        //This function is called when a vertex v (which is NOT in the dominating set) which
        //was previously excluded is allowed back into the pool of available vertices to add
        //to the dominating set.
        //The function should be called just before vertex v is unfixed.
        StackRow& row = pop_row(v);
        
        int highest_new_mdd = 0;
        while(!row.is_empty()){
            StackEntry& entry = row.pop();
            VertIndex u = entry.vertex;
            int new_mdd = entry.old_mdd;
            int old_mdd = mdd_values[u];
            assert(new_mdd > old_mdd);
            mdd_values[u] = new_mdd;
            mdd_counts[old_mdd]--;
            mdd_counts[new_mdd]++;
            highest_new_mdd = std::max(highest_new_mdd, new_mdd);
        }
        if (highest_new_mdd > max_mdd)
            max_mdd = highest_new_mdd;
    }
    
    //Count the minimum number of vertices needed to dominate all
    //remaining undominated vertices
    int min_vertices_needed(){
        if (mdd_counts[0] > 0)
            return unidom::MAX_VERTS; //If any vertices have MDD 0, it is impossible to dominate them, so the number needed is infinity.
    
        int verts_needed = 0;
        int c = 0;
        for(int mdd = 0; mdd <= max_mdd; mdd++){
            c += mdd_counts[mdd];
            while(c > 0){
                c -= mdd;
                verts_needed++;
            }
        }
        return verts_needed;
        
    }

    MDDStack(Graph& g, 
             std::vector<VertexSet>& CandidateNeighbours, 
             VertexSet& undominated_set,
             DegreePQLight& undominated_dpq):
        G(g), CandidateNeighboursArray(&CandidateNeighbours), UndominatedSet(undominated_set), UndominatedDPQ(&undominated_dpq) {
        n = G.n();
        
        stack_size = 0;
        for (auto& row: stack){
            row.size = 0;
            row.dominator = Graph::INVALID_VERTEX;
            for(auto& entry: row.entries){
                entry.vertex = Graph::INVALID_VERTEX;
                entry.old_mdd = -1;
            }
        }
        
        mdd_values.fill(-1);
        mdd_counts.fill(0);
        
        for(VertIndex v: UndominatedSet){
            int v_mdd = recompute_mdd(v);
            mdd_values[v] = v_mdd;
            mdd_counts[v_mdd]++;
        }
        
        for (int i = 0; i < n; i++)
            if (mdd_counts[i] > 0)
                max_mdd = i;
        
        
    }
    
    
protected:



    Graph& G;
    std::vector<VertexSet>* CandidateNeighboursArray;
    VertexSet& candidate_neighbour_set(VertIndex v){
        return (*CandidateNeighboursArray)[v];
    }
    VertexSet& UndominatedSet;
    DegreePQLight* UndominatedDPQ;
    
    
    int n;
    
    int stack_size;
    struct StackEntry{
        VertIndex vertex;
        int old_mdd;
    };
    struct StackRow{
    public:
        std::array<StackEntry,unidom::MAX_VERTS> entries;
        int size;
        VertIndex dominator;
        
        bool is_empty(){
            return size == 0;
        }
        StackEntry& new_entry(){
            return entries[size++];
        }
        StackEntry& new_entry(VertIndex vertex, int old_mdd){
            StackEntry& result = entries[size++];
            result.vertex = vertex;
            result.old_mdd = old_mdd;
            return result;
        }
        StackEntry& pop(){
            return entries[--size];
        }
    };
    std::array<StackRow,unidom::MAX_VERTS> stack;
    std::array<int, unidom::MAX_VERTS> mdd_values;
    std::array<int, unidom::MAX_VERTS> mdd_counts; //mdd_counts[i] == number of vertices with mdd equal to i
    
    int max_mdd;
    
    
    StackRow& new_row(VertIndex dominator){
        StackRow& result = stack[stack_size++];
        result.size = 0;
        result.dominator = dominator;
        return result;
    }
    StackRow& pop_row(VertIndex dominator){
        StackRow& result = stack[--stack_size];
        assert(dominator == result.dominator);
        return result;
    }
    
    int recompute_mdd(VertIndex v){
        int new_mdd = 0;
        for(VertIndex u: candidate_neighbour_set(v)){
            VertIndex u_DD = UndominatedDPQ->ranked_degree(u);
            new_mdd = std::max( new_mdd, u_DD );
        }
        return new_mdd;
    }
    
};

#endif