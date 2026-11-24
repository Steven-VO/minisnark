# minisnark 1.1

`minisnark` generates **snarks**: cubic graphs that are not 3-edge-colourable. You give it the number of
vertices and a lower bound for the girth. Optionally it can restrict the output to a minimum cyclic edge
connectivity, to strong snarks, or to cyclically 5-connected snarks without nontrivial cyclic 5-cuts. The
output is isomorphism-free (except when using certain flags).

**Author:** Gunnar Brinkmann
**License:** GNU General Public License, version 3 or (at your option) any later version. See `LICENSE`.

## Building

```
make                 # minisnark (girth >= 5), minisnark_g4, minisnark_g6, minisnark_g7
make LARGE=1         # needed for more than 40 vertices (at most 60)
make CC=gcc          # gcc instead of clang
```

or directly, e.g. `clang -O3 -march=native -o minisnark minisnark.c`.

| binary         | compile flag | girth         |
|----------------|--------------|---------------|
| `minisnark`    | (none)       | at least 5    |
| `minisnark_g4` | `-DGIRTH4`   | at least 4    |
| `minisnark_g6` | `-DGIRTH6`   | at least 6    |
| `minisnark_g7` | `-DGIRTH7`   | at least 7    |

Every binary refuses a girth that is smaller than the one it was compiled for. For girth 4 you need
`minisnark_g4`. The versions for girth 6 and 7 are faster for those girths.

## Usage

```
minisnark v g [cx] [q] [n] [m a b] [j c d] [S] [R] [T] [N] [Kk] [Pp] [F x1 ... xk] [f x1 ... xk]
```

| option     | meaning |
|------------|---------|
| `v g`      | snarks on `v` vertices with girth at least `g` |
| `cx`       | only snarks with cyclic edge connectivity at least `x` (e.g. `c4`) |
| `q`        | less intermediate output on stderr |
| `n`        | do not write the graphs, only count them |
| `m a b`    | only part `a` of `b` disjoint parts (`0 <= a < b`) that together give all graphs |
| `j c d`    | with `m`: pre-split. Only part `c` of `d` on an earlier level, then part `a` of `b` of the rest. The `d*b` jobs together give all graphs. |
| `S`        | only strong snarks |
| `N`        | only cyclically 5-connected snarks without nontrivial cyclic 5-cuts (implies `c5`) |
| `R`        | compact run-time encoding instead of multicode (see the header of `minisnark.c`) |
| `T`        | write a tfc stream (canonical 2-factor code, format TFC1); best compressed with `xz` |
| `F x1..xk` | only snarks that have a 2-factor with cycle lengths `x1..xk` |
| `f x1..xk` | only snarks whose canonical initial 2-factor has cycle lengths `x1..xk` |
| `Kk`       | from non-2-factor edge `k` on: complete forward checking and a fail-first vertex order. There is no isomorphism rejection below that edge, so graphs can be written more than once. Meant for proofs of non-existence (e.g. girth 7). Cannot be combined with `S`, `T` or `w`. |
| `Pp`       | with `K`: when at most `p` vertices have degree 2, prune if every completion contains a bad pair of edges |

The options `b`, `w`, `L`, `M` and `Ek` switch individual speedups off or on. They are only meant for
comparisons; run `minisnark` without arguments to see them.

The graphs go to stdout and the statistics to stderr. Without `R` or `T` the output is in **multicode**.
Each graph starts with one byte for the number of vertices `n`. Then, for each vertex `1..n-1`, the list of
its larger neighbours follows, closed by a 0.

### Examples

```
./minisnark 26 5 c4 n                 # counts the 280 snarks on 26 vertices with girth >= 5, cyclically 4-edge-connected
./minisnark_g6 28 6 c4 > g6_28.mc     # the snark on 28 vertices with girth >= 6
./minisnark 26 5 c4 m 3 7 > part3.mc  # part 3 of 7
./minisnark_g7 34 7 c4 n q K7 P18     # no snarks of girth >= 7 on 34 vertices
```

For large girth-7 runs the recommended options are (compiled with `-DGIRTH7`, and with `-DLARGE` above 40 vertices):

```
minisnark_g7 v 7 c4 n q K7 P18 j c d m a b       # e.g. d = 100
```

## Version history

**1.1** (2026). The extensions and speedups were developed with the help of Claude (Anthropic).

Without the options `N`, `K` and `P`, the output is identical to that of the original minisnark, also for
each part of option `m`.

* Faster for all girths: an iterative 2-factor test with cycle tracing from both ends, forward checking
  with lifting of forbidden pairs, and a perfect-matching test on the possible partners.
* New options:
  * `N`: cyclically 5-connected snarks without nontrivial cyclic 5-cuts.
  * `T`: tfc output.
  * `j`: pre-split.
* Girth at least 7 (`-DGIRTH7` only; the code for smaller girth is unchanged):
  * Cycle tracing from both ends in `endtest()`.
  * The matching test falls back to Edmonds' blossom algorithm after 32 backtracking steps.
  * Options `K` and `P` for proofs of non-existence. The possible partners are updated incrementally.
* Bug fixes:
  * The parity shortcut for girth 4 now uses the correct index.
  * The girth is raised to the value of option `c` before the distance function is chosen.
  * A missing argument of option `m` no longer causes a crash.

