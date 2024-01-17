# unidom: A modular domination solver

This program and the algorithms it contains are described in more detail in the PhD thesis _Computational Methods for Domination Problems_ from 2017, [available from Library and Archives Canada](https://library-archives.canada.ca/eng/services/services-libraries/theses/Pages/item.aspx?idNumber=1199659634)

To compile:
```
make
```

Basic usage: 
```
./unidom < some_graph.txt
```
where `some_graph.txt` contains an adjacency list representation in the format documented in the next section.

## Graph Input

The default input format is a text representation of an adjacency list following the pattern below.
```
<number of vertices>
<degree of vertex 0> <neighbour 0 of vertex 0> <neighbour 1 of vertex 0> ...
<degree of vertex 1> <neighbour 0 of vertex 1> <neighbour 1 of vertex 1> ...
...
<degree of vertex n-1> <neighbour 0 of vertex n-1> <neighbour 1 of vertex n-1> ...
```

For example, the adjacency list 
```
4
3 1 2 3
2 0 2
3 0 1 3
2 0 2
```
corresponds to the following graph:
```
O --- O
| \   |
|  \  |
|   \ |
O --- O
```

### Other input options

Besides the basic text input format, the program also contains several procedural generators for constructing graphs on the fly (without creating an adjacency list representation). Input generators can be selected with the `-I` parameter and may take extra parameters (e.g. `-I queen -n 10` to generate the 10 x 10 Queen graph). The available generators include:
 - `queen`: Generate the graph Queen(n), where the value of `n` is provided by the `-n` parameter.
 - `kneser`: Generate the graph Kneser(n,k), where `n` and `k` are specified by the `-n` and `-k` parameters.
 - `border_queen`: Generate an instance of Queen(n) (as above) with all interior vertices excluded from being part of any dominating set.

A full list of generators is available via `./unidom -h`. 

As a diagnostic, you can also output procedurally generated graphs as adjacency lists (in the format above), making `unidom` also useful as a graph generator on its own. The command below disables the domination solver and outputs only the input graph (which, in this case, is the 10 x 10 Queen graph):
```
./unidom -I queen -n 10 -S none -O graph_only
```

If you want to add extra generators and need some implementation context, look at `input_various_generators.cpp`.


## Solver Algorithms
The solver algorithm can be chosen with the `-S` parameter (e.g. `-S MDD`). If the program takes
a while with the default parameters, try the three solvers listed below
 - `fixed_order`: A very basic solver with low overhead, better for easy inputs.
 - `DD`: A solver which uses a bounding strategy based on _domination degree_. The domination degree of a vertex _v_ (with respect to some partially constructed dominating set which does not contain _v_) is the number of additional vertices that would be dominated if _v_ were added to the dominating set.
 - `MDD`: A solver which uses a bounding strategy based on _max dominator degree_. With respect to some partially constructed dominating set, the max dominator degree of an as-yet undominated vertex _v_ is the maximum domination degree of any vertex in N\[_v_\].

Each of the solver types above has several variants. Use `./unidom -h` to see a complete list.

To restrict the solver algorithm to dominating sets of particular sizes, use the following options after the solver selection parameter (e.g. '`-S MDD -l 5 -u 10`'):
 - `-u <upper bound>`: Restrict the computation to dominating sets of size at most `<upper bound>`
 - `-l <lower bound>`: Restrict the computation to dominating sets of size at least `<lower bound>`. When this parameter is provided, an optimizing solver will terminate immediately if a dominating set of size `<lower bound>` is generated.

### Exhaustive generation
There are two versions of each solver: _optimizing_ and _exhaustive generation_. 

The optimizing version solves the optimization problem of finding a minimum dominating set. Once an optimizing solver finds a dominating set of size `k`, no further sets of size `k` will be generated since the bounding mechanism will be adjusted to search for sets of size at most `k - 1`. 

The exhaustive generation version generates all dominating sets that meet the bounding criteria implied by the `-u` and `-l` options.

## Preprocessing
Various preprocessing filters are available for manipulating the graph or algorithm context before the domination solver is run. These filters can be added with the `-F` flag (and it is possible to add multiple filters by specifying `-F` more than once, with filters run in left-to-right order). A full list is available via `./unidom -h`, but the following two filters might be especially useful:
 - `force_in` (followed by a list of vertex indices): Force all of the provided vertices to be part of any generated dominating sets.
 - `force_out` (followed by a list of vertex indices): Force all of the provided vertices to be excluded from any generated dominating sets.

If you are searching for dominating sets with particular properties, the `force_in` and `force_out` filters can be helpful to limit the search space (and save running time).

For example, to search for dominating sets which must contain vertices 0 and 3 but must not contain vertices 6, 10 and 17, add `-F force_in 0 3 -F force_out 6 10 17` to the command line.

## Output options
The type of output produced can be controlled with the `-O` parameter. A complete list of output proxies is available via `./unidom -h`. The following two are particularly important.
 - `output_all`: Output every dominating set produced by the solver (one per line), followed by a line containing only `-1`. This is the default output method. Note that the word _all_ in this context does not imply that dominating sets will be generated exhaustively, just that every dominating set produced by the solver will be output; when combined with an optimizing solver, the output will usually be a cascading sequence of progressively smaller dominating sets (each one produced as the solver refines its bounds). If you want to exhaustively generate dominating sets, choose an appropriate solver (see above).
  - `output_best`: Output only the single smallest dominating set produced by the solver. With this option, no output is generated until the solver finishes.