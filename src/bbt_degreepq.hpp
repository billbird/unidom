/*  bbt_degreepq.hpp

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

#ifndef BBT_DEGREEPQ_H
#define BBT_DEGREEPQ_H

#include <map>
#include <string>
#include <array>
#include "unidom_common.hpp"
#include "unidom_arrayutil.hpp"


//A "heavy" DegreePQ keeps lists of vertices with each degree, rather
//than just keep track of the counts of vertices (and the value for each vertex)
//Don't use DegreePQBase directly (use one DegreePQLight or DegreePQHeavy, defined below)
template<bool is_heavy>
class DegreePQBase{
public:

    bool is_candidate(VertIndex v){
        return !vertices[v].is_fixed;
    }
    bool is_dominated(VertIndex v){
        return vertices[v].is_dominated;
    }

    int ranked_degree(VertIndex v){
        return vertices[v].degree_node->deg;
    }
    void add_candidate(VertIndex v){	//a.k.a unfix()
        PQVertex& vertex_node = vertices[v];
        assert(vertex_node.is_fixed);
        vertex_node.degree_node->unfixed_count++;
        vertex_node.is_fixed = false;
    }
    void remove_candidate(VertIndex v){ //a.k.a fix()
        PQVertex& vertex_node = vertices[v];
        assert(vertex_node.degree_node->unfixed_count > 0 && !vertex_node.is_fixed);
        vertex_node.degree_node->unfixed_count--;
        vertex_node.is_fixed = true;
    }
    void dominate(VertIndex v){ //a.k.a cover()
        PQVertex& vertex_node = vertices[v];
        assert(!vertex_node.is_dominated);
        vertex_node.is_dominated = true;
        
        if (is_heavy){
            vertex_node.degree_node->undominated_count--;
            splice_out(vertex_node);
        }
    }
    void undominate(VertIndex v){ //a.k.a uncover()
        PQVertex& vertex_node = vertices[v];
        assert(vertex_node.is_dominated);
        vertex_node.is_dominated = false;
        
        if (is_heavy){
            vertex_node.degree_node->undominated_count++;
            splice_in(vertex_node);
        }
    }
    
    int increment(VertIndex v){
        PQVertex& vertex_node = vertices[v];
        PQNode* old_node = vertex_node.degree_node;
        int old_deg = old_node->deg;
        int new_deg = old_deg+1;
        assert( old_deg >= 0 && old_deg < n-2 );
        
        
        PQNode* new_node = &nodes[new_deg];
        
        if (new_node->count == 0){
            new_node->next = old_node->next;
            new_node->prev = old_node;
            old_node->next->prev = new_node;
            old_node->next = new_node;
        }
        vertex_node.degree_node = new_node;
        new_node->count++;
        
        if (is_heavy){
            //In heavy mode, we have to also move the vertex from the undominated list
            //of the old degree to the undominated list of the new degree.
            if (!vertex_node.is_dominated){
                splice_out(vertex_node);
                splice_in(vertex_node); //splice_in will know where to put it based on the new degree.
                old_node->undominated_count--;
                new_node->undominated_count++;
            }
        }
        
        bool is_unfixed = !vertex_node.is_fixed;
        old_node->unfixed_count -= is_unfixed; //bool to int conversion is standard C++
        new_node->unfixed_count += is_unfixed;
        
        old_node->count--;
        if (old_node->count == 0){
            new_node->prev = old_node->prev;
            new_node->prev->next = new_node;
            old_node->next = old_node->prev = nullptr;
        }
        
        return new_deg;
    }
    
    int decrement(VertIndex v){
        PQVertex& vertex_node = vertices[v];
        PQNode* old_node = vertex_node.degree_node;
        int old_deg = old_node->deg;
        int new_deg = old_deg-1;
        assert( old_deg >= 1 && old_deg < n-1 );
        
        PQNode* new_node = &nodes[new_deg];
        
        if (new_node->count == 0){
            new_node->next = old_node;
            new_node->prev = old_node->prev;
            old_node->prev->next = new_node;
            old_node->prev = new_node;
        }
        vertex_node.degree_node = new_node;
        new_node->count++;
        
        if (is_heavy){
            //In heavy mode, we have to also move the vertex from the undominated list
            //of the old degree to the undominated list of the new degree.
            if (!vertex_node.is_dominated){
                splice_out(vertex_node);
                splice_in(vertex_node); //splice_in will know where to put it based on the new degree.
                old_node->undominated_count--;
                new_node->undominated_count++;
            }
        }
        
        bool is_unfixed = !vertex_node.is_fixed;
        old_node->unfixed_count -= is_unfixed; //bool to int conversion is standard C++
        new_node->unfixed_count += is_unfixed;
        
        old_node->count--;
        if (old_node->count == 0){
            new_node->next = old_node->next;
            new_node->next->prev = new_node;
            old_node->next = old_node->prev = nullptr;
        }
        return new_deg;
    }
    
    
    
    
    
    
    int get_min_degree(){
        return head->deg;
    }
    int get_max_degree(){
        return tail->deg;
    }
    
    int sum_of_top_k_degrees(int k){
        int sum = 0;
        PQNode* node = tail;
        while(node->deg != Graph::INVALID_VERTEX){
            int count = node->unfixed_count;
            if (count >= k){
                sum += node->deg*k;
                k = 0;
                break;
            }else{
                sum += node->deg*count;
                k -= count;
                node = node->prev;
            }
            
        }
        return sum;
    }
    //Return the minimum number of vertices needed to dominate m vertices
    int count_minimum_to_dominate(int m){
        int count = 0;
        PQNode* node = tail;
        while(1){
            int deg = node->deg;
            if (deg == 0 || deg == Graph::INVALID_VERTEX)
                return unidom::MAX_VERTS+1; //Equivalent to "infinity" since in this case it's impossible to cover the necessary vertices
            int vertices_needed = (m+deg-1)/deg;
            if (vertices_needed <= node->unfixed_count){
                count += vertices_needed;
                m = 0;
                break;
            }else{
                count += node->unfixed_count;
                m -= deg*node->unfixed_count;
                node = node->prev;
            }
        }
        return count;
    }
    

    //Equivalent of DegreePQ_init from C version
    DegreePQBase(Graph& g): G(g), head(head_tail.next), tail(head_tail.prev), n(G.n()){
        
        for(int i = 0; i < n; i++){
            nodes[i].deg = i;
        }
        
        //Initialize by giving every vertex a degree of 0 and then 
        //using increment repeatedly as each vertex is brought in.
        //The C version has a comment that this is a dumb idea, but I've
        //come around to it since 2014...
        for(int i = 0; i < n; i++){
            vertices[i].v = i;
            vertices[i].degree_node = &nodes[0];
            splice_in(vertices[i]);
        }
        
        head_tail.deg = head_tail.count = Graph::INVALID_VERTEX;
        head_tail.unfixed_count = head_tail.undominated_count = Graph::INVALID_VERTEX;
        
        head = tail = &nodes[0];
        nodes[0].next = nodes[0].prev = &head_tail;
        nodes[0].count = nodes[0].unfixed_count = nodes[0].undominated_count = n;
        
        for(auto vert: G.V())
            for(int i = 0; i < vert.deg(); i++)
                increment(vert.get_index());
        
    }

protected:



    Graph& G;
    int n;

    struct PQNode;
    struct PQVertex{
        PQVertex *next, *prev;
        PQNode *degree_node;
        VertIndex v;
        bool is_fixed, is_dominated;
        
        PQVertex(){
            next = prev = nullptr;
            degree_node = nullptr;
            is_fixed = is_dominated = false;
        }
    };
    struct PQNode{
        PQNode *next, *prev;
        VertIndex deg;
        int count;
        int unfixed_count;
        int undominated_count; //a.k.a uncovered_count or available_count
        
        PQVertex head_tail;
        //undominated_head and undominated_tail are aliased to the next and prev
        //pointers of the head_tail sentinel node.
        PQVertex*& undominated_head;
        PQVertex*& undominated_tail;
        PQNode(): undominated_head(head_tail.next), undominated_tail(head_tail.prev) {
            deg = 0;
            count = 0;
            unfixed_count = 0;
            undominated_count = 0;
            undominated_head = &head_tail;
            undominated_tail = &head_tail;
            head_tail.v = Graph::INVALID_VERTEX;
        }
    };
    
    PQNode head_tail;
    PQNode*& head; //Aliased to the next pointer of head_tail
    PQNode*& tail; 
    
    std::array<PQNode, unidom::MAX_VERTS> nodes;
    std::array<PQVertex, unidom::MAX_VERTS> vertices;
    

    //splice_in and splice_out look weird because they were optimized to avoid any kind
    //of if-statements since they get called so often.
    
    //Splice the supplied vertex node into the available list of the appropriate degree node
    void splice_in(PQVertex& vertex_node){
        static PQVertex dummy_node;
        PQNode* degree_node = vertex_node.degree_node;
        vertex_node.next = &degree_node->head_tail;
        vertex_node.prev = degree_node->undominated_tail;
        vertex_node.next->prev = vertex_node.prev->next = &vertex_node;
    }
    void splice_out(PQVertex& vertex_node){
        vertex_node.next->prev = vertex_node.prev;
        vertex_node.prev->next = vertex_node.next;
        vertex_node.prev = vertex_node.next = nullptr;
    }
    
};

typedef DegreePQBase<false> DegreePQLight;
class DegreePQHeavy: public DegreePQBase<true>{
public:
    DegreePQHeavy(Graph& G): DegreePQBase<true>(G){
    }

    VertIndex get_min_undominated_vertex(){
        for(PQNode* node = head; node->deg != Graph::INVALID_VERTEX; node = node->next){
            if (node->undominated_count == 0)
                continue;
            PQVertex* vertex_node = node->undominated_head;
            VertIndex v = vertex_node->v;
            assert(v != Graph::INVALID_VERTEX);
            return v;
        }
        return Graph::INVALID_VERTEX;
    }
    VertIndex get_max_undominated_vertex(){
        for(PQNode* node = tail; node->deg != Graph::INVALID_VERTEX; node = node->prev){
            if (node->undominated_count == 0)
                continue;
            PQVertex* vertex_node = node->undominated_head;
            VertIndex v = vertex_node->v;
            assert(v != Graph::INVALID_VERTEX);
            return v;
        }
        return Graph::INVALID_VERTEX;
    }
};

#endif