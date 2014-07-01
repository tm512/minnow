minnow
------

minnow is a simple and fairly weak chess engine written in C. It currently features:

* Partial UCI compatibility.
* A 10x12 board representation with piece lists.
* An iteratively deepened alpha-beta search.
* Hash tables for pruning and move ordering.
* PV-first, MVV/LVA move ordering.
* Primitive evaluation that takes material, position (piece-square tables), piece pairs, and relative advantage into account.

To compile minnow, use the provided Makefile. On non-GNU systems, you will need to use `gmake`. 
By default, it will compile with optimizations and debug symbols turned on. To disable optimizations, use `make debug`.
To disable or remove debug symbols, use `make strip`.

minnow is free, open source software, you can use, modify, and distribute it under the terms of the MIT license.
