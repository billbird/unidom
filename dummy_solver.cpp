/*  dummy_solver.cpp

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
#include "unidom_common.hpp"


using unidom::Solver;
using unidom::OutputProxy;
using unidom::DominationInstance;
using unidom::PreprocessFilter;

class DummySolver: public Solver{
public:
    void solve(DominationInstance& inst, unidom::OutputProxy& output_proxy){
        Graph& G = inst.G;
        int n = G.n();
        VertexSet V(n);
        output_proxy.initialize(inst);
        //output_proxy.process_set(inst,V);
        output_proxy.finalize(inst);
    }
};

REGISTER_SOLVER( DummySolver, "none", "Does nothing.");