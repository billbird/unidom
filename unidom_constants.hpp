/*  unidom_constants.hpp

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

#ifndef UNIDOM_CONSTANTS_H
#define UNIDOM_CONSTANTS_H



namespace unidom{
    
#ifndef OVERRIDE_MAX_VERTS
    const int MAX_VERTS = 1024;
    const int MAX_DEGREE = 1024;
#else
    const int MAX_VERTS = OVERRIDE_MAX_VERTS;
    const int MAX_DEGREE = OVERRIDE_MAX_VERTS;
#endif
    
    
    extern std::ostream& log;
    
    const std::string default_input_source {"basic_input"};
    const std::string default_solver {"fixed_order"};
    const std::string default_output_proxy {"output_all"};
    
    
    
}

#endif
