/* minisnark version 1.1 -- generation of snarks: cubic graphs that are not 3-edge-colourable, with a given girth (at least 4)
   and optionally a given cyclic edge connectivity.

   Author: Gunnar Brinkmann
   Copyright (C) 2025 - 2026 Gunnar Brinkmann

   This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program (file LICENSE). If not, see
   <https://www.gnu.org/licenses/>.

   Version 1.1 contains extensions and speedups that were developed in 2026 with the help of Claude (Anthropic).
   Changes with respect to the original minisnark:
   - option N: only cyclically 5-connected snarks without nontrivial cyclic 5-cuts, with early rejection of partial graphs
     that will always have such a cut (cheap tests in construct(), two 5-cycles sharing a path of length 2 through the new
     edge, optional general search with option E) and backjumping after a rejected complete graph;
   - faster 2-factor test: the cycles in endtest() are traced from both ends at the same time (girth 4 and 5 versions),
     weitermatchen() is iterative (same search in the same order);
   - givenumbers() numbers the cycles of the initial 2-factor arithmetically instead of tracing them;
   - forward checking in construct() (not for option S): if G+{v,w} has an even or better 2-factor through {v,w}, every
     completion containing {v,w} has it too. Such pairs are remembered for the whole subtree (they are not tried again as
     edges), and a partial graph is rejected when a vertex of degree 2 has no possible partner left. For each vertex a
     partner that passed the test is kept as witness and tested first in the children -- only for 2-factors that also
     contain the new edge. Option w switches it off.
     Partial graphs are only rejected this way at or below the split level of option m, so that the parts are the same.
     Lifting (idea of G. Brinkmann): a pair rejected by a 2-factor whose inserted edges were all inserted on level f or
     earlier is forbidden from level f on, so also in the siblings of the nodes between f and the current level. Option L
     switches it off.
   - matching test (idea of G. Brinkmann): after forward checking, a partial graph is rejected if the vertices of degree 2
     have no perfect matching using only possible partners. Option M switches it off.
   - work_bitgraph is kept equal to bitgraph (no copy for each 2-factor test); weitermatchen() stores the old rows of the
     matched vertices and restores them directly; vertices are sorted into buckets without branches.
   - option T: the graphs are written as a tfc stream (see below).
   - option j c d (with m a b): pre-split on the edge level two above the split level of m. Only every d-th node there is
     expanded (part c), and the parts of m are formed from what is left. The d*b jobs together form the whole set (different
     parts than with m alone). Each job only traverses about 1/d of the two levels above the split level, which is most of the
     work that every part of m repeats: for girth 7 at 46 vertices 70 seconds per job without j, 1.4 seconds with j c 100.
   Without the options N, K and P the output is identical to the output of minisnark (also for each part of option m) --
   with the corrected minisnark.c of October 2026 (index cyclelengths[incycle[nb]] in the parity shortcut for girth 4, girth
   raised to the value of option c before the distance function is chosen); both fixes are also contained here.

   Optimizations for girth at least 7 (compiled with -DGIRTH7; the code for smaller girth is not affected):
   - endtest(): the rest of a cycle is traced from both ends at the same time, as for girth 4 and 5.
   - matching test: a backtracking search is used; if it needs more than 32 steps (it can need thousands when there is no
     perfect matching), Edmonds' blossom algorithm decides.
   - the possible partners at distance at least girth-1 are updated incrementally in construct_mrv() (option K).
   - option K<k> (for proofs of non-existence, e.g. K7 for girth 7 -- it can be used for every girth, but was only tuned
     for girth 7): from the non-2-factor edge number k on, construct_mrv() is used instead of construct():
     * complete forward checking: all possible partners of all vertices of degree 2 are tested, so all forbidden pairs are
       known exactly. A pair that was accepted on the level before is only tested again with the 2-factors that also contain
       the new edge -- and only if each end of both edges has a neighbour of degree 3 in the initial 2-factor (such a
       2-factor drops one 2-factor edge at each end, and the dropped edge must lead to a vertex of degree 3);
     * the vertex b that gets its third edge next is the one with the smallest 4*(possible partners)+2*(neighbours in the
       initial 2-factor that have degree 2)-(possible partners whose two neighbours in the initial 2-factor have degree 2),
       the largest one if there are several ("fail first"); it is chosen after the matching test and option P;
     * the isomorphism rejection and the early tests of the connectivity depend on the order of the vertices, so they are
       not used there -- graphs can be output several times (no effect when there are no snarks, e.g. girth 7).
     All rejections by 2-factors, distances, forward checking and the matching test are kept, so no snark is lost.
   - option P<p> (with K): when at most p vertices have degree 2, construct_mrv() searches for a completion that keeps the
     girth, avoids all forbidden pairs and contains no bad pair of edges -- two new edges e,f such that the current graph
     plus e and f has a contradicting 2-factor through e and f (twoedge_test(); pairs of edges are only tested when they meet
     in the search). If there is no such completion, every completion has a colouring or a better 2-factor that uses at most
     two of the new edges, so the node is pruned.
   Girth 7, c4, time of the work below edge level 7 (one core of an i9-9900 or i7-8700K; total = 2.5*10^6 parts, measured on
   samples of parts): option K7 alone versus minisnark 2.2 (32 vertices) .. 5.0 (42); complete forward checking 0.62 of K7
   alone at 44; P18 0.82 of that at 42-44; faster tests 0.69 of that at 44; matching test with Edmonds and later choice of b
   0.87-0.89 of that at 46. Estimated totals with K7 P18: 42 vertices ~25, 44 ~460, 46 ~6000 CPU-days.
   At 46 vertices 2/3 of the time is spent at nodes with 18 vertices of degree 2, nearly all of which are pruned there
   (8% forward checking, 43% matching test, 49% option P). P16 and P20 (which prunes 99.4% of the nodes with 20 vertices of
   degree 2, but each test costs about 4 times as much) and K6 were slower.
   Recommended for girth 7 (compiled with -DGIRTH7, and with -DLARGE for more than 40 vertices):
     minisnark v 7 c4 n q K7 P18 j c d m a b      (e.g. d=100)

   Compile: clang -march=native -O3 -o minisnark minisnark.c      (or gcc instead of clang; see also the Makefile)
   By default the program is compiled for girth at least 5 (GIRTH5 is defined automatically). For girth 4 compile with
   -DGIRTH4 -- the code that only has an effect for girth 4 (the girth 4 version of the cycle tracing, the distance function
   for girth 4 and the parity shortcut in construct()) is only compiled then.
   add -DGIRTHX if the girth is at least X\in{6,7}.  This gives a very small speedup, but well...
   If used for more than 40 vertices, also compile it with -DLARGE

   The snarks are written to stdout in multicode. This is a binary code where the first unsigned char is the number of vertices. The vertices are always numberered
   1,...,|V|. After this first unsigned char the list of LARGER neigbours for each vertex 1,...,|V|-1 follows. Each list is closed by 0. As the last vertex can have no larger neighbours
   anyway, for this vertex no list end no end-of-list character are necessary.

   So the code for K_4 would be (as unsigned chars) 4 2 3 4 0 3 4 0 4 0 and for K_{3,3} e.g. 6 2 3 4 0 5 6 0 5 6 0 5 6 0 0

   To see a list of options, just run minisnark with option h.

   If compiled with -DGETSPLITS=x with x a number it only runs to the split level x and counts the number of branches starting there.
*/

/* option R changes the default multicode to some runtime encoding: each item is one byte -- interpreted as an unsigned char number. The first one is the number of bytes that
   must be taken from the previous code. So the first code always starts with a 0. After the first byte the lengths of the cycles in the 2-factor follow ended with a 0. E.g. 5 7 4 0
   would mean that there is a cycle 1-2-3-4-5, a cycle 6-7-8-9-10-11-12, and a cycle 13-14-15-16. This also encodes the number of vertices: 16. Then |V|/2 edges are still missing.
   These are the next |V|/2 bytes. For each vertex v in 1...|V| the neighbour is listed if and only if it has a larger number than v. So if there would be edges {1,3}, {2,7}, {4,9}, etc.
   the rest of the code would start 3 7 9 ... not listing an entry for 3. */

/* option T (added by Claude): the graphs are written as a tfc stream (format TFC1, see FORMAT.md of the separate tfc tools;
   decoder tfcdec.h, tool tfc). The code of a graph is n, the cycle lengths of the initial 2-factor and the partners of the labels that are matched
   to a larger label -- with the canonical labelling of minisnark this is the canonical code of tfc. The entries are stored as
   "vrank" (the index among the possible partners that are unmatched, larger and at distance at least girth-1), entries with
   only one possible partner are omitted, and each record is front coded against the previous one (flags 11, as written by
   tfc). Each initial 2-factor gives one batch; inside a batch the codes are increasing, as minisnark generates the graphs in
   this order. The output of each part of option m is a complete stream; streams can be concatenated. It is best compressed
   with xz. */


/* Also small cyclic cuts can be detected early, but it did not help at all. Either it cost even more trying to detect them (running explicit tests) or it simply didn't help -- when
   just performing the tests at the end and storing the edge to which one has to backtrack to destroy the detected cut. Though fewer graphs were tested at the end, the computation
   time was not shorter. */
/* From constructed 2-factors conclusions can be drawn which edges must not be inserted as otherwise one gets a 2-factor contradicting canonicity or even an even 2-factor.
   Also here the cost was higher than the benefit. Only slightly higher, but more complicated and slightly slower is not a good thing...*/
/* The result that the smallest snark with oddness 4 has 44 vertices would only give a tiny speedup, so it is not used  as otherwise the results would not be independent
   tests of snarkhunter. */
/* Claude, October 2026: forward checking does not use 2-factors with the same cycle lengths as the initial 2-factor (better
   labellings). This was measured: a pair {v,w} is forbidden if G+{v,w} already has a smaller representation that is decided
   by the edges present (positions of vertices of degree 2 count as undecided, so the rejection holds in the whole subtree).
   Output identical, but the number of nodes only decreases by 10.1%, 9.8%, 8.2%, 6.5%, 6.8% for 22, 24, 26, 28, 30 vertices
   (c4, girth 5), while the straightforward test made the program 5-7 times slower -- even with only the 2-factors stored on
   earlier levels (2/3 of the gain). So it is not used.
   Side remark: weitermatchen() also returns 1 when the graph has a closed component in which all vertices have degree 3 (only
   vertices with three possible partners left). The tests of the connectivity in construct() only check unions of the first
   cycles, so e.g. a closed component formed by cycles 0 and 2 is only detected by the 2-factor tests. This is correct, but the
   restricted test of a witness does not see it. It occurs in only about 1 of 2 million nodes, so no extra test is used. */

// author: Gunnar Brinkmann

#define MINISNARK_VERSION "1.1"

#include<stdlib.h>
#include<stdio.h>
#include<limits.h>
#include <unistd.h>
#include <sys/times.h>
#include<memory.h>
#include<string.h>
#include<ctype.h>
//#include<strings.h>

#define FFSLOK
#define KNOTEN 64 // larger than necessary, but faster than the really necessary value
#define LONGTYPE unsigned long int
//#define NUMBERTYPE char
//#define NUMBERTYPE int
#define NUMBERTYPE short
// tests showed that going to char is slower -- short is a compromise: fewer space than int, but more than char, but still fast

#define LAST_STORELEVEL ((nv>>1)-1) // for a smaller value, the edges inserted after this level must be stored and added before calling "weitermatchen()"

#define bit(i) (((LONGTYPE)1)<<(i))
#define DELBIT(a,i) ((a)&=(~bit(i)))
#define SETBIT(a,i) ((a)|=(bit(i)))
#define IS_SET(a,i) ((a)&(bit(i)))
#define NOT_SET(a,i) (!((a)&(bit(i))))
#define EMPTYSET ((LONGTYPE)0)
#define ALLBITS1 (~((LONGTYPE)0))
#define COMPL(x) (allvertices & (~(x)))
#define WITHOUTFIRST(x) ((x)&((x)-1))
#ifdef LARGE
LONGTYPE bit2value[KNOTEN];
// As even and odd numbers are treated separately, the same shifts can be used for an even and an odd number
#define bit2(i) (bit2value[i])
#else
#define bit2(i) (((LONGTYPE)1)<<(((i)-4)<<1))
// maximum length of cycle in 2-factor: 35 -- then the shift is (35-4)*2=62, so maximum number of vertices 40
// as even and odd numbers are treated separately, one has 4bits for counting, so OK up to 40
#endif
#define SETBIT2(a,i) ((a)+=(bit2(i)))
#define DELBIT2(a,i) ((a)-=(bit2(i)))
// As even and odd numbers are treated separately, the same shifts can be used for an even and an odd number


#define LESS_THAN_2BIT(a) (((a)&((a)-(LONGTYPE)1))==((LONGTYPE)0))
#define ATLEAST2BIT(a) ((a)&((a)-(LONGTYPE)1))
//#define ATLEAST2BIT(a) (POPC(a)>=2) seems to be slower
#define MORETHAN2BIT(a) ATLEAST2BIT((a)&((a)-(LONGTYPE)1)) // (POPC(a)>2)
#define ATMOST2BIT(a) (!MORETHAN2BIT(a))
#define ATMOST1BIT(a) (((a)&((a)-(LONGTYPE)1))==EMPTYSET)


//#define FFSL(x)  (__builtin_ffsl(x))
#define FFSL(x)  (ffsl(x))
#define SMALLEST(x) (FFSL(x)-1)
#define SECONDSMALLEST(x) (SMALLEST(WITHOUTFIRST(x)))
#define POPC(x) (__builtin_popcountl(x))
#define CTZL(x)  (__builtin_ctzl(x))
#define SMALLEST_CTZ(x) (__builtin_ctzl(x)) // for these one must be sure that x is not 0
#define LARGEST_CLZ(x) (63-__builtin_clzl(x)) // for these one must be sure that x is not 0
#define SECONDSMALLEST_CTZ(x) (__builtin_ctzl(WITHOUTFIRST(x)))


#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

#if !defined(GIRTH4) && !defined(GIRTH5) && !defined(GIRTH6) && !defined(GIRTH7)
#define GIRTH5
#endif
#if defined(GIRTH4) && (defined(GIRTH5) || defined(GIRTH6) || defined(GIRTH7))
#error "GIRTH4 can not be combined with GIRTH5, GIRTH6 or GIRTH7"
#endif
#define EVEN(x) (((x)&1)==0)
#define ODD(x) ((x)&1)

#ifdef GETSPLITS
unsigned long int numberbranches=0UL;
#endif
int splitlevel=-1, splitcounter;
int mod;
int presplitlevel= -1, presplitcounter, premod=1; // option j: only every premod-th node on edge level presplitlevel

#if (defined(GIRTH7))
LONGTYPE faraway[KNOTEN]; // wordt permanent overgeschreven
#endif


int cycstat[KNOTEN]={0}, cyccalls[KNOTEN]={0};

int (*endtestf)();
void (*codef)();
void tfc_flush(); void tfccode();
LONGTYPE (*set_OK_distances_pointer)(int v);

#ifdef FFSLOK
LONGTYPE _gbu;
#define FORALLELEMENTS(xx,yy) for (_gbu=xx, yy= FFSL(_gbu)-1; yy>=0; _gbu&=(_gbu-1), yy= FFSL(_gbu)-1) 
// define local variable _gbu if needed in several places
#define FORALLELEMENTS_NOINIT(yy) for (  ; yy>=0; _gbu&=(_gbu-1), yy= FFSL(_gbu)-1) // only useful after FORALLELEMENTS first.
// define local variable _gbu if needed in several places


LONGTYPE _gbu2;
#define FORALLELEMENTS2(xx,yy) for (_gbu2=xx, yy= FFSL(_gbu2)-1; yy>=0; _gbu2&=(_gbu2-1), yy= FFSL(_gbu2)-1)
// define local variable _gbu2 if needed in several places
#endif

int nv, nvh, girth, doublegirth, ne, finalne, isolimitne; // ne is the number of edges not in the initial 2-factor, so nv/2=nvh in the end
int maxlength, maxlength_odd, maxlength_even; // the maximum length of a cycle (resp. odd cycle) in the initial 2-factor
int cyclevector[KNOTEN]; // cyclevector[i]=k als er k cycles van lengte i in de originele 2-factor zitten.
LONGTYPE bitgraph[KNOTEN], initialgraph[KNOTEN], work_bitgraph[KNOTEN];
LONGTYPE dist2bitgraph[KNOTEN]; // vertices at distance at most 2 -- BUT: only vertices with degree _2_ and distance at most 2 are guaranteed to be in the set.
LONGTYPE dist3bitgraph[KNOTEN]; // vertices at distance at most 3 -- BUT: only vertices with degree _2_ and distance at most 3 are guaranteed to be in the set.
LONGTYPE allvertices, refset_o=EMPTYSET, refset_e=EMPTYSET, globalwithdeg3, globalwithdeg2;
int firstvertexof[KNOTEN+2]; // firstvertexof[i]: which vertex is the first in the cycle with vertex i. For the first vertex in the cycle it is set to 64
int previous_in2f[KNOTEN], next_in2f[KNOTEN]; // what is the previous resp. next vertex of vertex i in the initial 2-factor. Most of the time i-1, resp. i+1
unsigned long int counter=0UL, remember_counter=0UL, remember_time=0UL;
int representation[KNOTEN]; // Only describes the graph together with the initial 2-factor. representation[i] is the neighbour of vertex i not along an edge of the 2-factor
// one could also reconstruct the graph by "entry representation[i] is the larger of the two vertices of the ith edge inserted", but during comparison that might be slower
unsigned char twofaclengths[KNOTEN];

int incycle[KNOTEN]; // incycle[i]: in which cycle of the initial 2-factor is vertex i -- starts at number 0
int firstofcycle[KNOTEN>>2]; // firstofcycle[i]: what is the smallest vertex in cycle i
int cyclength_van_top[KNOTEN]; // cyclength_van_top[i] is the length of the cycle of the initial 2-factor containg i
int firstnumber[KNOTEN][KNOTEN>>2]; // firstnumber[i][j]: what is the first number of a cycle of length i in the 2-factor if j cycles have already been numbered?
LONGTYPE outgoing[KNOTEN>>2]; // outgoing[i]: the set of all neighbours of vertices in cycles i
LONGTYPE outgoing_upto[KNOTEN>>2];
LONGTYPE setcycle[KNOTEN>>2]; // setcycle[i] the set of vertices in cycle i
LONGTYPE setlargercycles[KNOTEN>>2]; // setcycle[i] the set of vertices in cycle i

int numbertwofac[KNOTEN]={0}; // numbertwofac[i]: how many twofactors containing edge number i with the same signature are there when ne==i
LONGTYPE *the_twofactors[(KNOTEN>>1)+1];
int maxnumbertwofactors[(KNOTEN>>1)+1];
NUMBERTYPE *the_cyclengths_van_top[(KNOTEN>>1)+1];
LONGTYPE *the_vertices_with_given_cyclength[(KNOTEN>>1)+1];
// these three arrays of pointers point to lists of 2-factors and some fields with data fror these 2-factors. The number of items in all three fields will always be the same.
// The index, so e.g. the i in the_twofactors[i], is the number of edges inserted into the initial 2-factor when the 2-factor was constructed (so it contains that last edge)

LONGTYPE deg3nb_global[KNOTEN>>1][4]; // deg3nb[i][j]: set of vertices of degree 3 with j neighbours of degree 3 when i edges have been inserted into the start-2-factor

LONGTYPE forbiddenlength, forbiddenlength_odd; // which lengths can nver be in a contradicting 2-factor and which lengths can never be in such a factor if an odd length has already been found

LONGTYPE allowedstrong[KNOTEN>>1][KNOTEN]; // allowedstrong[ne][i]: met welke toppen mag i bij optie strong nog verbonden worden als er al ne
// niet-2fak-bogen zijn zonder dat er een boog tussen precies 2 oneven cykels in een 2-faktor zou zijn

typedef struct isodata
{
  LONGTYPE *twofac;
  NUMBERTYPE nextpos;
  LONGTYPE numbered;
  LONGTYPE numbers_given;
  NUMBERTYPE number[KNOTEN];
  NUMBERTYPE invnumber[KNOTEN];
  NUMBERTYPE *cyclength_van_top;
  LONGTYPE *vertices_with_given_cyclength;
  NUMBERTYPE numbercycles[KNOTEN];
} ISOSTRUCT;

ISOSTRUCT **the_isodatapointers[(KNOTEN>>1)+1];
ISOSTRUCT *the_isodatastructs[(KNOTEN>>1)+1];
int numberisopointers[(KNOTEN>>1)+1], maxnumberisopointers[(KNOTEN>>1)+1];
int numberisostructs[(KNOTEN>>1)+1],  maxnumberisostructs[(KNOTEN>>1)+1];

struct tms timestruct;

#define UPTO(i) ((((LONGTYPE)1)<<((i)+1))-1)
#define LARGERTHAN(i) (~UPTO(i))

#define ADD_EDGE(i,j) { SETBIT(bitgraph[i],j); SETBIT(bitgraph[j],i); SETBIT(work_bitgraph[i],j); SETBIT(work_bitgraph[j],i); ne++; representation[i]=j; representation[j]=i;}
// Only if the edge {i,j} was the last edge added to both:
#define DEL_EDGE(i,j) { DELBIT(bitgraph[i],j); DELBIT(bitgraph[j],i); DELBIT(work_bitgraph[i],j); DELBIT(work_bitgraph[j],i); ne--; } //  representation only valid for deg 3 vertices, so it does not have to be reset

#define ADD_EDGEBG(bitgraph,i,j) { SETBIT(bitgraph[i],j); SETBIT(bitgraph[j],i); }
// Only if the edge {i,j} was the last edge added to both:
#define DEL_EDGEBG(bitgraph,i,j) { DELBIT(bitgraph[i],j); DELBIT(bitgraph[j],i); }

#define CLEARGRAPH { int i; for (i=0;i<KNOTEN;i++) { bitgraph[i]=(LONGTYPE)0; } }

/* Option N (added by Claude): only output graphs that are cyclically 5-connected and where every cyclic 5-cut separates a 5-cycle
   (no nontrivial cyclic 5-cuts). Implies c5 and girth at least 5.
   H denotes the current partial graph where each vertex of degree 2 gets a virtual edge to an extra vertex "infinity".
   If H has a set S of real vertices with |S|>=6, |V\S|>=6 and at most 5 edges between S and the rest (infinity on either side),
   then every completion is rejected: the final cut is at most that size and both sides contain cycles. Inserting edges never
   increases these cut sizes, which is used for early rejection and for backjumping after a rejection of a complete graph. */
int nontriv5=0, use_backjump=1;
int cutlimit, minside; // parameters for cycl_con_rek: look for cuts with fewer than cutlimit edges and sides with at least minside vertices
LONGTYPE witness_set; // the set found by the last successful cycl_con_rek
int jumplevel=-1; // if >=0: backtrack until the edge inserted as edge number jumplevel is removed
int edgelevel[KNOTEN]; // edgelevel[v]: ne after the non-2-factor edge at v was inserted
unsigned long int jumphist[KNOTEN]={0}, sidehist[KNOTEN]={0}, earlyrej[8]={0}, final_rejected=0UL, final_tested=0UL, htests=0UL;
int earlylevel=KNOTEN; // option E<k>: the search for H-cuts is done after inserting edge number k and later edges
LONGTYPE hdeg2, infbit; // for the H-search: the vertices of degree 2 and the bit of the vertex infinity (number nv)
#define HNB(v) (bitgraph[v] | (IS_SET(hdeg2,(v)) ? infbit : EMPTYSET)) // neighbours of real vertex v in H

typedef struct hstate
{ LONGTYPE set, out; // set never contains infinity, out always does
  int cut, lb, size; // cut: edges leaving set in H; lb: edges between set and out
} HSTATE;

int given_sequence=0, writeoutput=1, mincon=3, quiet=0, strong=0; // the mincon only helps to get rid of graphs with trivially too small cyclic connectivity. It does NOT guarantee a certain cyclic connectivity
int wanted_sequence[KNOTEN/2];
int testgap=4;

int cyclelengths[KNOTEN], howmanycycles[KNOTEN], numbercycles, numberoddcycles, use_oddness_result= 0;

int nextlevel_numbernotyetfixed(LONGTYPE twofac[], int nextpos, LONGTYPE numbered,	LONGTYPE numbers_given,NUMBERTYPE number[], NUMBERTYPE invnumber[],
				NUMBERTYPE cyclength_van_top[], LONGTYPE vertices_with_given_cyclength[], NUMBERTYPE numbercycles[]);
int cycl_con();
int nontriv5_final();

void writeset(char st[], LONGTYPE set)
{
  int i;
  fprintf(stderr,"%s:",st);
  for (i=0; i<(8*sizeof(LONGTYPE));i++) if IS_SET(set,i) fprintf(stderr," %d",i);
  fprintf(stderr,"\n");
} 

void writeset2(LONGTYPE set)
{
  int i;
  fprintf(stderr,"{");
  for (i=0; i<(8*sizeof(LONGTYPE));i++) if IS_SET(set,i) fprintf(stderr," %d",i);
  fprintf(stderr," }\n");
}

void writesetp1(char st[], LONGTYPE set)
{
  int i;
  fprintf(stderr,"%s {",st);
  for (i=0; i<(8*sizeof(LONGTYPE));i++) if IS_SET(set,i) fprintf(stderr," %d",i+1);
  fprintf(stderr," }\n");
}

void writeset2p1(LONGTYPE set)
{
  int i;
  fprintf(stderr,"{");
  for (i=0; i<(8*sizeof(LONGTYPE));i++) if IS_SET(set,i) fprintf(stderr," %d",i+1);
  fprintf(stderr," }\n");
}


void writelist(LONGTYPE list[], int number)
{
  int i;
  fprintf(stderr,"-------------%d elements-------------------\n",number);
  for (i=0;i<number;i++) writeset2(list[i]);
  fprintf(stderr,"--------------------------------\n");
}

void writegraph(LONGTYPE g[])
{
  int i;
  fprintf(stderr,"-------\n %d vertices, %d non-2-factor edges :\n",nv,ne);
  for (i=0; i<nv; i++) { fprintf(stderr,"%d: ",i); writeset2(g[i]); }
  fprintf(stderr,"--------\n");
} 

void writegraphp1(LONGTYPE g[])
{
  int i;
  fprintf(stderr,"-------\n %d vertices:\n",nv);
  for (i=0; i<nv; i++) { fprintf(stderr,"%d: ",i+1); writeset2p1(g[i]); }
  fprintf(stderr,"--------\n");
}

void usage(char name[])
{
  fprintf(stderr,"minisnark version %s\n",MINISNARK_VERSION);
  fprintf(stderr,"Usage: %s v g [cx] [q] [n] [t] [m a b] [j c d] [S] [R] [T] [N] [Kk] [Pp] [b] [w] [L] [M] [Ek] [F x1 x2 ...xk] [f x1 x2 ...xk]\n",name);
  fprintf(stderr,"v and g: Generate 3-connected snarks on v vertices with girth at least g>=4 (g=4 only if compiled with -DGIRTH4). \n");
  fprintf(stderr,"cx: with x a number -- generate only snarks with cyclic edge connectivity at least x\n");
  fprintf(stderr,"q: Suppress some output -- e.g. the intermediate output, reporting time and number for each set of initial cycles.\n");
  fprintf(stderr,"n: don't output any graphs, just count.\n");
  fprintf(stderr,"m a b with 0<=a<b: Only generate part a of in total b disjoint parts that together form the whole set of graphs.\n");
  fprintf(stderr,"j c d with 0<=c<d (with m): pre-split -- only part c of d on an earlier level, then part a of b of what is left.\n");
  fprintf(stderr,"   The d*b jobs (c,a) together form the whole set; each job does about 1/d of the work above the level of m.\n");
  fprintf(stderr,"S: only generate strong snarks.\n");
  fprintf(stderr,"N: only generate cyclically 5-connected snarks without nontrivial cyclic 5-cuts (implies c5).\n");
  fprintf(stderr,"Ek: with N -- after inserting the k-th non-2-factor edge also do a general search for sets that will give too small cuts.\n");
  fprintf(stderr,"    (experimental: in tests up to 26 vertices this was slower than without it)\n");
  fprintf(stderr,"b: with N -- no backjumping after a complete graph was rejected (only for comparisons).\n");
  fprintf(stderr,"w: no forward checking (only for comparisons).\n");
  fprintf(stderr,"L: no lifting of forbidden pairs (only for comparisons).\n");
  fprintf(stderr,"M: no matching test (only for comparisons).\n");
  fprintf(stderr,"Kk: from edge number k on, all possible partners are tested and the vertex with the fewest possible partners\n");
  fprintf(stderr,"    gets its edge first -- no isomorphism rejection there, so graphs can be written several times.\n");
  fprintf(stderr,"    For showing non-existence (e.g. K7 for girth 7).\n");
  fprintf(stderr,"Pp: with K -- if at most p vertices have degree 2, prune when every completion contains a bad pair of edges.\n");
  fprintf(stderr,"    Recommended for girth 7 (compiled with -DGIRTH7): K7 P18.\n");
  fprintf(stderr,"R: use a special encoding for the graphs -- see the header for details.\n");
  fprintf(stderr,"T: write the graphs as a tfc stream (canonical 2-factor code, format TFC1 of the tfc tools) -- best compressed with xz.\n");
  fprintf(stderr,"F x1 x2 ...xk: only generate snarks that have a 2-factor with cycle lengths x1,...,xk.\n");
  fprintf(stderr,"f x1 x2 ...xk: only generate snarks where the initial 2-factor with cycle lengths x1,...,xk is canonical.\n");
  exit(1);
}


#ifdef GIRTH4
LONGTYPE set_OK_distances_3(int v)
{
  return allvertices & (~(bit(v) | bitgraph[v]));
}

LONGTYPE set_OK_distances_4(int v)
{
  return allvertices &(~dist2bitgraph[v]);
}
#endif

LONGTYPE set_OK_distances_5(int v)
{
  //  LONGTYPE set;
  //int i;

  return allvertices &(~dist3bitgraph[v]);

  /*set=bit(v) | bitgraph[v];
  FORALLELEMENTS(bitgraph[v],i) { set |= dist2bitgraph[i]; }
  return allvertices &(~set);*/
}

LONGTYPE set_OK_distances_6(int v)
{
  LONGTYPE set;
  int i;

  set=bit(v) | bitgraph[v];
  FORALLELEMENTS(bitgraph[v],i) { set |= dist3bitgraph[i]; }
  return allvertices &(~set);
}


LONGTYPE set_OK_distances_atleast7(int v)
// returns the set of vertices of degree 2 to which v can be connected, together with possibly some degree 3 vertices.
// there can be deg 3 vertices too close -- they must be deleted later!
// girth must be at least 3. So the distance must be at least girth-1.
// first computes the set of vertices at a smaller distance -- that is: at most girth-2 and then complements it
{
  LONGTYPE set, inlist;
  int i, distances[KNOTEN], list[KNOTEN], ll, pos, top;

  // compute all vertices that are at a smaller distance
  set=bit(v) | bitgraph[v];
 
  ll=0;
  FORALLELEMENTS(bitgraph[v],i) { list[ll]=i; distances[ll]=6; ll++; set |= dist3bitgraph[i]; }
  // the initial value 5 takes into account that working from real distance d when the neighbours are considered, vertices at distance 3 are added

  inlist=bit(v) | bitgraph[v];
  for (pos=0; (pos<ll) && (distances[pos]<girth); pos++)
    {
      top=list[pos];
      FORALLELEMENTS(bitgraph[top] & (~inlist),i) { set |= dist3bitgraph[i];  list[ll]=i; distances[ll]=distances[pos]+1; ll++; SETBIT(inlist,i); }
    }
  // as it is only applied for graphs with girth at least girth, one doesn't have to test again for being in the list already -- otherwise vertices
  // could be added to the list twice -- but that would also not produce errors

  return allvertices &(~set);
}

void givenumbers(LONGTYPE g[], int v, int nextnumber, NUMBERTYPE number[], NUMBERTYPE invnumber[], LONGTYPE *numbered, LONGTYPE *numbers_given,
		 int direction, int call)
// numbers a component in a 2-regular graph in a certain direction
// if (call>0) numbered and numbers_given are already OK
{
  int next;
  LONGTYPE notseen;

  if (g==the_twofactors[0])
    // Claude: the initial 2-factor -- its cycles are the intervals firstofcycle[c]...firstofcycle[c]+cyclelengths[c]-1 in this order,
    // so the numbering can be computed without tracing the cycle (same result as the general code below)
    {
      int c, s, l, p, k, step, x, up, down;

      c=incycle[v]; s=firstofcycle[c]; l=cyclelengths[c]; p=v-s;
      up= s+((p+1==l) ? 0 : p+1); down= s+((p==0) ? l-1 : p-1); // the two neighbours of v
      // direction 0 goes to the smaller neighbour, direction 1 to the larger one
      if (direction==0) step= (up<down) ? 1 : l-1; else step= (up<down) ? l-1 : 1;
      for (k=0, x=p; k<l; k++)
	{ number[s+x]=nextnumber+k; invnumber[nextnumber+k]=s+x;
	  x+=step; if (x>=l) x-=l; }
      if (call==0)
	{ *numbered |= setcycle[c];
	  *numbers_given |= (UPTO(nextnumber+l-1) & (~UPTO(nextnumber-1))); }
      return;
    }

  if (call==0)
    {
      number[v]=nextnumber; invnumber[nextnumber]=v;
      SETBIT(*numbered,v);
      SETBIT(*numbers_given,nextnumber);
      nextnumber++;
      notseen= ~bit(v);

        if (direction==0) next=SMALLEST(g[v]); else next=SMALLEST(g[v]&(g[v]-1));
      
      while (next>=0)
	{
	  number[next]=nextnumber;  invnumber[nextnumber]=next;
	  SETBIT(*numbered,next);
	  SETBIT(*numbers_given,nextnumber);
	  nextnumber++;
	  DELBIT(notseen,next);
	  next=SMALLEST(g[next] & notseen); 
	}
    }
  else
  {
      number[v]=nextnumber; invnumber[nextnumber]=v;
      nextnumber++;
      notseen= ~bit(v);
      
      if (direction==0) next=SMALLEST(g[v]); else next=SMALLEST(g[v]&(g[v]-1));
      
      while (next>=0)
	{
	  number[next]=nextnumber;  invnumber[nextnumber]=next;
	  nextnumber++;
	  DELBIT(notseen,next);
	  next=SMALLEST(g[next] & notseen); 
	}
    }
}


void add_isodatapointer(ISOSTRUCT *p, int ne_p)
{
  ISOSTRUCT **dummy;

  if (numberisopointers[ne_p]==maxnumberisopointers[ne_p])
    {
      dummy=the_isodatapointers[ne_p];
      maxnumberisopointers[ne_p]*=2;
      the_isodatapointers[ne_p]=malloc(sizeof(ISOSTRUCT *)*maxnumberisopointers[ne_p]);
      if (the_isodatapointers[ne_p]==NULL) { fprintf(stderr,"Do not get memory for %d isodata pointers -- exit(1).\n", maxnumberisopointers[ne_p]); exit(1); }
      memcpy(the_isodatapointers[ne_p],dummy,sizeof(ISOSTRUCT *)*numberisopointers[ne_p]);
      free(dummy);
    }
  the_isodatapointers[ne_p][numberisopointers[ne_p]]=p;
  numberisopointers[ne_p]++;
  return;
}

void add_isodatastruct(LONGTYPE *twofac,  NUMBERTYPE nextpos, LONGTYPE numbered, LONGTYPE numbers_given,
		       NUMBERTYPE number[],  NUMBERTYPE invnumber[],  NUMBERTYPE *cyclength_van_top,
		       LONGTYPE *vertices_with_given_cyclength, NUMBERTYPE numbercycles[], int ne_d)
{
  ISOSTRUCT *dummy, *max;
 int i;

 
   if (numberisostructs[ne_d]==maxnumberisostructs[ne_d])
    {
      dummy=the_isodatastructs[ne_d];
      maxnumberisostructs[ne_d]*=2;
       the_isodatastructs[ne_d]=malloc(sizeof(ISOSTRUCT)*maxnumberisostructs[ne_d]);
       if (the_isodatastructs[ne_d]==NULL) { fprintf(stderr,"Do not get memory for %d isodata structs -- exit(1).\n", maxnumberisostructs[ne_d]); exit(1); }
       memcpy(the_isodatastructs[ne_d],dummy,sizeof(ISOSTRUCT)*numberisostructs[ne_d]);
       // unfortunately some pointers might still lead to old structs -- but they are all in the list ne_d:
       max=dummy+numberisostructs[ne_d];
       for (i=0;i<numberisopointers[ne_d]; i++)
	 { if ((the_isodatapointers[ne_d][i]>=dummy) && (the_isodatapointers[ne_d][i]<max)) { the_isodatapointers[ne_d][i]=the_isodatastructs[ne_d]+(the_isodatapointers[ne_d][i]-dummy); }}
       free(dummy);
     }

  dummy=the_isodatastructs[ne_d]+numberisostructs[ne_d];
  dummy->twofac=twofac;
  dummy->nextpos=nextpos;
  dummy->numbered=numbered;
  dummy->numbers_given=numbers_given;
  memcpy(dummy->number,number,sizeof(NUMBERTYPE)*nv);
  memcpy(dummy->invnumber,invnumber,sizeof(NUMBERTYPE)*nv);
  dummy->cyclength_van_top=cyclength_van_top;
  dummy->vertices_with_given_cyclength=vertices_with_given_cyclength;
  memcpy(dummy->numbercycles,numbercycles,sizeof(NUMBERTYPE)*nv);
  
  numberisostructs[ne_d]++;

  add_isodatapointer(dummy, ne_d);

}

int nextlevel(LONGTYPE twofac[], int firsttop, int nextpos, LONGTYPE numbered,
	      LONGTYPE numbers_given,NUMBERTYPE number[], NUMBERTYPE invnumber[], NUMBERTYPE cyclength_van_top[],
	      LONGTYPE vertices_with_given_cyclength[], NUMBERTYPE numbercycles[])
{
  int direction, top, buur, runnextpos, nextcyclength;

  nextcyclength=cyclength_van_top[firsttop];

  for (direction=0; direction<2; direction++)
    {
      givenumbers(twofac, firsttop,  firstnumber[nextcyclength][numbercycles[nextcyclength]], number, invnumber, &numbered, &numbers_given, direction, direction);
      // the second "direction" is also the call number
      numbercycles[nextcyclength]++;
      for ( runnextpos=nextpos; runnextpos<nv; runnextpos++) // the alternative representation can be compared at most as long as the original
	{
	  if (IS_SET(numbers_given,runnextpos))
	    {
	      top=invnumber[runnextpos];
	      if (IS_SET(globalwithdeg3,top))
		{ buur=SMALLEST(bitgraph[top]&(~twofac[top]));
		  if (IS_SET(numbered,buur))
		    {
		      if (number[buur]<representation[runnextpos]) { numbercycles[nextcyclength]--; return 1; }
		      if (number[buur]>representation[runnextpos]) break;
		    }
		  else // de buur zit in een andere cykel die nog niet genummered is. Dus op naar de iteratie die daar de twee mogelijkheden toetst.
		    {
		      if (nextlevel(twofac,buur,runnextpos,numbered,numbers_given,number,invnumber,cyclength_van_top, vertices_with_given_cyclength, numbercycles)) { numbercycles[nextcyclength]--; return 1;  }
		      break;
		    }
		}
	      else
		{
		  if (ne<=isolimitne) add_isodatastruct(twofac, runnextpos, numbered, numbers_given, number,  invnumber, cyclength_van_top,
				    vertices_with_given_cyclength, numbercycles, ne);
		  break;
		}
	    }
	  else // not yet set -- new cycle
	    {
	      if (nextlevel_numbernotyetfixed(twofac,runnextpos,numbered,numbers_given,number,invnumber,cyclength_van_top, vertices_with_given_cyclength, numbercycles))
		{ numbercycles[nextcyclength]--; return 1;  }
	      break;
	    }
	}
      numbercycles[nextcyclength]--;
    }
      
  

  return 0;
}



int nextlevel_numbernotyetfixed(LONGTYPE twofac[], int nextpos, LONGTYPE numbered,
				LONGTYPE numbers_given,NUMBERTYPE number[], NUMBERTYPE invnumber[], NUMBERTYPE cyclength_van_top[], LONGTYPE vertices_with_given_cyclength[], NUMBERTYPE numbercycles[])
// Here numbers are given to cycles that have not yet been reached, but must be numbered -- e.g. a second 5-cycle after the first has been numbered, but it has
// no connection to the first 5-cycle.
// returns 1 if the representation is smaller, 0 else
// Only tests the component of the vertex given number 0
{
  int i,top,buur,direction, nextcyclength,runnextpos;
  LONGTYPE _gbu, worknumbered, worknumbersgiven, filter;
  // first try: only look at consequences of the first cycle

  // first determine which cyclelength nextpos should have:

  for(i=0; numbercycles[cyclelengths[i]]==howmanycycles[cyclelengths[i]]; i++);
  nextcyclength=cyclelengths[i];
  numbercycles[nextcyclength]++;

  if (ne<=isolimitne) filter=allvertices; else filter=globalwithdeg3;

  
 //for (i=0; i<nv; i++) // i=0 and original direction is just what is stored in representation[]. The reverse direction still has to be tested
 // if ((local_cyclength_van_top[i]==cyclelengths[0]) && IS_SET(globalwithdeg3,i))
    FORALLELEMENTS(vertices_with_given_cyclength[nextcyclength]&filter&(~numbered),i)
     for (direction=0; direction<2; direction++)
       {
	 // eerst nummeren in de gewone richting
	 worknumbered=numbered;
	 worknumbersgiven=numbers_given;
	 givenumbers(twofac,i, firstnumber[nextcyclength][numbercycles[nextcyclength]-1],number,invnumber,&worknumbered,&worknumbersgiven,direction,0);

	 for (runnextpos=nextpos ;runnextpos<nv;runnextpos++) // the alternative representation can be compared at most as long as the original
	   {
	     if (IS_SET(worknumbersgiven,runnextpos))
	       {
		 top=invnumber[runnextpos];
		 if (IS_SET(globalwithdeg3,top))
		   { buur=SMALLEST(bitgraph[top]&(~twofac[top]));
		     if (IS_SET(worknumbered,buur))
		       {
			 if (number[buur]<representation[runnextpos]) { numbercycles[nextcyclength]--; return 1; }
			 if (number[buur]>representation[runnextpos]) { break; }
		       }
		     else // de buur zit in een andere cykel die nog niet genummered is. Dus op naar de iteratie die daar de twee mogelijkheden toetst.
		       {
			 if (nextlevel(twofac,buur,runnextpos,worknumbered,worknumbersgiven,number,invnumber,cyclength_van_top, vertices_with_given_cyclength, numbercycles))
			   { numbercycles[nextcyclength]--; return 1;  }
			 break;
		       }
		   }
		 else
		 {
		   if (ne<=isolimitne) add_isodatastruct(twofac, runnextpos, worknumbered, worknumbersgiven, number,  invnumber, cyclength_van_top,
				     vertices_with_given_cyclength, numbercycles, ne); 
		   break;
		 }
	       }
	     else // the next number belongs to a new still unreached cycle
	       {
		 if (nextlevel_numbernotyetfixed(twofac, runnextpos, worknumbered, worknumbersgiven, number, invnumber, cyclength_van_top, vertices_with_given_cyclength, numbercycles))
		   { numbercycles[nextcyclength]--; return 1; }
		 break;
	       }
	   }
       }
    numbercycles[nextcyclength]--;
  return 0;
  }

int smaller_rep_isostruct(LONGTYPE twofac[], int nextpos, LONGTYPE numbered,LONGTYPE numbersgiven,NUMBERTYPE number[], NUMBERTYPE invnumber[],
			   NUMBERTYPE cyclength_van_top[], LONGTYPE vertices_with_given_cyclength[], NUMBERTYPE numbercycles[])
{
  int top, buur;

  for (  ;nextpos<nv;nextpos++) // the alternative representation can be compared at most as long as the original
    {
      if (IS_SET(numbersgiven,nextpos))
	{
	  top=invnumber[nextpos];
	  if (IS_SET(globalwithdeg3,top))
	    { buur=SMALLEST(bitgraph[top]&(~twofac[top]));
	      if (IS_SET(numbered,buur))
		{
		  if (number[buur]<representation[nextpos]) return 1;
		  if (number[buur]>representation[nextpos]) return 0;
		}
	      else // de buur zit in een andere cykel die nog niet genummered is. Dus op naar de iteratie die daar de twee mogelijkheden toetst.
		{
		  if (nextlevel(twofac,buur,nextpos,numbered,numbersgiven,number,invnumber,cyclength_van_top, vertices_with_given_cyclength, numbercycles))
		    { return 1;  }
		  break;
		}
	    }
	  else
	    {
	      if (ne<=isolimitne) add_isodatastruct(twofac, nextpos, numbered, numbersgiven, number,  invnumber, cyclength_van_top,
				vertices_with_given_cyclength, numbercycles, ne); 
	      break;
	    }
	}
      else // the next number belongs to a new still unreached cycle
	{
	  if (nextlevel_numbernotyetfixed(twofac, nextpos, numbered, numbersgiven, number, invnumber, cyclength_van_top, vertices_with_given_cyclength, numbercycles))
	    { return 1; }
	  break;
	}
    }
  return 0;
}


void smaller_rep_init()
{
  int i,j,k,length;
  LONGTYPE marks, set, _gbu, *vertices_with_given_cyclength, *twofac;
  NUMBERTYPE *local_cyclength_van_top; // in the new 2-factor -- not the one used for construction
  // first try: only look at consequences of the first cycle

   marks=allvertices;

   twofac=the_twofactors[0];
   vertices_with_given_cyclength=the_vertices_with_given_cyclength[0];
   local_cyclength_van_top=the_cyclengths_van_top[0];

   // first assign the length of its cycle to each vertex v: local_cyclength_van_top[v]
   for (k=girth;k<=maxlength;k++) { vertices_with_given_cyclength[k]=EMPTYSET; }
   for ( ; marks; )
     {
       i=SMALLEST(marks); DELBIT(marks,i);
       set=bit(i);
       // must have two unmarked neighbours
       for (j=SMALLEST(twofac[i]), length=1; j>=0 ;  j=SMALLEST(twofac[j]&marks)) { DELBIT(marks,j); SETBIT(set,j); length++; }
       vertices_with_given_cyclength[length] |= set;
       FORALLELEMENTS(set,j) local_cyclength_van_top[j]=length;
     }
   return;
}

int smaller_rep_list( int which_ne, int number_in_list, LONGTYPE startset)
// returns 1 if the representation is smaller, 0 else
// Only tests the component of the vertex given number 0
// startset is the set of vertices that might be new starts for isomorphism -- so the set of vertices of the last edge for old 2-factors and all deg 3 vertices for new ones
{
  int i,j,k,top,buur,nextpos,direction, length;
  NUMBERTYPE number[KNOTEN], invnumber[KNOTEN], numbercycles[KNOTEN];
  LONGTYPE numbered, numbers_given;
  // the sets of vertices that have been numbered and the set of numbers that have been assigned.
  // Note that on higher levels these numbers need not be contiguous
  LONGTYPE marks, set, _gbu, *vertices_with_given_cyclength, *twofac;
  NUMBERTYPE *local_cyclength_van_top; // in the new 2-factor -- not the one used for construction
  // first try: only look at consequences of the first cycle

   marks=allvertices;

   for (k=girth;k<=maxlength;k++) { numbercycles[k]=0; }

   twofac=the_twofactors[which_ne]+(nv*number_in_list);
   vertices_with_given_cyclength=the_vertices_with_given_cyclength[which_ne]+(nv*number_in_list);
   local_cyclength_van_top=the_cyclengths_van_top[which_ne]+(nv*number_in_list);

   if ((which_ne==ne) || (which_ne>isolimitne))
     {
       // first assign the length of its cycle to each vertex v: local_cyclength_van_top[v]
       for (k=girth;k<=maxlength;k++) { vertices_with_given_cyclength[k]=EMPTYSET; }
       for ( ; marks; )
	 {
	   i=SMALLEST(marks); DELBIT(marks,i);
	   set=bit(i);
	   // must have two unmarked neighbours
	   for (j=SMALLEST(twofac[i]), length=1; j>=0 ;  j=SMALLEST(twofac[j]&marks)) { DELBIT(marks,j); SETBIT(set,j); length++; }
	   vertices_with_given_cyclength[length] |= set;
	   FORALLELEMENTS(set,j) local_cyclength_van_top[j]=length;
	 }
     }

   if ((vertices_with_given_cyclength[cyclelengths[0]]&startset)==EMPTYSET) return 0;

  numbercycles[cyclelengths[0]]=1; // here only those cycles are used and only one -- and that one starts at 0
 
 numbered=numbers_given=EMPTYSET;
 //for (i=0; i<nv; i++) // i=0 and original direction is just what is stored in representation[]. The reverse direction still has to be tested
 // if ((local_cyclength_van_top[i]==cyclelengths[0]) && IS_SET(globalwithdeg3,i))
 FORALLELEMENTS(vertices_with_given_cyclength[cyclelengths[0]]&startset /*globalwithdeg3*/,i)
     for (direction=0; direction<2; direction++)
       {
	 // eerst nummeren in de gewone richting
	 if (NOT_SET(numbered,i)) // the vertex is in another cycle than the previous, so the sets numbered and numbers_given have to be recomputed
	   { numbered=numbers_given=EMPTYSET;
	     givenumbers(twofac,i,0,number,invnumber,&numbered,&numbers_given,direction,0);
	   }
	   else givenumbers(twofac,i,0,number,invnumber,&numbered,&numbers_given,direction,1);
	 for (nextpos=0;nextpos<nv;nextpos++) // the alternative representation can be compared at most as long as the original
	   if (IS_SET(numbers_given,nextpos))
	     {
	       top=invnumber[nextpos];
	       if (IS_SET(globalwithdeg3,top))
		 { buur=SMALLEST(bitgraph[top]&(~twofac[top]));
		   if (IS_SET(numbered,buur))
		     {
		       if (number[buur]<representation[nextpos]) { return 1;}
		       if (number[buur]>representation[nextpos]) break;
		     }
		   else // de buur zit in een andere cykel die nog niet genummered is. Dus op naar de iteratie die daar de twee mogelijkheden toetst.
		     {
		       if (nextlevel(twofac,buur,nextpos,numbered,numbers_given,number,invnumber,local_cyclength_van_top, vertices_with_given_cyclength, numbercycles)) { return 1;  }
		       break; // nextpos loop
		     }
		 }
	       else
		 {  
		   if (ne<=isolimitne) add_isodatastruct(twofac, nextpos, numbered, numbers_given, number,  invnumber, local_cyclength_van_top,
				     vertices_with_given_cyclength, numbercycles, ne); 
		   break;
		 }
	     }
	   else // the next number belongs to a new still unreached cycle
	     {
	       if (nextlevel_numbernotyetfixed(twofac, nextpos, numbered, numbers_given, number, invnumber, local_cyclength_van_top, vertices_with_given_cyclength, numbercycles))
		 { return 1; }
	       break;
	     }
       }
  return 0;
  }

void init(int cyclelengths[], int numcycs)
{
  int i, nvsofar, limit, evencycles, next;
 
  numbercycles=numcycs;
  if (codef==tfccode) tfc_flush(); // a new initial 2-factor starts a new batch
  for (i=0;i<numcycs;i++) twofaclengths[i]=cyclelengths[i];
  
   for (i=4;i<nv-4;i++) howmanycycles[i]=cyclevector[i]=0;
  
   if (!quiet)
     { fprintf(stderr,"Initial 2-factor: ");
       for (next=i=0;i<numcycs;i++)
	 { fprintf(stderr,"%d ",cyclelengths[i]);
	   firstnumber[cyclelengths[i]][howmanycycles[cyclelengths[i]]]=next; 
	   next+=cyclelengths[i];
	   howmanycycles[cyclelengths[i]]++;
	 }  fprintf(stderr,"\n");
     }
   else
     { 
       for (next=i=0;i<numcycs;i++)
	 { 
	   firstnumber[cyclelengths[i]][howmanycycles[cyclelengths[i]]]=next; 
	   next+=cyclelengths[i];
	   howmanycycles[cyclelengths[i]]++;
	 } 
     }
     
  for (i=evencycles=0;i<numcycs;i++) if (EVEN(cyclelengths[i])) evencycles++;

  CLEARGRAPH;

  if (maxlength_odd>maxlength_even) maxlength=maxlength_odd; else maxlength=maxlength_even;

  for (i=nvsofar=0;i<numcycs;i++)
    {
      cyclevector[cyclelengths[i]]++;
      firstofcycle[i]=nvsofar;
      incycle[nvsofar]=i;
      previous_in2f[nvsofar]=nvsofar+cyclelengths[i]-1; next_in2f[nvsofar]=nvsofar+1;
      cyclength_van_top[nvsofar]=cyclelengths[i];
      limit=nvsofar+cyclelengths[i]-1;
      setcycle[i]=(UPTO(limit) & (~UPTO(nvsofar-1)));
      ADD_EDGE(nvsofar, limit);
      for (nvsofar++ ; nvsofar<=limit; nvsofar++)
	{incycle[nvsofar]=i;
	  cyclength_van_top[nvsofar]=cyclelengths[i];
	  previous_in2f[nvsofar]=nvsofar-1; next_in2f[nvsofar]=nvsofar+1;
	  ADD_EDGE(nvsofar, nvsofar-1); }
      next_in2f[nvsofar-1]=nvsofar-cyclelengths[i];
      if (i==0) setlargercycles[i]= (allvertices & ~setcycle[i]);
      else setlargercycles[i]= (setlargercycles[i-1] & ~setcycle[i]);
    }
  ne=0;
  memcpy(work_bitgraph,bitgraph,sizeof(LONGTYPE)*nv); // from now on work_bitgraph is kept equal to bitgraph outside the 2-factor tests

  for (i=0; i<nv;i++) dist2bitgraph[i]= bitgraph[i] | bitgraph[SMALLEST_CTZ(bitgraph[i])] | bitgraph[LARGEST_CLZ(bitgraph[i])] ;
  for (i=0; i<nv;i++) dist3bitgraph[i]= dist2bitgraph[i] | dist2bitgraph[SMALLEST_CTZ(bitgraph[i])] | dist2bitgraph[LARGEST_CLZ(bitgraph[i])] ;

#if (defined(GIRTH7))
  for (i=0;i<nv;i++) faraway[i]=(*set_OK_distances_pointer)(i);
#endif


  memcpy(initialgraph,bitgraph,sizeof(LONGTYPE)*nv);
  memcpy(the_twofactors[0],initialgraph,sizeof(LONGTYPE)*nv);
  numbertwofac[0]=1;
  numberisopointers[0]=numberisostructs[0]=0;

  smaller_rep_init();
  jumplevel= -1;

   if (strong) 
    {
      for (i=0;i<nv;i++) allowedstrong[0][i]=allvertices;
      if (numberoddcycles==2)
	{
	  for (i=0;i<cyclelengths[0];i++) allowedstrong[0][i] &= (~setcycle[1]);
	  for ( ;i<cyclelengths[0]+cyclelengths[1];i++) allowedstrong[0][i] &= (~setcycle[0]); // overbodig, maar zo is het zuiverder
	}
    }

   return;
}

void runtimecode()
{
  static unsigned char code1[KNOTEN]={0}, code2[KNOTEN]={0};
  unsigned char *oldcode, *newcode;
  int i,j,pos;
 
  if (nontriv5 ? nontriv5_final() : cycl_con()) return;

  counter++;
  
  if (counter& (1UL))
    { oldcode=code1; newcode=code2; }
  else
    { oldcode=code2; newcode=code1; }

  for (pos=0;pos<numbercycles;pos++) newcode[pos]=cyclelengths[pos];
  newcode[pos]=0; pos++; 

  for (i=0;i<nv-1;i++)
    {
      j=FFSL(bitgraph[i]&(~initialgraph[i])); // starts at 1!
      if (j>i) { newcode[pos]=j; pos++; }
    }

  for (i=0; newcode[i]==oldcode[i]; i++); // there must always be a difference

  putchar(i);
  for( ; i<pos; i++) putchar(newcode[i]);

}

#define TFC_BATCH 10000000UL // at most this number of graphs in one batch of the tfc stream
unsigned char *tfc_buf=NULL, tfc_prev[2*KNOTEN];
size_t tfc_len=0, tfc_cap=0;
unsigned long tfc_count=0;
int tfc_prevlen=0;

void tfc_put(unsigned char b)
{
  if (tfc_len==tfc_cap)
    { tfc_cap= tfc_cap ? 2*tfc_cap : (1<<20);
      tfc_buf=realloc(tfc_buf,tfc_cap);
      if (tfc_buf==NULL) { fprintf(stderr,"Do not get room for the tfc buffer -- exit(1).\n"); exit(1); } }
  tfc_buf[tfc_len++]=b;
}

void tfc_flush()
// writes the graphs collected so far as one batch: 1, transform 3 (vrank), flags 11, girth, count (varint), records
{
  unsigned long c=tfc_count;

  if (tfc_count==0) return;
  putchar(1); putchar(3); putchar(11); putchar(girth);
  for ( ; c>=128; c>>=7) putchar((int)((c&127)|128));
  putchar((int)c);
  fwrite(tfc_buf,1,tfc_len,stdout);
  tfc_len=0; tfc_count=0; tfc_prevlen=0;
}

void tfccode()
{
  unsigned char rec[2*KNOTEN];
  int i, p, k, c, len=0, r;
  LONGTYPE unmatched, ball, nbs, s, cand, known[KNOTEN];

  if (nontriv5 ? nontriv5_final() : cycl_con()) return;

  counter++;

  if (!writeoutput) return;

  r= (girth-2>1) ? girth-2 : 1; // a partner at distance at most r would close a cycle shorter than the girth
  rec[len++]=nv;
  for (i=0;i<numbercycles;i++) rec[len++]=cyclelengths[i];
  memcpy(known,initialgraph,sizeof(LONGTYPE)*nv);
  for (unmatched=allvertices; unmatched; )
    {
      i=SMALLEST_CTZ(unmatched);
      p=SMALLEST_CTZ(bitgraph[i]&(~initialgraph[i]));
      for (ball=bit(i), k=0; k<r; k++)
	{ for (nbs=ball, s=ball; s; s&=s-1) nbs|=known[SMALLEST_CTZ(s)];
	  ball=nbs; }
      cand= unmatched & (~UPTO(i)) & (~ball);
      if (NOT_SET(cand,p)) { fprintf(stderr,"Error in tfccode(): partner %d of %d is not a candidate.\n",p,i); exit(1); }
      if (POPC(cand)>1) rec[len++]=POPC(cand&(bit(p)-1)); // with only one candidate the entry is omitted
      SETBIT(known[i],p); SETBIT(known[p],i);
      DELBIT(unmatched,i); DELBIT(unmatched,p);
    }

  if (tfc_count==0) for (k=0;k<len;k++) tfc_put(rec[k]);
  else
    { // front coding: length of the common prefix, then the rest -- the first differing byte as the difference minus 1
      for (c=0; (c<len) && (c<tfc_prevlen) && (rec[c]==tfc_prev[c]); c++);
      tfc_put(c);
      if (c<len) { tfc_put((unsigned char)(rec[c]-tfc_prev[c]-1)); for (k=c+1;k<len;k++) tfc_put(rec[k]); }
    }
  memcpy(tfc_prev,rec,len); tfc_prevlen=len;
  tfc_count++;
  if (tfc_count>=TFC_BATCH) tfc_flush();
}

void output()
{
  int i,j;
  LONGTYPE _gbu, mask;

  if (nontriv5 ? nontriv5_final() : cycl_con()) return;

  counter++;

   if (!writeoutput) return;
  
  putchar(nv);
  mask=allvertices;
  for (i=0;i<nv-1;i++)
    { DELBIT(mask,i);
      FORALLELEMENTS(bitgraph[i]&mask,j) putchar(j+1);
      putchar(0);
    }

}

void output2()
{
  int i,j;
  LONGTYPE _gbu, mask;

 
  putchar(nv);
  mask=allvertices;
  for (i=0;i<nv-1;i++)
    { DELBIT(mask,i);
      FORALLELEMENTS(bitgraph[i]&mask,j) putchar(j+1);
      putchar(0);
    }
  fflush(stdout);
}

int compareg(LONGTYPE bitg[])
{
  LONGTYPE verg[KNOTEN];

  int i;

  for (i=0;i<nv;i++) verg[i]=bitg[i];

  verg[0]=bit(1) | bit(4);
  verg[1]=bit(0) | bit(7);
   verg[4]=bit(0) | bit(8);
  verg[14]=bit(13) | bit(18);
  verg[10]=bit(9) | bit(11);

  for (i=0;i<nv;i++) if (verg[i]!=bitg[i]) return 0;

  return 1;
}

/**********************STORE_TWOFACTOR********************/
void store_twofactor(int ne, LONGTYPE twofac[])
{
  LONGTYPE *dummy;

  if (numbertwofac[ne]==maxnumbertwofactors[ne])
    {
      dummy=the_twofactors[ne];
      maxnumbertwofactors[ne] *= 2;
      the_twofactors[ne]=malloc(sizeof(LONGTYPE)*nv*maxnumbertwofactors[ne]);
      if (the_twofactors[ne]==NULL) { fprintf(stderr,"Do not get room for %d twofactors -- exit(1).\n",maxnumbertwofactors[ne] ); exit(1); }
      memcpy(the_twofactors[ne],dummy,sizeof(LONGTYPE)*nv*numbertwofac[ne]);
      free(dummy);
      free(the_cyclengths_van_top[ne]);
      the_cyclengths_van_top[ne]=malloc(sizeof(NUMBERTYPE)*nv*maxnumbertwofactors[ne]);
      if (the_cyclengths_van_top[ne]==NULL) { fprintf(stderr,"Do not get room for %d invariants of twofactors -- exit(1).\n",maxnumbertwofactors[ne] ); exit(1); }
      free(the_vertices_with_given_cyclength[ne]);
      the_vertices_with_given_cyclength[ne]=malloc(sizeof(LONGTYPE)*nv*maxnumbertwofactors[ne]);
      if (the_vertices_with_given_cyclength[ne]==NULL) { fprintf(stderr,"Do not get room for %d invariants of twofactors -- exit(1).\n",maxnumbertwofactors[ne] ); exit(1); }
      // as first all twofactors are filled in, the last two arrays will be filled in when all 2-factors are filled in (or not at all if an even 2-factor is found)
    }
  memcpy(the_twofactors[ne]+(numbertwofac[ne]*nv),twofac,sizeof(LONGTYPE)*nv);
  numbertwofac[ne]++;
}



/****************************ENDTEST_FIX2FAK************************/

int fc_nostore=0, fwdcheck=1;
int fc_wantlevel=0, fc_level=0, fc_lift=1, fc_v, fc_w; // lifting: a rejected pair is stored on the level of the largest inserted edge of the 2-factor found
unsigned long fc_lifted=0, fc_liftdist[KNOTEN]={0}, pm_prune=0;
int pm_use=1; // matching test
LONGTYPE pm_P[KNOTEN];

#ifdef GIRTH7
/* Edmonds' blossom algorithm for the matching test (Claude, November 2026; only compiled for girth at least 7): a greedy matching of the graph of possible
   partners pm_P[] (restricted to alive) is augmented. If there is no augmenting path from an unmatched vertex, there is none
   for any larger matching either, so there is no perfect matching. */
int bm_match[KNOTEN], bm_p[KNOTEN], bm_base[KNOTEN];
LONGTYPE bm_blossom;

int bm_lca(int a, int b)
{
  LONGTYPE used=EMPTYSET;

  for (;;) { a=bm_base[a]; SETBIT(used,a); if (bm_match[a]<0) break; a=bm_p[bm_match[a]]; }
  for (;;) { b=bm_base[b]; if (IS_SET(used,b)) return b; b=bm_p[bm_match[b]]; }
}

void bm_mark(int v, int b, int child)
{
  while (bm_base[v]!=b)
    { SETBIT(bm_blossom,bm_base[v]); SETBIT(bm_blossom,bm_base[bm_match[v]]);
      bm_p[v]=child; child=bm_match[v]; v=bm_p[bm_match[v]]; }
}

int bm_find(int root, LONGTYPE alive)
// end of an augmenting path from root, -1 if there is none
{
  LONGTYPE used, s, t;
  int q[KNOTEN], qh=0, qt=0, v, to, cb, i;

  for (s=alive; s; s&=s-1) { i=SMALLEST_CTZ(s); bm_p[i]= -1; bm_base[i]=i; }
  used=bit(root); q[qt++]=root;
  while (qh<qt)
    { v=q[qh++];
      for (s=pm_P[v]&alive; s; s&=s-1)
	{ to=SMALLEST_CTZ(s);
	  if ((bm_base[v]==bm_base[to]) || (bm_match[v]==to)) continue;
	  if ((to==root) || ((bm_match[to]>=0) && (bm_p[bm_match[to]]>=0)))
	    { // a blossom: its vertices get the base cb and are put into the queue
	      cb=bm_lca(v,to); bm_blossom=EMPTYSET;
	      bm_mark(v,cb,to); bm_mark(to,cb,v);
	      for (t=alive; t; t&=t-1)
		{ i=SMALLEST_CTZ(t);
		  if (IS_SET(bm_blossom,bm_base[i])) { bm_base[i]=cb; if (NOT_SET(used,i)) { SETBIT(used,i); q[qt++]=i; } } } }
	  else if (bm_p[to]<0)
	    { bm_p[to]=v;
	      if (bm_match[to]<0) return to;
	      to=bm_match[to]; SETBIT(used,to); q[qt++]=to; } } }
  return -1;
}

int pm_blossom(LONGTYPE alive)
// Is there a perfect matching of the vertices in alive in the graph of possible partners pm_P[]?
{
  LONGTYPE s, unmatched=alive, c;
  int v, u, pv, ppv;

  for (s=alive; s; s&=s-1) bm_match[SMALLEST_CTZ(s)]= -1;
  for (s=alive; s; s&=s-1)
    { v=SMALLEST_CTZ(s); if (NOT_SET(unmatched,v)) continue;
      c=pm_P[v]&unmatched; if (c==EMPTYSET) continue;
      u=SMALLEST_CTZ(c); bm_match[v]=u; bm_match[u]=v; unmatched&=~(bit(v)|bit(u)); }
  for (s=unmatched; s; s&=s-1)
    { v=SMALLEST_CTZ(s); if (bm_match[v]>=0) continue;
      u=bm_find(v,alive); if (u<0) return 0;
      while (u>=0) { pv=bm_p[u]; ppv=bm_match[pv]; bm_match[u]=pv; bm_match[pv]=u; u=ppv; } }
  return 1;
}

#define PM_STEPS 32 // pm_rec() hands over to pm_blossom() after this many steps
#endif

int pm_rec(LONGTYPE alive)
// Is there a perfect matching of the vertices in alive in the graph of possible partners pm_P[]?
// The vertex with the fewest possible partners is matched first.
// Claude: iterative version of the former recursive search -- the same vertices are chosen and the partners are tried in
// the same order. A frame is only stored if there are untried partners left (if not, a failure is passed on directly).
// For girth at least 7 (-DGIRTH7): the search is fast when there is a perfect matching, but can take thousands of steps when
// there is none -- so after PM_STEPS steps the question is decided by pm_blossom() (November 2026: about 2.5 times faster at
// 18-22 vertices of degree 2). The answer is the same.
{
  int v, w, d, best, top=0;
  LONGTYPE s, c, st_alive[KNOTEN/2+1], st_c[KNOTEN/2+1];
  int st_v[KNOTEN/2+1];
#ifdef GIRTH7
  int steps=PM_STEPS;
  LONGTYPE alive0=alive;
#endif

  for (;;)
    {
      if (alive==EMPTYSET) return 1;
#ifdef GIRTH7
      if (--steps<0) return pm_blossom(alive0);
#endif
      // the chosen vertex: the smallest one with at most 1 partner if there is one, otherwise the smallest one with the
      // smallest number of partners (computed without branches as the minimum of a key)
      { unsigned int key, minkey=~0U;
	for (s=alive; s; s&=s-1)
	  { w=SMALLEST_CTZ(s); d=POPC(pm_P[w]&alive); key=((d>1) ? (((unsigned int)d)<<6) : 0U) | (unsigned int)w; if (key<minkey) minkey=key; }
	v=(int)(minkey&63U); best=POPC(pm_P[v]&alive); }
      if (best==0)
	{ // backtrack to the last frame with untried partners
	  if (top==0) return 0;
	  top--; alive=st_alive[top]; c=st_c[top]; v=st_v[top];
	}
      else c=pm_P[v]&alive;
      w=SMALLEST_CTZ(c); c&=c-1;
      if (c) { st_alive[top]=alive; st_c[top]=c; st_v[top]=v; top++; }
      alive&=~(bit(v)|bit(w));
    }
}
unsigned long fc_tests=0, fc_prune=0, fc_rtests=0;
LONGTYPE fc_bad[(KNOTEN>>1)+1][KNOTEN]; // fc_bad[ne][v]: partners w such that G+{v,w} has an even or better 2-factor through {v,w}
signed char fc_wit[(KNOTEN>>1)+1][KNOTEN]; // fc_wit[ne][v]: a partner of v that passed the test on this level
LONGTYPE fc_verified[(KNOTEN>>1)+1]; // the vertices for which fc_wit[ne][] is valid
int fc_restrict=0, fc_ea, fc_eb; // for contradicting_twofac_through()
int (*fc_saved_endtest)();
int endtest_fix2fak()
  /* Berechnet die Zykellaengen und zieht die Konsequenzen */
  /* kanonisch ist hier immer der originale 2-Faktor! */

{
  int i, j, k, length, rest, oldrest;
 LONGTYPE marks;
 LONGTYPE lset_o=EMPTYSET, lset_e=EMPTYSET;
#if defined(GIRTH6) || defined(GIRTH7)
int z,u;
#elif defined(GIRTH5)
int z; 
#endif

 marks=allvertices;
 rest=nv;
    
 for (rest=nv; rest; )
   {
     oldrest=rest;
     if (rest<doublegirth) { rest=0;}
     else
       {
	 i=SMALLEST_CTZ(marks);
#if defined(GIRTH6) || defined(GIRTH7)
	 z=SMALLEST_CTZ(work_bitgraph[i]);
	 k=SECONDSMALLEST_CTZ(work_bitgraph[i]);
	 DELBIT(marks,i);
	 j=SMALLEST_CTZ(work_bitgraph[z]&marks);
	 u=SMALLEST_CTZ(work_bitgraph[k]&marks);
	 marks &= ~(work_bitgraph[z] | work_bitgraph[k] | work_bitgraph[i] | work_bitgraph[u]); 
	 for (rest-=6; (work_bitgraph[j]&marks) != EMPTYSET ; )
#elif defined(GIRTH5)
	 z=SMALLEST_CTZ(work_bitgraph[i]);
	 k=SECONDSMALLEST_CTZ(work_bitgraph[i]);
	 j=SMALLEST_CTZ(work_bitgraph[z]&(~bit(i)));
	 marks &= ~(work_bitgraph[z] | work_bitgraph[k] | work_bitgraph[i]); 
	 for (rest-=5; (work_bitgraph[j]&marks) != EMPTYSET ; )
#elif defined(GIRTH4)
	 j=SMALLEST_CTZ(work_bitgraph[i]);
	 k=SECONDSMALLEST_CTZ(work_bitgraph[i]);		
	 marks &= ~(work_bitgraph[i] | work_bitgraph[k]); // also here girth at least 4 is used
	 for (rest-=4; (work_bitgraph[j]&marks) != EMPTYSET ; )
#endif
	   { j=SMALLEST_CTZ(work_bitgraph[j]&marks); 
	     DELBIT(marks,j);
	     rest--; if (rest<girth) { rest=0; break; }
	     }
       }

     length=oldrest-rest;

      if (length&1)
	{
	  SETBIT2(lset_o,length);
	   if (lset_o>refset_o)  { return 0; }
	  if (rest && (lset_o+bit2(rest))<refset_o) { return 0; }
	}
      else
	{
	  if ((lset_o!=EMPTYSET) && rest &&  ((lset_o+bit2(rest))<refset_o))  { return 0; }
	  SETBIT2(lset_e,length);
	}
    }

 if (lset_o==EMPTYSET)  {return 1; }
 if (lset_o<refset_o)  { return 0; }
 if (lset_e>refset_e) { return 0; }
 if (lset_e<refset_e) { return 0; }
 
 if (!fc_nostore) store_twofactor(ne,work_bitgraph);

 return 0;
}


/****************************ENDTEST************************/

int endtest()
  /* Berechnet die Zykellaengen und zieht die Konsequenzen */

{
  int i, j, k, length, rest, oldrest;
 LONGTYPE marks;
#if !defined(GIRTH6) && !defined(GIRTH7)
 int b;
 LONGTYPE nbset;
#endif
 LONGTYPE lset_o=EMPTYSET, lset_e=EMPTYSET;
 #ifdef GIRTH7
 int z,u,w,b,pj,pb,nx;
 LONGTYPE cyc;
#else
#ifdef GIRTH6
int z,u;
#else
#ifdef GIRTH5
int z; 
#endif
#endif
#endif
 
 marks=allvertices;
 rest=nv;
    
 for (rest=nv; rest; )
   {
     oldrest=rest;
     if (rest<doublegirth) { rest=0;}
     else
       {
	 i=SMALLEST_CTZ(marks);
#ifdef GIRTH7
	 // the 7 vertices i, z, w, j and k, u, b of the cycle are distinct (girth 7); the rest of the cycle is a path between
	 // the neighbours of j and of b: it is traced from both ends at the same time. The 2-factor is 2-regular, so the next
	 // vertex only depends on the previous one -- the two chains are independent (marks is only updated after the cycle).
	 z=SMALLEST_CTZ(work_bitgraph[i]);
	 k=SECONDSMALLEST_CTZ(work_bitgraph[i]);
	 w=SMALLEST_CTZ(work_bitgraph[z]^bit(i));
	 u=SMALLEST_CTZ(work_bitgraph[k]^bit(i));
	 j=SMALLEST_CTZ(work_bitgraph[w]^bit(z)); pj=w;
	 b=SMALLEST_CTZ(work_bitgraph[u]^bit(k)); pb=u;
	 cyc=bit(i)|bit(z)|bit(k)|bit(w)|bit(u)|bit(j)|bit(b);
	 for (rest-=7; ; )
	   { nx=SMALLEST_CTZ(work_bitgraph[j]^bit(pj)); if (nx==b) break;
	     pj=j; j=nx; cyc|=bit(j);
	     rest--; if (rest<girth) { rest=0; break; }
	     nx=SMALLEST_CTZ(work_bitgraph[b]^bit(pb)); if (nx==j) break;
	     pb=b; b=nx; cyc|=bit(b);
	     rest--; if (rest<girth) { rest=0; break; } }
	 marks&=~cyc;
#else
#ifdef GIRTH6
	 z=SMALLEST_CTZ(work_bitgraph[i]);
	 k=SECONDSMALLEST_CTZ(work_bitgraph[i]);
	 DELBIT(marks,i);
	 j=SMALLEST_CTZ(work_bitgraph[z]&marks);
	 u=SMALLEST_CTZ(work_bitgraph[k]&marks);
	 marks &= ~(work_bitgraph[z] | work_bitgraph[k] | work_bitgraph[i] | work_bitgraph[u]); 
	 for (rest-=6; (work_bitgraph[j]&marks) != EMPTYSET ; )
	   { j=SMALLEST_CTZ(work_bitgraph[j]&marks); 
	     DELBIT(marks,j);
	     rest--; if (rest<girth) { rest=0; break; }
	     }
#else
#ifdef GIRTH5
	 z=SMALLEST_CTZ(work_bitgraph[i]);
	 k=SECONDSMALLEST_CTZ(work_bitgraph[i]);
	 j=SMALLEST_CTZ(work_bitgraph[z]&(~bit(i)));
	 b=SMALLEST_CTZ(work_bitgraph[k]&(~bit(i)));
	 marks &= ~(work_bitgraph[z] | work_bitgraph[k] | work_bitgraph[i]); 
	 rest-=5;
#elif defined(GIRTH4)
	 j=SMALLEST_CTZ(work_bitgraph[i]);
	 k=SECONDSMALLEST_CTZ(work_bitgraph[i]);		
	 b=SMALLEST_CTZ(work_bitgraph[k]&(~bit(i)));
	 marks &= ~(work_bitgraph[i] | work_bitgraph[k]); // also here girth at least 4 is used
	 rest-=4;
#endif
	 // the unmarked vertices of the cycle form a path between the neighbours of j and b: it is traced from both ends
	 // at the same time -- the two chains of dependent instructions are independent, so they can be executed in parallel
	 for ( ; ; )
	   { nbset=work_bitgraph[j]&marks; if (nbset==EMPTYSET) break;
	     j=SMALLEST_CTZ(nbset); DELBIT(marks,j);
	     rest--; if (rest<girth) { rest=0; break; }
	     nbset=work_bitgraph[b]&marks; if (nbset==EMPTYSET) break;
	     b=SMALLEST_CTZ(nbset); DELBIT(marks,b);
	     rest--; if (rest<girth) { rest=0; break; }
	   }
#endif
#endif
       }

     length=oldrest-rest;

      if (length&1)
	{
	  SETBIT2(lset_o,length);
	   if (lset_o>refset_o)  { return 1; }
	  if (rest && (lset_o+bit2(rest))<refset_o) { return 0; }
	}
      else
	{
	  if ((lset_o!=EMPTYSET) && rest &&  ((lset_o+bit2(rest))<refset_o))  { return 0; }
	  SETBIT2(lset_e,length);
	}
    }

 if (lset_o==EMPTYSET)  {return 1; }
 if (lset_o<refset_o)  { return 0; }
 if (lset_e>refset_e) { return 1; }
 if (lset_e<refset_e) { return 0; }
 
 if (!fc_nostore) store_twofactor(ne,work_bitgraph);

 return 0;
}
/****************************ENDTEST************************/

int endtest_strong()
  /* Berechnet die Zykellaengen und zieht die Konsequenzen */

{
  int i, j, k, length, rest, oldrest,oddones;
  LONGTYPE marks,set[16]; // maar 2 sets worden gebruikt, maar zo kan toegevoegd worden zonder te testen of er al 2 zijn
  LONGTYPE lset_o=EMPTYSET, lset_e=EMPTYSET, oldmarks, _gbu;
 #ifdef GIRTH7
 int z,u,w;
#else
#ifdef GIRTH6
int z,u;
#else
#ifdef GIRTH5
int z; 
#endif
#endif
#endif
 
 marks=allvertices;
 rest=nv;
 oddones=0;
    
 for (rest=nv; rest; )
   {
     oldrest=rest;
     oldmarks=marks;
     if (rest<doublegirth) { rest=0; marks=EMPTYSET;}
     else
       {
	 i=SMALLEST_CTZ(marks);
#ifdef GIRTH7
	 z=SMALLEST_CTZ(work_bitgraph[i]);
	 k=SECONDSMALLEST_CTZ(work_bitgraph[i]);
	 DELBIT(marks,i);
	 w=SMALLEST_CTZ(work_bitgraph[z]&marks);
	 u=SMALLEST_CTZ(work_bitgraph[k]&marks);
	 marks &= ~(work_bitgraph[z] | work_bitgraph[k] | work_bitgraph[i] | work_bitgraph[u]);
	 j=SMALLEST_CTZ(work_bitgraph[w]&marks);
	 DELBIT(marks,j);
	 for (rest-=7; (work_bitgraph[j]&marks) != EMPTYSET ; )
#else
#ifdef GIRTH6
	 z=SMALLEST_CTZ(work_bitgraph[i]);
	 k=SECONDSMALLEST_CTZ(work_bitgraph[i]);
	 DELBIT(marks,i);
	 j=SMALLEST_CTZ(work_bitgraph[z]&marks);
	 u=SMALLEST_CTZ(work_bitgraph[k]&marks);
	 marks &= ~(work_bitgraph[z] | work_bitgraph[k] | work_bitgraph[i] | work_bitgraph[u]); 
	 for (rest-=6; (work_bitgraph[j]&marks) != EMPTYSET ; )
#else
#ifdef GIRTH5
	 z=SMALLEST_CTZ(work_bitgraph[i]);
	 k=SECONDSMALLEST_CTZ(work_bitgraph[i]);
	 j=SMALLEST_CTZ(work_bitgraph[z]&(~bit(i)));
	 marks &= ~(work_bitgraph[z] | work_bitgraph[k] | work_bitgraph[i]); 
	 for (rest-=5; (work_bitgraph[j]&marks) != EMPTYSET ; )
#elif defined(GIRTH4)
	 j=SMALLEST_CTZ(work_bitgraph[i]);
	 k=SECONDSMALLEST_CTZ(work_bitgraph[i]);		
	 marks &= ~(work_bitgraph[i] | work_bitgraph[k]); // also here girth at least 4 is used
	 for (rest-=4; (work_bitgraph[j]&marks) != EMPTYSET ; )
#endif
#endif
#endif	   
	   { j=SMALLEST_CTZ(work_bitgraph[j]&marks); 
	     DELBIT(marks,j);
	     rest--; if (rest<girth) { rest=0; break; }
	     }
       }

     length=oldrest-rest;

      if (length&1)
	{
	  set[oddones]=oldmarks & (~marks);
	  oddones++;
	  SETBIT2(lset_o,length);
	   if (lset_o>refset_o)  { return 1; }
	   if ((oddones>2) && rest && (lset_o+bit2(rest))<refset_o) { return 0; }
	}
      else
	{
	  if ((oddones>2) && (lset_o!=EMPTYSET) && rest &&  ((lset_o+bit2(rest))<refset_o))  { return 0; }
	  SETBIT2(lset_e,length);
	}
    }

 if (lset_o==EMPTYSET)  {return 1; }

 if (oddones==2)
   {
     FORALLELEMENTS(set[0] & globalwithdeg3, i) if (bitgraph[i]&set[1]) return 1;
     FORALLELEMENTS(set[0] & globalwithdeg2,i)  { allowedstrong[ne][i] &= (~set[1]); if ((globalwithdeg2 & allowedstrong[ne][i])==EMPTYSET) return 1; }
     FORALLELEMENTS(set[1] & globalwithdeg2,i)   { allowedstrong[ne][i] &= (~set[0]); if ((globalwithdeg2 & allowedstrong[ne][i])==EMPTYSET) return 1; }
   }
 
 if (lset_o<refset_o)  { return 0; }
 if (lset_e>refset_e) { return 1; }
 if (lset_e<refset_e) { return 0; }
 
 if (!fc_nostore) store_twofactor(ne,work_bitgraph);

 return 0;
}

/*********************************WEITERMATCHEN***********************/

int twofactor_level()
// work_bitgraph is the 2-factor just found (the matching is removed). Returns the largest edgelevel of an inserted edge in it,
// not counting the edge {fc_v,fc_w} that is tested. 0 if the 2-factor contains no other inserted edge.
{
  int x, l=0;
  LONGTYPE s;

  for (s=globalwithdeg3 & ~(bit(fc_v)|bit(fc_w)); s; s&=s-1)
    { x=SMALLEST_CTZ(s);
      if ((edgelevel[x]>l) && IS_SET(work_bitgraph[x],representation[x])) l=edgelevel[x]; }
  return l;
}

void fc_store_bad(int v, int w, int from, int to)
// the pair {v,w} is forbidden on the levels from...to of the current path
{
  int j;

  if (from<to) { fc_lifted++; fc_liftdist[to-from]++; }
  for (j=from; j<=to; j++) { SETBIT(fc_bad[j][v],w); SETBIT(fc_bad[j][w],v); }
}

int weitermatchen(LONGTYPE necessary, LONGTYPE nb1, LONGTYPE nb2, LONGTYPE nb3)

/* versucht das matching auszubauen. Wenn es maximal ist und alle "forced"
   Knoten saturiert sind, wird die Entscheidung darauf reduziert, ob der
   Graph ohne dieses Matching faerbbar ist. Gibt 1 zurueck, wenn das bisherige
   Matching zu einer Faerbung fortsetzbar ist, 0 sonst. */

/* Iterative version (Claude): the same search in the same order as the former recursive version -- also the 2-factors are
   passed to (*endtestf)() in the same order. Only the vertices with two possible partners (nb2) get a frame on the stack;
   a vertex with only one possible partner (nb1) is matched in the loop. All edges removed from work_bitgraph are on an
   edge stack, so that they can be put back when the search returns to a frame. */
{
  struct { LONGTYPE necessary, nb1, nb2, nb3, neighb; int start, round, height; } frame[KNOTEN/2+2];
  unsigned char edgestart[KNOTEN/2+2], edgeziel[KNOTEN/2+2];
  LONGTYPE oldstart[KNOTEN/2+2], oldziel[KNOTEN/2+2]; // the rows of start and ziel before the edge was removed
  int top= -1, height=0, start, ziel;
  LONGTYPE dummy, dummy2, dummy3, localnb1, localnb2, localnb3, ws, wz;

  for (;;) // one iteration matches one vertex -- like one call of the recursive version
    {
      if (nb1!=EMPTYSET)
	{
	  start=SMALLEST_CTZ(nb1);
	  ws=work_bitgraph[start];
	  ziel=SMALLEST_CTZ(necessary&ws);
	}
      else
	{
	  if (nb2==EMPTYSET) { while (height>0) { height--; work_bitgraph[edgestart[height]]=oldstart[height]; work_bitgraph[edgeziel[height]]=oldziel[height]; } return 1; } // This can happen, but only if the graph is not connected.
	  start=SMALLEST_CTZ(nb2);
	  ws=work_bitgraph[start];
	  top++;
	  frame[top].necessary=necessary; frame[top].nb1=nb1; frame[top].nb2=nb2; frame[top].nb3=nb3;
	  frame[top].neighb=(necessary&ws); frame[top].start=start; frame[top].round=0; frame[top].height=height;
	  ziel=SMALLEST_CTZ(necessary&ws);
	}
      necessary &= (~(bit(start)|bit(ziel)));

    match_edge: // start and ziel are matched, necessary is already updated, nb1..nb3 are those of the current "call"
      // ws is work_bitgraph[start] before the edge is removed
      wz=work_bitgraph[ziel];
      work_bitgraph[start]=ws&(~bit(ziel)); work_bitgraph[ziel]=wz&(~bit(start));
      edgestart[height]=start; edgeziel[height]=ziel; oldstart[height]=ws; oldziel[height]=wz; height++;
      if (necessary==EMPTYSET)
	{ if ((*endtestf)()) { if (fc_wantlevel) fc_level=twofactor_level(); while (height>0) { height--; work_bitgraph[edgestart[height]]=oldstart[height]; work_bitgraph[edgeziel[height]]=oldziel[height]; } return 1; } }
      else
	{
	  // start and ziel are not in necessary, so the bits of start and ziel in dummy do not matter: all results are intersected with necessary
	  dummy=ws | wz;
	  dummy3= nb3 & dummy;
	  localnb3 =  nb3 & (~dummy3) & necessary;
	  dummy2= nb2 & dummy;
	  localnb2 = (nb2 | dummy3) & (~dummy2) & necessary;
	  if ((nb1 & dummy & necessary) == EMPTYSET)
	    {
	      localnb1 = (nb1 | dummy2) & necessary;
	      nb1=localnb1; nb2=localnb2; nb3=localnb3;
	      continue;
	    }
	}

      // backtrack to the last vertex with an untried second partner
      for (;;)
	{
	  if (top<0)
	    { while (height>0) { height--; work_bitgraph[edgestart[height]]=oldstart[height]; work_bitgraph[edgeziel[height]]=oldziel[height]; }
	      return 0; }
	  while (height>frame[top].height) { height--; work_bitgraph[edgestart[height]]=oldstart[height]; work_bitgraph[edgeziel[height]]=oldziel[height]; }
	  if (frame[top].round==0) break;
	  top--;
	}
      frame[top].round=1;
      start=frame[top].start;
      ws=work_bitgraph[start];
      ziel=SMALLEST_CTZ(frame[top].neighb & (frame[top].neighb-1));
      necessary= frame[top].necessary & (~(bit(start)|bit(ziel)));
      nb1=frame[top].nb1; nb2=frame[top].nb2; nb3=frame[top].nb3;
      goto match_edge;
    }
}


/**************************CONTRADICTING_TWOFAC****************************/

int contradicting_twofac(int v, int w, LONGTYPE necessary)
// hat der Graph nachdem gerade die Kante v,w zugefuegt wurde einen 2-Faktor durch diese Kante oder einen 2-Faktor der leicht zu detektieren ist als
// "kanonischer" als der zur Konstruktion benutzte?
// in necessary, the vertices v,w must already be included

{
  int v1,v2;
  LONGTYPE dummy, *deg3nb, *deg3nb_old, nb1, nb2, nb3, dummy3, dummy2;

  /* nb1,nb2,nb3: toppen met graad 3 die 1,2,3 buren hebben met graad 3. For readability. It can also be inserted directly into the call of the function -- hopefully
   the optimizer does it.*/

/*Solange der Graph nicht 3-regulaer ist, kann man nicht eine Kante fest waehlen -- aber man weiss, dass {v,w}
   nicht in dem complementairen matching liegen darf. */

  // work_bitgraph is equal to bitgraph here -- weitermatchen() restores it
  
  deg3nb_old=deg3nb_global[ne-1];
  deg3nb=deg3nb_global[ne];
  dummy= work_bitgraph[v] | work_bitgraph[w];
  dummy2= dummy & deg3nb_old[1];
  deg3nb[1]= deg3nb_old[1] & (~dummy2);
  dummy3= dummy & deg3nb_old[2];
  deg3nb[2]= (deg3nb_old[2] & (~dummy3)) | dummy2;
  deg3nb[3]= deg3nb_old[3] | dummy3;
  
  SETBIT(deg3nb[POPC(necessary&work_bitgraph[v])],v);
  SETBIT(deg3nb[POPC(necessary&work_bitgraph[w])],w);

  if (fc_restrict) // the edge {fc_ea,fc_eb} is not in work_bitgraph: both ends have one possible partner less
    { int x;
      LONGTYPE b2, b3;
      for (x=fc_ea; ; x=fc_eb)
	{ // x is in exactly one of deg3nb[1..3]: it is moved to the next lower set (without branches)
	  if (IS_SET(deg3nb[1],x)) return 0; // x can not be matched
	  b2=deg3nb[2]&bit(x); b3=deg3nb[3]&bit(x);
	  if ((b2|b3)==EMPTYSET) { fprintf(stderr,"Error in contradicting_twofac(): vertex %d of degree 3 not found\n",x); exit(1); }
	  deg3nb[1]|=b2; deg3nb[2]=(deg3nb[2]^b2)|b3; deg3nb[3]^=b3;
	  if (x==fc_eb) break; }
    }
  
  dummy= (work_bitgraph[v]&(~bit(w)));

  v1=SMALLEST_CTZ(dummy);
  v2=SECONDSMALLEST_CTZ(dummy);

  DELBIT(necessary,v);

 // Eerst:
     
  if (IS_SET(necessary,v1))
   {
     DEL_EDGEBG(work_bitgraph,v,v1);
     DELBIT(necessary,v1);
     /* belangrijk: girth>=4 => v en v1 hebben geen gemeenschappelijke buren! */
     dummy=work_bitgraph[v] | work_bitgraph[v1];
     dummy3=deg3nb[3] & dummy;
     nb3 =  deg3nb[3] & (~dummy3) & necessary;
     dummy2=deg3nb[2]&dummy;
     nb2 = (deg3nb[2] | dummy3) & (~dummy2) & necessary;
     if ((deg3nb[1] & dummy & necessary)==EMPTYSET)
       {
	 nb1 = (deg3nb[1] | dummy2) & necessary;
	 /* jetzt hat man sich fuer die erste Kante entschieden, die man festlegen will. */
	 if (weitermatchen(necessary,nb1,nb2,nb3)) { ADD_EDGEBG(work_bitgraph,v,v1); return 1; }
       }
     ADD_EDGEBG(work_bitgraph,v,v1);
     SETBIT(necessary,v1);
   }

// Dan:
 if (IS_SET(necessary,v2))
   {
     DEL_EDGEBG(work_bitgraph,v,v2);
     DELBIT(necessary,v2);
     /* belangrijk: girth>=4 => v en v2 hebben geen gemeenschappelijke buren! */
     dummy=work_bitgraph[v] | work_bitgraph[v2];
     dummy3=deg3nb[3] & dummy;
     nb3 =  deg3nb[3] & (~dummy3) & necessary;
     dummy2=deg3nb[2]&dummy;
     nb2 = (deg3nb[2] | dummy3) & (~dummy2) & necessary;
     if ((deg3nb[1] & dummy & necessary) != EMPTYSET) { ADD_EDGEBG(work_bitgraph,v,v2); return 0; }
     nb1 = (deg3nb[1] | dummy2) & necessary;
     /* jetzt hat man sich fuer die erste Kante entschieden, die man festlegen will. */
     if (weitermatchen(necessary,nb1,nb2,nb3)) { ADD_EDGEBG(work_bitgraph,v,v2); return 1; }
     ADD_EDGEBG(work_bitgraph,v,v2);
     //SETBIT(necessary,v2);
   }

 return 0;
}


int endtest_plus_e()
// the edge {fc_ea,fc_eb} belongs to the 2-factor
{
  int r;

  ADD_EDGEBG(work_bitgraph,fc_ea,fc_eb);
  r=(*fc_saved_endtest)();
  DEL_EDGEBG(work_bitgraph,fc_ea,fc_eb);
  return r;
}

int contradicting_twofac_through(int v, int w, LONGTYPE necessary, int a, int b)
// like contradicting_twofac(v,w,necessary), but only 2-factors that also contain the edge {a,b} are considered
{
  int r;

  DEL_EDGEBG(work_bitgraph,a,b);
  fc_ea=a; fc_eb=b; fc_restrict=1;
  fc_saved_endtest=endtestf; endtestf=endtest_plus_e;
  r=contradicting_twofac(v,w,necessary);
  endtestf=fc_saved_endtest; fc_restrict=0;
  ADD_EDGEBG(work_bitgraph,a,b);
  return r;
}

void set_deg3nb(int v, int w, LONGTYPE necessary)
// the update of deg3nb_global[ne] done at the start of contradicting_twofac() -- for an edge {v,w} that is not tested
// itself (option P)
{
  LONGTYPE dummy, *deg3nb, *deg3nb_old, dummy3, dummy2;

  deg3nb_old=deg3nb_global[ne-1];
  deg3nb=deg3nb_global[ne];
  dummy= work_bitgraph[v] | work_bitgraph[w];
  dummy2= dummy & deg3nb_old[1];
  deg3nb[1]= deg3nb_old[1] & (~dummy2);
  dummy3= dummy & deg3nb_old[2];
  deg3nb[2]= (deg3nb_old[2] & (~dummy3)) | dummy2;
  deg3nb[3]= deg3nb_old[3] | dummy3;
  SETBIT(deg3nb[POPC(necessary&work_bitgraph[v])],v);
  SETBIT(deg3nb[POPC(necessary&work_bitgraph[w])],w);
}

int comparerep()
{
  int altrep[KNOTEN]={10,12,14,11,16,17};
  int len=5,i;
 
  
  for (i=0;i<len;i++) if (altrep[i]!=representation[i]) return 0;
  return 1;
  }

int rek_cutcombinations(int min, int max, LONGTYPE set, LONGTYPE out)
// returns 1 if a combination of the cycles 0...cumcycles-1 in the beginnning has fewer outgoing edges than mincon
{
  int i;

  if (min==max)
    { if (POPC(out & (~set))<mincon) return 1;
      return 0;
    }

  for (i=min;i<=max;i++) // max: go to test
    {
      if (rek_cutcombinations(i+1,max,set | setcycle[i], out | outgoing[i])) return 1;
    }
  return 0;
}

// The following function is not used as it doesn't seem to speed up things though it does not cost much and
// sometimes bounds the construction
int cut_combinations(int numcycles)
// returns 1 if a combination of the cycles 0...cumcycles-1 in the beginnning has fewer outgoing edges than mincon
{
  LONGTYPE set, out;
  int i;

  for (i=0;i<numcycles-1;i++) // the single set of cycles "numcycles-1" has already been tested
    // others too that will be tested again, but well...
    {
      set=setcycle[i]; out=outgoing[i];
      if (rek_cutcombinations(i+1,numcycles,set,out)) return 1;
    }
  return 0;
}

int put_outside(int top, LONGTYPE *outside, LONGTYPE set)
{
  int top2;
  LONGTYPE nbnotout, nbset;

  SETBIT(*outside, top);

  nbnotout= (bitgraph[top]&(~(*outside))); //neighbours of top that are not yet in the set outside

  nbset=nbnotout&set; // neighbours of top that are in "set"

  if (nbset!=EMPTYSET) // can contain only one element
    {
      if (ATLEAST2BIT(bitgraph[SMALLEST(nbset)]&(*outside))) { return 1; }  // contradiction: element in set with now two outside neighbours
      nbnotout=nbnotout & (~set); // zero, one or two still undetermined neighbours of top
      if (nbnotout!=EMPTYSET)
	{
	  top2=SMALLEST(nbnotout);
	  if (ATLEAST2BIT(bitgraph[top2]&(*outside)) && put_outside(top2,outside,set)) return 1;
	  nbnotout= (nbnotout & (nbnotout-1) & (~(*outside))); // the remaining element can have been added in the recursion
	  if (nbnotout!=EMPTYSET)
	    { top2=SMALLEST(nbnotout);
	      if (ATLEAST2BIT(bitgraph[top2]&(*outside)) && put_outside(top2,outside,set)) return 1;
	    }
	}
    }
  else // all in principle 0 to 3 neighbours undetermined. There seem to be only 0 or 1, but that relies on an order of edges one shouldn't rely on
    if (nbnotout!=EMPTYSET)
      {
	top2=SMALLEST(nbnotout);
	if (ATLEAST2BIT(bitgraph[top2]&(*outside)) && put_outside(top2,outside,set)) return 1;
	nbnotout= (nbnotout & (nbnotout-1) & (~(*outside)));
	if (nbnotout!=EMPTYSET)
	  {
	    top2=SMALLEST(nbnotout);
	    if (ATLEAST2BIT(bitgraph[top2]&(*outside)) && put_outside(top2,outside,set)) return 1;
	    nbnotout= (nbnotout & (nbnotout-1) & (~(*outside)));
	    if (nbnotout!=EMPTYSET)
	      {
		top2=SMALLEST(nbnotout);
		if (ATLEAST2BIT(bitgraph[top2]&(*outside)) && put_outside(top2,outside,set)) return 1;
		nbnotout= (nbnotout & (nbnotout-1) & (~(*outside)));
	      }
	  }
      }
  return 0;
}

int put_inside(int top, LONGTYPE outside, LONGTYPE *set, LONGTYPE *allneighbours, int *outgoing, int *size)
// top has one or two neighbours in set
// returns 1 if there is a contradiction with minimality
{
  int top2;
  LONGTYPE nbnotinset, nbout;

  //if (*size>nvh) return 1;
 
   *allneighbours |= bitgraph[top];
   SETBIT(*set, top);
   (*size)++;

    nbnotinset= bitgraph[top]&(~(*set));

   if (nbnotinset==EMPTYSET) { *outgoing -= 3; return 0; }
   nbout= nbnotinset & outside; // set of neighbours in outside

   if (ATLEAST2BIT(nbnotinset)) // so one in set 
     {
       *outgoing += 1;

         if (nbout != EMPTYSET) // can be only size 0 or 1, so in this case 1
	 { if (ATLEAST2BIT(bitgraph[SMALLEST(nbout)]&(*set))) { return 1; } // contradiction: element in outside set with two neighbours in set
	   // the other one is then still undecided and must be in set
	   if (put_inside(SMALLEST(nbnotinset & (~nbout)),outside,set,allneighbours,outgoing,size)) return 1;
	 }
       // otherwise there are two undetermined neighbours and at least one must be in the set, but in this function only forced consequences are implemented
       else
	 {
	   top2=SMALLEST(nbnotinset);
	   if (ATLEAST2BIT(bitgraph[top2]&(*set)) && put_inside(top2,outside,set,allneighbours,outgoing,size)) return 1;
	   
	   // the other vertex might have been added in the previous recursion
	   top2= SMALLEST(nbnotinset & (nbnotinset-1));
	   if (NOT_SET(*set,top2) && ATLEAST2BIT(bitgraph[top2]&(*set)) && put_inside(top2,outside,set,allneighbours,outgoing,size)) return 1;
	 }
     }
   else // only one neighbour not in set, so two in set

     {
     *outgoing -= 1;
     if (nbout==EMPTYSET)
       {
	 top2=SMALLEST(nbnotinset);
	 if (ATLEAST2BIT(bitgraph[top2]&(*set)) && put_inside(top2,outside,set,allneighbours,outgoing,size)) return 1;
       }
     }
   return 0;
}


int cycl_con_rek(LONGTYPE set, LONGTYPE allneighbours, LONGTYPE outside, int numberout, int size)
// outside are vertices that are known to be not in the set
// whenever this is called, there are no vertices not in outside having 2 neighbours in outside and no vertices not in set
// having two neighbours in set
{
  int top, buffersize, buffernbout;
  LONGTYPE _gbu, directnb;
  LONGTYPE bufferset, bufferallneighbours;

  //if (size>=(nvh))
  //{ if (numberout<mincon) return 1; else return 0; }

  if (POPC(allneighbours & outside)>=cutlimit) return 0;

  directnb=(allneighbours & (~(set | outside)));

  FORALLELEMENTS(directnb, top)
    if (NOT_SET(outside,top))
      {
	bufferset=set; bufferallneighbours=allneighbours; buffernbout=numberout; buffersize=size;
	if (put_inside( top, outside, &bufferset, &bufferallneighbours, &buffernbout, &buffersize)==0) // no contradiction to minimality of set
	  {
	    if ((buffernbout<cutlimit) && (buffersize>=minside) &&  (nv-buffersize>=minside)) { witness_set=bufferset; return 1; }
	    // the lst size test is not necessary, but looks safe :-)
	    if (cycl_con_rek(bufferset, bufferallneighbours, outside, buffernbout, buffersize)) return 1;
	  }
	if (put_outside(top, &outside, set)) return 0;
      }
  return 0;
}


int cycl_con()
// must be called only when the graph is cubic -- that is: no deg 2 vertices left
  
// checks whether a connected set with fewer than mincon outgoing edges containing vertex nv-1 exists.
// Seems to be faster than restricting the size to n/2 and iterating over all starting points.
// simple -- think about better algorithm if necessary
// A minimal cut has one set with at most n/2 elements. This set has no vertex with two neighbours outside the set -- otherwise it would not be a minimal cut.
// On the other hand, the same is true for the vertices outside the set -- for the same reason.
// So each vertex in one of the sets has at least two neighbours in the same set.
// It turned out not to be useful to test graphs early -- the cost exceeded the benefit though one could bound the construction.
// Most likely graphs with early cuts were also rejected by other criteria soon afterwards.
// So it is only called at the end and then the largest edge necessary for this cut is stored and the construction backtracks until this edge is removed
{
  cutlimit=mincon; minside=mincon-1;
  return cycl_con_rek(bit(nv-1), bitgraph[nv-1] , EMPTYSET, 3, 1);
}

int last_inner_edge(LONGTYPE set)
// the largest edgelevel of a non-2-factor edge with both ends in set -- 0 if there is none
{
  int v, l=0;
  LONGTYPE _gbu;

  FORALLELEMENTS(set,v)
    if ((edgelevel[v]>l) && IS_SET(set,SMALLEST(bitgraph[v]&(~initialgraph[v])))) l=edgelevel[v];
  return l;
}

int nontriv5_final()
// must be called only when the graph is cubic. Returns 1 if the graph has a cyclic cut with at most 4 edges or a nontrivial cyclic 5-cut.
// In that case also jumplevel is set if backjumping is used.
// If a side of a cut with at most 5 edges has at least 6 vertices, it has at least 7 vertices (girth>=5) and contains a cycle.
// With minimal cut size such a cut is 2-closed and both sides are connected, so the search of cycl_con_rek() is complete.
{
  int l, ls, lt, size;

  final_tested++;
  cutlimit=6; minside=6;
  if (!cycl_con_rek(bit(nv-1), bitgraph[nv-1] , EMPTYSET, 3, 1)) return 0;
  final_rejected++;
  size=POPC(witness_set);
  sidehist[MIN(size,nv-size)]++;
  ls=last_inner_edge(witness_set);
  lt=last_inner_edge(allvertices & ~witness_set);
  // with E the edges inside the side X that do not belong to the initial 2-factor, X already has an H-cut with at most 5 edges
  // (infinity on the other side) after the last edge of E has been inserted. All later edges have at most one end in X.
  l=MIN(ls,lt);
  jumphist[l]++;
  if (use_backjump)
    {
      if (l<splitlevel) l=splitlevel; // otherwise the partition into parts with option m would not be consistent
      jumplevel=l;
    }
  return 1;
}

int doomed(LONGTYPE set, LONGTYPE deg2)
// tests whether set (closed under adding vertices with two neighbours in it) has an H-cut with at most 5 edges and sides with at least 6 vertices
{
  int v, cut, cutinf, changed;
  LONGTYPE _gbu, nbs;

  do
    {
      changed=0;
      nbs=EMPTYSET;
      FORALLELEMENTS(set,v) nbs|=bitgraph[v];
      nbs&= ~set;
      FORALLELEMENTS(nbs,v) if (ATLEAST2BIT(bitgraph[v]&set)) { SETBIT(set,v); changed=1; }
    }
  while (changed);

  v=POPC(set);
  if ((v<6) || (nv-v<6)) return 0;
  cut=0;
  FORALLELEMENTS(set,v) cut+=POPC(bitgraph[v]&(~set));
  cutinf=cut+POPC(deg2&(~set)); // infinity in set
  cut+=POPC(deg2&set); // infinity outside
  return ((cut<=5) || (cutinf<=5));
}

int h_out(HSTATE *st, int v);

int h_in(HSTATE *st, int v)
// puts real vertex v into the set and applies the closure rules. Returns 1 if a minimal new witness is impossible in this branch.
{
  int w;
  LONGTYPE nb, rest;

  nb=HNB(v);
  SETBIT(st->set,v); st->size++;
  st->cut+= 3-2*POPC(nb&st->set);
  st->lb+= POPC(nb&st->out);
  if (ATLEAST2BIT(nb&st->out)) return 1;
  nb=bitgraph[v]; // only real neighbours from now on
  rest=nb&st->out;
  while (rest) { w=SMALLEST_CTZ(rest); rest&=rest-1; if (ATLEAST2BIT(HNB(w)&st->set)) return 1; }
  rest=nb&~(st->set|st->out);
  while (rest) { w=SMALLEST_CTZ(rest); rest&=rest-1;
    if (NOT_SET(st->set,w) && ATLEAST2BIT(HNB(w)&st->set) && h_in(st,w)) return 1; }
  return 0;
}

int h_out(HSTATE *st, int v)
// puts real vertex v into out and applies the closure rules
{
  int w;
  LONGTYPE nb, rest;

  nb=HNB(v);
  SETBIT(st->out,v);
  st->lb+= POPC(nb&st->set);
  if (ATLEAST2BIT(nb&st->set)) return 1;
  nb=bitgraph[v];
  rest=nb&st->set;
  while (rest) { w=SMALLEST_CTZ(rest); rest&=rest-1; if (ATLEAST2BIT(HNB(w)&st->out)) return 1; }
  rest=nb&~(st->set|st->out);
  while (rest) { w=SMALLEST_CTZ(rest); rest&=rest-1;
    if (NOT_SET(st->out,w) && ATLEAST2BIT(HNB(w)&st->out) && h_out(st,w)) return 1; }
  return 0;
}

int h_flow(LONGTYPE src, LONGTYPE snk, int limit)
// maximum number (but at most limit) of edge disjoint paths in H from src to snk -- every cut separating them has at least that many edges
{
  LONGTYPE fl[KNOTEN]={EMPTYSET}, visited, cand;
  int queue[KNOTEN], parent[KNOTEN], head, tail, u, v, w, flow;

  for (flow=0; flow<limit; flow++)
    {
      visited=src; tail=0; v= -1;
      cand=src; while (cand) { queue[tail++]=SMALLEST_CTZ(cand); cand&=cand-1; }
      for (head=0; (head<tail) && (v<0); head++)
	{
	  u=queue[head];
	  cand= ((u==nv) ? hdeg2 : HNB(u)) & ~fl[u] & ~visited; // residual capacity u->v unless one unit already flows u->v
	  while (cand)
	    { w=SMALLEST_CTZ(cand); cand&=cand-1;
	      parent[w]=u; SETBIT(visited,w);
	      if (IS_SET(snk,w)) { v=w; break; }
	      queue[tail++]=w;
	    }
	}
      if (v<0) return flow;
      for ( ; NOT_SET(src,v); v=u)
	{ u=parent[v];
	  if (IS_SET(fl[v],u)) DELBIT(fl[v],u); else SETBIT(fl[u],v); }
    }
  return flow;
}

int h_rek(HSTATE *st)
{
  int v;
  LONGTYPE nbs, _gbu;
  HSTATE buffer;

  if (st->lb>=6) return 0;
  if ((st->cut<=5) && (st->size>=6) && (nv-st->size>=6)) { witness_set=st->set; return 1; }
  if (st->size>nv-6) return 0;
  if (h_flow(st->set,st->out,6)>=6) return 0;
  nbs=EMPTYSET;
  FORALLELEMENTS(st->set,v) nbs|=bitgraph[v];
  nbs&= ~(st->set|st->out);
  if (nbs==EMPTYSET) return 0;
  v=SMALLEST_CTZ(nbs);
  buffer= *st;
  if (!h_in(&buffer,v) && h_rek(&buffer)) return 1;
  if (h_out(st,v)) return 0;
  return h_rek(st);
}

int h_doomed(int a, int b, LONGTYPE withdeg3)
// edge {a,b} has just been inserted. Searches for a set S containing a and b with an H-cut of at most 5 edges (infinity outside S)
// and both sides with at least 6 vertices. If such a set exists, every completion has a cyclic <=4-cut or a nontrivial cyclic 5-cut.
// If there was no such set before the edge was inserted, every new one contains a and b, and one with minimal cut is 2-closed,
// so the search is complete in that case.
{
  HSTATE st;
  int v, cut;
  LONGTYPE _gbu;

  if ((!nontriv5) || (ne<earlylevel)) return 0;
  htests++;
  hdeg2=allvertices & ~withdeg3;
  infbit=bit(nv);
  st.set=EMPTYSET; st.out=infbit; st.cut=st.lb=st.size=0;
  if (h_in(&st,a)) return 0;
  if (NOT_SET(st.set,b) && h_in(&st,b)) return 0;
  if (!h_rek(&st)) return 0;
  // independent check of the witness
  cut=POPC(witness_set&hdeg2);
  FORALLELEMENTS(witness_set,v) cut+=POPC(bitgraph[v]&(~witness_set));
  v=POPC(witness_set);
  if ((cut>5) || (v<6) || (nv-v<6)) { fprintf(stderr,"Error in h_doomed() -- exit.\n"); exit(1); }
  earlyrej[4]++;
  return 1;
}

int h7_doomed(int a, int b)
// Edge {a,b} has just been inserted. A set with 7 vertices and at most 5 edges leaving it in H (girth>=5) induces two 5-cycles sharing
// exactly a path of length 2 -- and each of its 8 edges lies on a 5-cycle. So it is sufficient to check the 5-cycles through {a,b}.
// The set has an H-cut of exactly 21-2*8=5 edges, so if nv>=13, every completion has a nontrivial cyclic 5-cut or a smaller cyclic cut.
{
  int x, z, y, k, cyc[5];
  LONGTYPE xs, zs, ys, cset, p, q;

  if ((!nontriv5) || (nv<13)) return 0;
  xs=bitgraph[b] & ~bit(a);
  while (xs)
    { x=SMALLEST_CTZ(xs); xs&=xs-1;
      zs=bitgraph[a] & ~bit(b);
      while (zs)
	{ z=SMALLEST_CTZ(zs); zs&=zs-1;
	  ys=bitgraph[x] & bitgraph[z] & ~(bit(a)|bit(b));
	  while (ys)
	    { y=SMALLEST_CTZ(ys); ys&=ys-1;
	      cyc[0]=a; cyc[1]=b; cyc[2]=x; cyc[3]=y; cyc[4]=z;
	      cset=bit(a)|bit(b)|bit(x)|bit(y)|bit(z);
	      for (k=0;k<5;k++)
		{ // second 5-cycle through the path cyc[k-1],cyc[k],cyc[k+1] and two vertices not on this cycle
		  p=bitgraph[cyc[(k+4)%5]] & ~cset; q=bitgraph[cyc[(k+1)%5]] & ~cset;
		  if (p && q && (bitgraph[SMALLEST_CTZ(p)] & q)) { earlyrej[5]++; return 1; }
		}
	    }
	}
    }
  return 0;
}

LONGTYPE getverticesatdistance(int v, int d)
// returns the set of vertices at distance d
{
  LONGTYPE set, seen;
  int list[KNOTEN],dist[KNOTEN], run,i, ll;

  set=seen=bit(v); list[0]=v, dist[0]=0; ll=1;

  for (run=0; dist[run]<d; run++)
    {
      FORALLELEMENTS(bitgraph[list[run]] & (~seen),i)
	{ SETBIT(set,i); list[ll]=i; dist[ll]=dist[run]+1; ll++; }
    }

  return set;
}

// Claude: inlined version of (*set_OK_distances_pointer)(v) for the common cases (same result)
#ifdef GIRTH4
#define OKDIST(mode,v) ((mode)==5 ? (allvertices&(~dist3bitgraph[v])) : (mode)==4 ? (allvertices&(~dist2bitgraph[v])) : (*set_OK_distances_pointer)(v))
#define OKDIST_MODE ((set_OK_distances_pointer==(&set_OK_distances_5)) ? 5 : (set_OK_distances_pointer==(&set_OK_distances_4)) ? 4 : 0)
#else
#define OKDIST(mode,v) ((mode)==5 ? (allvertices&(~dist3bitgraph[v])) : (*set_OK_distances_pointer)(v))
#define OKDIST_MODE ((set_OK_distances_pointer==(&set_OK_distances_5)) ? 5 : 0)
#endif


/* Claude: option K -- see the header. */
int mrvlevel=KNOTEN; // from edge number mrvlevel on construct_mrv() is used
unsigned long mrv_nodes=0;
LONGTYPE fc_okpairs[(KNOTEN>>1)+1][KNOTEN]; // fc_okpairs[l][v]: the partners of v accepted on level l
#define BR_W1 4 // weights of the choice of the vertex in construct_mrv()
#define BR_W2 2
#define BR_W3 (-1)

/* Claude: option P -- see the header. */
int pairlevel=0; // option P: the two-edge test is used if at most pairlevel vertices have degree 2
#define TE_PA 2 // weights of the choice of the vertex in te_search()
#define TE_PB 1
#define TWOEDGE_BUDGET 5000 // twoedge_test() gives up (and keeps the node) after this many search nodes
#define TE_WORDS 4 // at most 64*TE_WORDS candidate edges
typedef struct { unsigned long long w[TE_WORDS]; } edgeset;
unsigned long te_calls=0, te_prune=0, te_gaveup=0, te_searchnodes=0, te_pairtests=0;
long te_budget;
LONGTYPE te_withdeg3;
unsigned char te_id[KNOTEN][KNOTEN]; // the number of the candidate edge {v,w}
int te_m, te_v[64*TE_WORDS], te_w[64*TE_WORDS]; // the candidate edges
edgeset te_bad[64*TE_WORDS], te_ok[64*TE_WORDS], te_chosen; // te_bad[e] (te_ok[e]): edges known to form (no) bad pair with e

int bad_edgepair(int a, int b, int v, int w)
// 1 if the current graph plus the edges {a,b} and {v,w} has a contradicting 2-factor containing both edges
{
  int r;
  LONGTYPE nec=te_withdeg3|bit(a)|bit(b), n4=nec|bit(v)|bit(w);

  te_pairtests++;
  // each of a, b, v, w must drop a 2-factor edge to a vertex of degree 3 (see construct_mrv_inner())
  if (!(initialgraph[a]&n4) || !(initialgraph[b]&n4) || !(initialgraph[v]&n4) || !(initialgraph[w]&n4)) return 0;

  ADD_EDGE(a,b); set_deg3nb(a,b,nec);
  ADD_EDGE(v,w); nec|=bit(v)|bit(w); globalwithdeg3=nec;
  r=contradicting_twofac_through(v,w,nec,a,b);
  DEL_EDGE(v,w); DEL_EDGE(a,b);
  return r;
}

int te_compatible(int e)
// 1 if the candidate edge e forms no bad pair with one of the chosen edges -- pairs not known yet are tested now
{
  int k, f;
  unsigned long long u;

  for (k=0; k<TE_WORDS; k++) if (te_bad[e].w[k] & te_chosen.w[k]) return 0;
  for (k=0; k<TE_WORDS; k++)
    for (u=te_chosen.w[k] & ~(te_bad[e].w[k]|te_ok[e].w[k]); u; u&=u-1)
      {
	f=(k<<6)|__builtin_ctzll(u);
	if (bad_edgepair(te_v[f],te_w[f],te_v[e],te_w[e]))
	  { te_bad[e].w[f>>6]|=1ULL<<(f&63); te_bad[f].w[e>>6]|=1ULL<<(e&63); return 0; }
	te_ok[e].w[f>>6]|=1ULL<<(f&63); te_ok[f].w[e>>6]|=1ULL<<(e&63);
      }
  return 1;
}

int te_search(LONGTYPE g[], LONGTYPE deg2, LONGTYPE dm[])
// 1 if the vertices in deg2 can be matched by edges from dm[] so that the girth (in g[]) is still at least girth and the
// new edges together with the chosen ones (te_chosen) contain no bad pair -- or if the budget is exhausted
{
  int v, w, x, r, R=girth-3, bv= -1, bc=INT_MAX, c, e, k;
  LONGTYPE s, t, b, f, nf, ba[8], bw[8], nd[KNOTEN], nd2;

  if (deg2==EMPTYSET) return 1;
  te_searchnodes++;
  if (--te_budget<0) return 1;
  // the vertex with the smallest key TE_PA*(possible partners)+TE_PB*(neighbours in the initial 2-factor that have degree 2
  // here): the new edges at a vertex with such neighbours are less likely to be part of a bad pair, so branching on it
  // tends to give larger search trees
  for (s=deg2; s; s&=s-1)
    { v=SMALLEST_CTZ(s); c=POPC(dm[v]&deg2); if (c==0) return 0;
      c=TE_PA*c+TE_PB*POPC(initialgraph[v]&deg2); if (c<bc) { bc=c; bv=v; } }
  b=f=ba[0]=bit(bv); // ba[k]: the vertices at distance at most k from bv
  for (k=1; k<=R; k++) { nf=EMPTYSET; for (t=f; t; t&=t-1) nf|=g[SMALLEST_CTZ(t)]; nf&=~b; b|=nf; f=nf; ba[k]=b; }
  for (s=dm[bv]&deg2; s; s&=s-1)
    {
      w=SMALLEST_CTZ(s);
      e=te_id[bv][w];
      if (!te_compatible(e)) continue;
      b=f=bw[0]=bit(w);
      for (k=1; k<=R; k++) { nf=EMPTYSET; for (t=f; t; t&=t-1) nf|=g[SMALLEST_CTZ(t)]; nf&=~b; b|=nf; f=nf; bw[k]=b; }
      nd2=deg2&~(bit(bv)|bit(w));
      for (t=nd2; t; t&=t-1) { x=SMALLEST_CTZ(t); nd[x]=dm[x]&nd2; }
      // as in far_update(): {x,y} gets distance at most girth-2 iff d(x,bv)+d(w,y)<=girth-3 or d(x,w)+d(bv,y)<=girth-3
      // (the balls are nested, so only the exact distance of x to bv resp. w matters)
      for (r=1; r<=R; r++)
	{ for (t=(ba[r]&~ba[r-1])&nd2; t; t&=t-1) { x=SMALLEST_CTZ(t); nd[x]&=~bw[R-r]; }
	  for (t=(bw[r]&~bw[r-1])&nd2; t; t&=t-1) { x=SMALLEST_CTZ(t); nd[x]&=~ba[R-r]; } }
      SETBIT(g[bv],w); SETBIT(g[w],bv); te_chosen.w[e>>6]|=1ULL<<(e&63);
      c=te_search(g,nd2,nd);
      te_chosen.w[e>>6]&=~(1ULL<<(e&63)); DELBIT(g[bv],w); DELBIT(g[w],bv);
      if (c) return 1;
    }
  return 0;
}

int twoedge_test(LONGTYPE withdeg3, LONGTYPE deg2, LONGTYPE dm[])
// option P: returns 0 if every completion of the current graph that keeps the girth contains a forbidden pair (dm[v] are
// the possible partners of v) or a bad pair of edges -- then no snark can be constructed from the current graph
{
  int v, w, r;
  LONGTYPE g[KNOTEN], s, t;

  te_calls++;
  te_m=0;
  for (s=deg2; s; s&=s-1)
    { v=SMALLEST_CTZ(s);
      for (t=dm[v]&deg2&~((bit(v)<<1)-1); t; t&=t-1)
	{ w=SMALLEST_CTZ(t);
	  if (te_m==64*TE_WORDS) return 1; // too many candidate edges -- no test
	  te_id[v][w]=te_id[w][v]=te_m; te_v[te_m]=v; te_w[te_m]=w; te_m++; } }
  memset(te_bad,0,te_m*sizeof(edgeset)); memset(te_ok,0,te_m*sizeof(edgeset)); memset(&te_chosen,0,sizeof(edgeset));
  memcpy(g,bitgraph,sizeof(LONGTYPE)*nv);
  te_withdeg3=withdeg3; te_budget=TWOEDGE_BUDGET;
  r=te_search(g,deg2,dm);
  globalwithdeg3=withdeg3;
  if (te_budget<0) te_gaveup++;
  return r;
}

#if (defined(GIRTH7))
void far_layers(int v, LONGTYPE ball[], int r)
// ball[k]: the vertices at distance at most k from v (k=0...r)
{
  LONGTYPE b=bit(v), f=bit(v), nf, s;
  int k;

  ball[0]=b;
  for (k=1; k<=r; k++)
    { nf=EMPTYSET;
      for (s=f; s; s&=s-1) nf|=bitgraph[SMALLEST_CTZ(s)];
      nf&=~b; b|=nf; f=nf; ball[k]=b; }
}

int far_update(LONGTYPE pfar[], LONGTYPE newdeg2, LONGTYPE bb[], LONGTYPE bi[])
// computes faraway[] after the edge {b,i} was inserted from the values pfar[] before (bb, bi: the balls around b and i before
// the edge was inserted). A pair {x,y} gets distance at most girth-2 by the new edge iff d(x,b)+d(i,y)<=girth-3 or
// d(x,i)+d(b,y)<=girth-3. Returns 0 if a vertex of degree 2 has no possible partner left. Same result as recomputing
// faraway[] with (*set_OK_distances_pointer)().
{
  int x, r, R=girth-3;
  LONGTYPE s;

  for (s=newdeg2; s; s&=s-1) { x=SMALLEST_CTZ(s); faraway[x]=pfar[x]&newdeg2; }
  for (r=0; r<=R; r++)
    { for (s=bb[r]&newdeg2; s; s&=s-1) { x=SMALLEST_CTZ(s); faraway[x]&=~bi[R-r]; }
      for (s=bi[r]&newdeg2; s; s&=s-1) { x=SMALLEST_CTZ(s); faraway[x]&=~bb[R-r]; } }
  for (s=newdeg2; s; s&=s-1) if (faraway[SMALLEST_CTZ(s)]==EMPTYSET) return 0;
  return 1;
}
#endif

void construct_mrv_inner(LONGTYPE withdeg3, int prevtop);

void construct_mrv(LONGTYPE withdeg3, int prevtop)
{
  int saved=fc_nostore;

  construct_mrv_inner(withdeg3,prevtop);
  fc_nostore=saved; globalwithdeg3=withdeg3;
}

void construct_mrv_inner(LONGTYPE withdeg3, int prevtop)
// Option K (see the header): like construct(), but every possible partner of every vertex of degree 2 is tested, and the
// vertex b that gets its third edge is chosen by the number of its possible partners. No isomorphism rejection, no early
// tests of the connectivity (the graph is tested by (*codef)()). prevtop is the vertex that got its third edge last.
{
  int v, w, b, k, ok, i, n1, n2, i1, i2, ea, eb, nverts, order[KNOTEN], aok, bok;
  LONGTYPE deg2, vs, ws, cs, partners, *bad, dom[KNOTEN], okpair[KNOTEN], *pok, hasc, fa, fb, okv, acc;
  LONGTYPE rem_n2, rem_i2, rem_n3, rem_i3, remn1_3, remn2_3, remi1_3, remi2_3;
#if (defined(GIRTH7))
  LONGTYPE deg2_2, pfar[KNOTEN], bb[8], bi[8];
#else
  int okmode=OKDIST_MODE;
#endif

  if (ne==splitlevel)
    {
      splitcounter++;
      if (splitcounter==mod) splitcounter=0;
      else return;
    }
  if (ne==presplitlevel)
    {
      presplitcounter++;
      if (presplitcounter==premod) presplitcounter=0;
      else return;
    }
  mrv_nodes++;
  deg2=COMPL(withdeg3);
  if (deg2==EMPTYSET) { (*codef)(); return; }

  // forward checking as in construct() -- but all possible partners of all vertices of degree 2 are tested, in the order
  // of increasing number of possible partners
  bad=fc_bad[ne]; memcpy(bad,fc_bad[ne-1],nv*sizeof(LONGTYPE));
  ea=prevtop; eb=representation[prevtop]; // the edge inserted last
  { LONGTYPE bucket[KNOTEN+1], dsizes=EMPTYSET;
    int d, dv[KNOTEN], start[KNOTEN+1], cum=0;

    for (d=0; d<nv; d++) bucket[d]=EMPTYSET;
    for (vs=deg2; vs; vs&=vs-1)
      { v=SMALLEST_CTZ(vs);
#if (defined(GIRTH7))
	dom[v]=faraway[v] & (~bit(v)) & (~bad[v]); // faraway[] was computed for this graph before the call
#else
	dom[v]=OKDIST(okmode,v) & deg2 & (~bit(v)) & (~bad[v]);
#endif
	d=POPC(dom[v]); dv[v]=d; SETBIT(bucket[d],v); SETBIT(dsizes,d); }
    if (bucket[0]) { fc_prune++; return; }
    for (vs=dsizes; vs; vs&=vs-1) { d=SMALLEST_CTZ(vs); start[d]=cum; cum+=POPC(bucket[d]); }
    nverts=cum;
    for (vs=deg2; vs; vs&=vs-1) { v=SMALLEST_CTZ(vs); d=dv[v]; order[start[d]+POPC(bucket[d]&(bit(v)-1))]=v; }
  }
  fc_nostore=1; // no 2-factors are stored for the isomorphism rejection
  // A pair accepted on the level before can only be rejected by a 2-factor that also contains the new edge {ea,eb}. Such a
  // 2-factor of G+{v,w} drops one 2-factor edge at each of v, w, ea and eb, so each of them needs a neighbour in the initial
  // 2-factor that has degree 3 (in G+{v,w}): it is matched with it in the complementary perfect matching. Restricted tests
  // of pairs without such neighbours are not done. hasc: the vertices of degree 2 with such a neighbour in G.
  pok= (ne>mrvlevel) ? fc_okpairs[ne-1] : NULL;
  hasc=EMPTYSET; fa=initialgraph[ea]; fb=initialgraph[eb]; aok=((fa&withdeg3)!=EMPTYSET); bok=((fb&withdeg3)!=EMPTYSET);
  for (vs=deg2; vs; vs&=vs-1)
    { v=SMALLEST_CTZ(vs); okpair[v]=EMPTYSET; if (initialgraph[v]&withdeg3) SETBIT(hasc,v); }
  for (k=0; k<nverts; k++)
    {
      v=order[k];
      // the partners that were already accepted from the other side are not tested again (a test only changes the bits of
      // the pair tested, so the results do not depend on the order of the tests)
      partners=dom[v] & (~bad[v]) & (~okpair[v]);
      fc_tests+=POPC(partners);
      okv=EMPTYSET;
      if (pok!=NULL)
	{ // partners accepted on the level before: only 2-factors containing the new edge can reject them
	  acc=partners&pok[v]; partners&=~acc;
	  fc_rtests+=POPC(acc);
	  ws=EMPTYSET; // the partners for which such a 2-factor is possible
	  if (IS_SET(hasc,v))
	    { ws=acc&hasc;
	      if (!aok && !(fa&bit(v))) ws&=fa;
	      if (!bok && !(fb&bit(v))) ws&=fb; }
	  okv=acc&~ws;
	  for ( ; ws; ws&=ws-1)
	    {
	      w=SMALLEST_CTZ(ws);
	      ADD_EDGE(v,w);
	      globalwithdeg3=withdeg3|bit(v)|bit(w);
	      ok= !contradicting_twofac_through(v,w,globalwithdeg3,ea,eb);
	      DEL_EDGE(v,w);
	      if (ok) SETBIT(okv,w); else { SETBIT(bad[v],w); SETBIT(bad[w],v); }
	    }
	}
      for (ws=partners; ws; ws&=ws-1)
	{
	  w=SMALLEST_CTZ(ws);
	  ADD_EDGE(v,w);
	  globalwithdeg3=withdeg3|bit(v)|bit(w);
	  fc_v=v; fc_w=w; fc_level=ne-1; fc_wantlevel=fc_lift;
	  ok= !contradicting_twofac(v,w,globalwithdeg3);
	  fc_wantlevel=0;
	  DEL_EDGE(v,w);
	  if (ok) SETBIT(okv,w); else fc_store_bad(v,w,fc_level,ne);
	}
      okpair[v]|=okv;
      for (ws=okv; ws; ws&=ws-1) SETBIT(okpair[SMALLEST_CTZ(ws)],v);
      if (okpair[v]==EMPTYSET) { globalwithdeg3=withdeg3; fc_prune++; return; }
    }
  globalwithdeg3=withdeg3;
  for (vs=deg2; vs; vs&=vs-1) { v=SMALLEST_CTZ(vs); fc_okpairs[ne][v]=okpair[v]; pm_P[v]=dom[v] & (~bad[v]); }

  // the matching test and option P
  if (pm_use && !pm_rec(deg2)) { pm_prune++; return; }
  if (POPC(deg2)<=pairlevel)
    { if (!twoedge_test(withdeg3,deg2,pm_P)) { te_prune++; return; } }

  // The vertex b with the smallest key BR_W1*(possible partners)+BR_W2*(neighbours in the initial 2-factor that have
  // degree 2)+BR_W3*(possible partners whose two neighbours in the initial 2-factor have degree 2) -- the largest one if
  // there are several. It is only chosen after the tests above: nearly all nodes with few vertices of degree 2 are pruned.
  { int key, bkey=INT_MAX, nh;
    LONGTYPE cs2;
    b= -1;
    for (vs=deg2; vs; vs&=vs-1)
      { v=SMALLEST_CTZ(vs);
	cs2=dom[v]&(~bad[v]); k=POPC(cs2);
	for (nh=0; cs2; cs2&=cs2-1) if (!(initialgraph[SMALLEST_CTZ(cs2)]&withdeg3)) nh++;
	key=(BR_W1*k+BR_W2*POPC(initialgraph[v]&deg2)+BR_W3*nh)*64+(63-v);
	if (key<bkey) { bkey=key; b=v; } } }

  SETBIT(withdeg3,b);
#if (defined(GIRTH7))
  for (vs=deg2; vs; vs&=vs-1) { v=SMALLEST_CTZ(vs); pfar[v]=faraway[v]; }
  far_layers(b,bb,girth-3);
#endif
  n1=SMALLEST_CTZ(bitgraph[b]);
  n2=LARGEST_CLZ(bitgraph[b]);
  for (cs=dom[b] & (~bad[b]); cs; cs&=cs-1)
    {
      i=SMALLEST_CTZ(cs);
      if (IS_SET(bad[b],i)) continue; // can have been forbidden by lifting in the meantime
      i1=SMALLEST_CTZ(bitgraph[i]);
      i2=LARGEST_CLZ(bitgraph[i]);
#if (defined(GIRTH7))
      far_layers(i,bi,girth-3);
#endif
      ADD_EDGE(b,i);
      edgelevel[b]=edgelevel[i]=ne;
      globalwithdeg3=withdeg3 | bit(i);
      set_deg3nb(b,i,globalwithdeg3); // {b,i} passed the forward checking on this level, so it is not tested again
      rem_i2=dist2bitgraph[i]; rem_n2=dist2bitgraph[b];
      dist2bitgraph[i] |= bitgraph[b]; dist2bitgraph[b] |= bitgraph[i];
      rem_i3=dist3bitgraph[i]; rem_n3=dist3bitgraph[b];
      dist3bitgraph[i] |= dist2bitgraph[b]; dist3bitgraph[b] |= dist2bitgraph[i];
      remi1_3=dist3bitgraph[i1]; remi2_3=dist3bitgraph[i2];
      dist3bitgraph[i1] |= dist2bitgraph[i]; dist3bitgraph[i2] |= dist2bitgraph[i];
      remn1_3=dist3bitgraph[n1]; remn2_3=dist3bitgraph[n2];
      dist3bitgraph[n1] |= dist2bitgraph[b]; dist3bitgraph[n2] |= dist2bitgraph[b];
#if (defined(GIRTH7))
      deg2_2=COMPL(globalwithdeg3);
      if (far_update(pfar,deg2_2,bb,bi))
#endif
      construct_mrv(withdeg3 | bit(i), b);
      dist2bitgraph[i] = rem_i2; dist2bitgraph[b] = rem_n2;
      dist3bitgraph[i] = rem_i3; dist3bitgraph[b] = rem_n3;
      dist3bitgraph[n1] = remn1_3; dist3bitgraph[n2] = remn2_3;
      dist3bitgraph[i1] = remi1_3; dist3bitgraph[i2] = remi2_3;
      DEL_EDGE(b,i);
      if (jumplevel>=0) { if (ne>=jumplevel) return; jumplevel= -1; } // the removed edge had number ne+1
    }
}

void construct(LONGTYPE withdeg3, int previouscycle, LONGTYPE allout, int prevtop)
// Adds the next edge. Up to next-1 all vertices have degree 3
{

  int i, j, m,n, next, mycycle, nextcycle, doit;
#ifdef GIRTH4
  int nb;
#endif
  LONGTYPE deg2, _gbu, verticesfaraway, rem_n2, rem_i2, rem_n3, rem_i3;
  ISOSTRUCT *isostruct;
  static LONGTYPE edgeset[KNOTEN>>1], edges_above_isolimit;
  LONGTYPE remn1_3, remn2_3,remi1_3, remi2_3;
  int n1, n2, i1, i2;
#if (defined(GIRTH7))
  LONGTYPE _gbu2, deg2_2;
  int ii;
#endif

 #ifdef GETSPLITS
  if (ne==splitlevel) { numberbranches++; return; }
#else
  if (ne==splitlevel)
    {
      splitcounter++;
      if (splitcounter==mod) splitcounter=0;
      else return;
    }
  if (ne==presplitlevel)
    {
      presplitcounter++;
      if (presplitcounter==premod) presplitcounter=0;
      else return;
    }
#endif

  deg2=COMPL(withdeg3);
  next=SMALLEST(deg2);

   if (next<0) { (*codef)(); return; }

   n1=SMALLEST_CTZ(bitgraph[next]);
   n2=LARGEST_CLZ(bitgraph[next]);
     
  // helps a lot:
  for (i=prevtop; i<next; i++) {allout = (allout | bitgraph[i]) & LARGERTHAN(i); if ((POPC(allout)<mincon) && (next>=3)) { return;}
    if (nontriv5 && (POPC(allout)<=5) && (i>=5) && doomed(UPTO(i),deg2)) { earlyrej[0]++; return; } }
  
  mycycle=incycle[next];
  
  if (mycycle!=previouscycle)
    {
      for (i=previouscycle; i<mycycle; i++)
	{
	  outgoing[i]=EMPTYSET;
	  FORALLELEMENTS(setcycle[i],j) outgoing[i]|=bitgraph[j];
	  if (POPC(outgoing[i]&(~setcycle[i]))<mincon) return; // too few edges leaving the cycle
	  if (nontriv5 && (POPC(outgoing[i]&(~setcycle[i]))<=5) && doomed(setcycle[i],deg2)) { earlyrej[1]++; return; }
	  if (i==0) outgoing_upto[i]=outgoing[i]; else outgoing_upto[i]=outgoing_upto[i-1] | outgoing[i];
	  if (POPC(outgoing_upto[i]&setlargercycles[i])<mincon) return; // too few edges leaving the smaller cycles
	  if (nontriv5 && (POPC(outgoing_upto[i]&setlargercycles[i])<=5) && doomed(allvertices&(~setlargercycles[i]),deg2)) { earlyrej[2]++; return; }
	}
      if (mincon> (POPC(setcycle[mycycle]&withdeg3) + POPC((~setcycle[mycycle])&deg2))) return;
      if (nontriv5 && doomed(setcycle[mycycle],deg2)) { earlyrej[3]++; return; }
      // e.g. when the sequence is 5 29 6 and all edges of the 5cycle go to the 6cycle it is detected (in a cheap way) that the 29-cycle will never be
      // sufficiently connected to the rest
     }

   if (fwdcheck)
     { // forward checking: a 2-factor of G+{v,w} through {v,w} is also a 2-factor of every completion containing {v,w}, so pairs
       // rejected by contradicting_twofac() stay rejected in the whole subtree. If a vertex of degree 2 has no partner left, the
       // subtree contains no snark.
       // The vertices are checked in the order of increasing number of remaining partners -- so a wipeout is found earlier.
       // For each vertex v the partner w that passed the test (witness) is stored. In the child the witness is tested first: as
       // G+{v,w} had no rejecting 2-factor through {v,w}, only 2-factors that also contain the new edge must be considered.
       int v, w, ok, nverts=0, k, order[KNOTEN], ea= -1, eb= -1, okmode=OKDIST_MODE;
       LONGTYPE vs, ws, partners, *bad=fc_bad[ne], rem, dom[KNOTEN], parentverified=EMPTYSET, verified=EMPTYSET;
       signed char *wit=fc_wit[ne], *parentwit=NULL;

       if (ne==0) memset(bad,0,nv*sizeof(LONGTYPE));
       else
	 { memcpy(bad,fc_bad[ne-1],nv*sizeof(LONGTYPE));
	   parentverified=fc_verified[ne-1]; parentwit=fc_wit[ne-1];
	   ea=prevtop; eb=representation[prevtop]; } // the edge inserted last
       { // the vertices are sorted by (dsize,v) -- this is the same order as that of a stable insertion sort by dsize
	 LONGTYPE bucket[KNOTEN+1], dsizes=EMPTYSET, vs0=deg2&(~bit(next));
	 int d, dv[KNOTEN], start[KNOTEN+1], cum=0;
	 // Claude: branch-free buckets; the position of v in the order is computed from the bucket sizes (same order as before)
	 for (d=0; d<nv; d++) bucket[d]=EMPTYSET; // d<=nv-1: a vertex has at most nv-1 partners
	 if (okmode==5)
	   for (vs=vs0; vs; vs&=vs-1)
	     {
	       v=SMALLEST_CTZ(vs);
	       dom[v]=(~dist3bitgraph[v]) & deg2 & (~bit(v)) & (~bad[v]);
	       d=POPC(dom[v]); dv[v]=d;
	       SETBIT(bucket[d],v); SETBIT(dsizes,d);
	     }
	 else
	   for (vs=vs0; vs; vs&=vs-1)
	     {
	       v=SMALLEST_CTZ(vs);
	       dom[v]=OKDIST(okmode,v) & deg2 & (~bit(v)) & (~bad[v]);
	       d=POPC(dom[v]); dv[v]=d;
	       SETBIT(bucket[d],v); SETBIT(dsizes,d);
	     }
	 if (bucket[0] && (ne>=splitlevel)) { fc_verified[ne]=EMPTYSET; fc_prune++; return; } // a vertex without partners
	 for (vs=dsizes; vs; vs&=vs-1) { d=SMALLEST_CTZ(vs); start[d]=cum; cum+=POPC(bucket[d]); }
	 nverts=cum;
	 for (vs=vs0; vs; vs&=vs-1)
	   { v=SMALLEST_CTZ(vs); d=dv[v]; order[start[d]+POPC(bucket[d]&(bit(v)-1))]=v; }
       }
       fc_nostore=1; rem=globalwithdeg3;
       for (k=0; k<nverts; k++)
	 {
	   v=order[k];
	   partners=dom[v] & (~bad[v]);
	   ok=0;
	   if (IS_SET(parentverified,v) && IS_SET(partners,parentwit[v]))
	     {
	       w=parentwit[v];
	       DELBIT(partners,w);
	       ADD_EDGE(v,w);
	       globalwithdeg3=withdeg3|bit(v)|bit(w);
	       fc_tests++; fc_rtests++;
	       ok= !contradicting_twofac_through(v,w,globalwithdeg3,ea,eb);
	       DEL_EDGE(v,w);
	       if (!ok) { SETBIT(bad[v],w); SETBIT(bad[w],v); }
	     }
	   for (ws=partners; ws && !ok; )
	     {
	       w=LARGEST_CLZ(ws); // the largest vertices get their third edge last, so they remain witnesses longer
	       DELBIT(ws,w);
	       ADD_EDGE(v,w);
	       globalwithdeg3=withdeg3|bit(v)|bit(w);
	       fc_tests++;
	       fc_v=v; fc_w=w; fc_level=ne-1; fc_wantlevel=fc_lift;
	       ok= !contradicting_twofac(v,w,globalwithdeg3);
	       fc_wantlevel=0;
	       DEL_EDGE(v,w);
	       if (!ok) fc_store_bad(v,w,fc_level,ne);
	     }
	   if (ok) { wit[v]=w; SETBIT(verified,v); }
	   else { if (ne<splitlevel) break; fc_verified[ne]=verified; fc_nostore=0; globalwithdeg3=rem; fc_prune++; return; }
	 }
       fc_verified[ne]=verified;
       fc_nostore=0; globalwithdeg3=rem;

       if (pm_use && (ne>=splitlevel))
	 { // the vertices of degree 2 must be matched by the edges still to be inserted: is there a perfect matching in the
	   // graph of possible partners (allowed distance, not known to give a rejecting 2-factor)?
	   // dom[v] (v!=next) was computed with a subset of the current bad[v] -- the sets only grow
	   for (vs=deg2&(~bit(next)); vs; vs&=vs-1) { v=SMALLEST_CTZ(vs); pm_P[v]=dom[v] & (~bad[v]); }
	   pm_P[next]=OKDIST(okmode,next) & deg2 & (~bit(next)) & (~bad[next]);
	   if (!pm_rec(deg2)) { pm_prune++; return; }
	 }
     }

   SETBIT(withdeg3,next);

#if (defined(GIRTH7))
   verticesfaraway=  faraway[next];
#else
   verticesfaraway= OKDIST(OKDIST_MODE,next) & deg2;
#endif		     
   if (fwdcheck) verticesfaraway &= ~fc_bad[ne][next];

#ifdef GIRTH4
   // Claude: only for girth 4 -- for girth at least 5 the neighbours of nb are at distance at most 3 from next and never in
   // verticesfaraway (measured: no effect at all for girth 5 and 6)
   nb = SMALLEST(bitgraph[previous_in2f[next]] & (~setcycle[mycycle]));

   if ((nb>0) && ((numberoddcycles<4) || ((cyclelengths[mycycle]&1)==0) || ((cyclelengths[incycle[nb]]&1)==0)) && (endtestf==(&endtest)))
     verticesfaraway &= (~bitgraph[nb]); // connecting to one of them would always lead to a better (maybe even even) 2-factor
   // only connecting two odd cycles of 4 or more and reducing the largest odd cycle by this might be allowed (although such a case might not exist for a canonical 2F)
#endif

   FORALLELEMENTS(verticesfaraway, i)
       {
	 i1=SMALLEST_CTZ(bitgraph[i]);
	 i2=LARGEST_CLZ(bitgraph[i]);
	 nextcycle=incycle[i];
	 if (((firstofcycle[nextcycle]==i) || NOT_SET(deg2,firstofcycle[nextcycle]) ||   (firstofcycle[nextcycle]==next)) // i is the first in its cycle or the first has already been connected
	     && ((nextcycle==0) || (cyclelengths[nextcycle]!=cyclelengths[nextcycle-1]) ||  NOT_SET(deg2,firstofcycle[nextcycle-1]) || (firstofcycle[nextcycle-1]==next))) // and due to symmetry one can not just go to the previous cycle
	   {
	     doit=1;
	     ADD_EDGE(next,i);
	     edgelevel[next]=edgelevel[i]=ne;
	     edgeset[ne]=bit(next)|bit(i);
	     numbertwofac[ne]=numberisopointers[ne]=numberisostructs[ne]=0;
	     globalwithdeg3=withdeg3 | bit(i);
	     //globalwithdeg2=COMPL(globalwithdeg3);
	     fc_v=next; fc_w=i; fc_level=ne-1; fc_wantlevel=fwdcheck && fc_lift;
	     doit= !contradicting_twofac(next,i,globalwithdeg3);
	     fc_wantlevel=0;
	     if (!doit && fwdcheck && (fc_level<ne-1)) fc_store_bad(next,i,fc_level,ne-1);
	     if (doit)
	       {
		 if (ne<=isolimitne) 
		   {
		     // first the ones for which isostructs have already been created // they are all on the level one below
		     for (n=0;(n<numberisopointers[ne-1]) && doit;n++)
		       {
			 isostruct=the_isodatapointers[ne-1][n];
			 if (IS_SET(globalwithdeg3,isostruct->invnumber[isostruct->nextpos]))
			   {
			     if (smaller_rep_isostruct(isostruct->twofac, isostruct->nextpos, isostruct->numbered, isostruct->numbers_given,isostruct->number, isostruct->invnumber,
						isostruct->cyclength_van_top, isostruct->vertices_with_given_cyclength, isostruct->numbercycles)) doit=0;
			     }
			   else
			     { add_isodatapointer(isostruct, ne); } //Just add a pointer to the old struct
			 }
		     // then the ones for small edge numbers where one of the new vertices might be the first, so that no isostructs have already been created    
  
		     for (m=0;(m<ne) && doit;m++)
		       {
			 for (n=0;(n<numbertwofac[m]) && doit;n++)
			   {
			     if (smaller_rep_list(m,n,edgeset[ne])) doit=0;
			   }
		       }
		     // and finally the new 2-factors
		     for (n=0;(n<numbertwofac[ne]) && doit;n++)
			   {
			     if (smaller_rep_list(ne,n,globalwithdeg3)) doit=0;
			   }
  		   }

		 if (ne==finalne) 
		   {
		     // first the ones for which isostructs have already been created // they are all on the level one below
		     for (n=0;(n<numberisopointers[isolimitne]) && doit;n++)
		       {
			 isostruct=the_isodatapointers[isolimitne][n];
			 if (smaller_rep_isostruct(isostruct->twofac, isostruct->nextpos, isostruct->numbered, isostruct->numbers_given,isostruct->number, isostruct->invnumber,
						   isostruct->cyclength_van_top, isostruct->vertices_with_given_cyclength, isostruct->numbercycles)) doit=0;
			 }
		     // then the ones for small edge numbers where one of the new vertices might be the first, so that no isostructs have already been created

		     edges_above_isolimit=edgeset[ne];
		     for (j=isolimitne+1; j<ne; j++) edges_above_isolimit |= edgeset[j];
  
		     for (m=0;(m<=isolimitne) && doit;m++)
		       {
			 for (n=0;(n<numbertwofac[m]) && doit;n++)
			   {
			     if (smaller_rep_list(m,n,edges_above_isolimit)) doit=0;
			   }
		       }
		     // and finally the new 2-factors above isolimit
		     for (m=isolimitne+1;(m<=ne) && doit;m++)
		       {
			 for (n=0;(n<numbertwofac[m]) && doit;n++)
			   {
			     if (smaller_rep_list(m,n,globalwithdeg3)) doit=0;
			   }
		       }
  		   }

		 if (doit && !h7_doomed(next,i) && !h_doomed(next,i,globalwithdeg3))
		   {
		     // dist2		     
		     rem_i2=dist2bitgraph[i];
		     rem_n2=dist2bitgraph[next];
		     dist2bitgraph[i] |= bitgraph[next];
		     dist2bitgraph[next] |= bitgraph[i];
		     // end dist 2 now dist 3
		     rem_i3=dist3bitgraph[i];
		     rem_n3=dist3bitgraph[next];
		     dist3bitgraph[i] |= dist2bitgraph[next];
		     dist3bitgraph[next] |= dist2bitgraph[i];
		     remi1_3=dist3bitgraph[i1];
		     remi2_3=dist3bitgraph[i2];
		     dist3bitgraph[i1] |= dist2bitgraph[i];
		     dist3bitgraph[i2] |= dist2bitgraph[i];
		     remn1_3=dist3bitgraph[n1];
		     remn2_3=dist3bitgraph[n2];
		     dist3bitgraph[n1] |= dist2bitgraph[next];
		     dist3bitgraph[n2] |= dist2bitgraph[next];
		     //end dist 3
#if (defined(GIRTH7))
		     deg2_2=COMPL(globalwithdeg3);
		     FORALLELEMENTS2(deg2_2,ii) if ((faraway[ii]=((*set_OK_distances_pointer)(ii)&deg2_2))==EMPTYSET) { doit=0; }
		     if (doit)
#endif
		     { if (ne>=mrvlevel) construct_mrv(withdeg3 | bit(i), next);
		       else construct(withdeg3 | bit(i), mycycle, allout, next); }
		     dist2bitgraph[i] = rem_i2;
		     dist2bitgraph[next] = rem_n2;
		     dist3bitgraph[i] = rem_i3;
		     dist3bitgraph[next] = rem_n3;
		     dist3bitgraph[n1] = remn1_3;
		     dist3bitgraph[n2] = remn2_3;
		     dist3bitgraph[i1] = remi1_3;
		     dist3bitgraph[i2] = remi2_3;
		   }
	       }// end no contradicting 2-factors
	     DEL_EDGE(next,i);
	     if (jumplevel>=0) { if (ne>=jumplevel) return; jumplevel= -1; } // the removed edge had number ne+1
	   }
       }

  return;
}


void construct_strong(LONGTYPE withdeg3, int previouscycle, LONGTYPE allout, int prevtop)
// Adds the next edge. Up to next-1 all vertices have degree 3
{

  int i, j, m,n, next, mycycle, nextcycle, doit;
#ifdef GIRTH4
  int nb;
#endif
  LONGTYPE deg2, _gbu, verticesfaraway, rem_n2, rem_i2, rem_n3, rem_i3;
  ISOSTRUCT *isostruct;
  static LONGTYPE edgeset[KNOTEN>>1], edges_above_isolimit;
  LONGTYPE remn1_3, remn2_3,remi1_3, remi2_3;
  int n1, n2, i1, i2;
#if (defined(GIRTH7))
  LONGTYPE _gbu2, deg2_2;
  int ii;
#endif

 #ifdef GETSPLITS
  if (ne==splitlevel) { numberbranches++; return; }
#else
  if (ne==splitlevel)
    {
      splitcounter++;
      if (splitcounter==mod) splitcounter=0;
      else return;
    }
  if (ne==presplitlevel)
    {
      presplitcounter++;
      if (presplitcounter==premod) presplitcounter=0;
      else return;
    }
#endif

  deg2=COMPL(withdeg3);
  next=SMALLEST(deg2);

   if (next<0) { (*codef)(); return; }
 
   n1=SMALLEST_CTZ(bitgraph[next]);
   n2=LARGEST_CLZ(bitgraph[next]);
     
  // helps a lot:
  for (i=prevtop; i<next; i++) {allout = (allout | bitgraph[i]) & LARGERTHAN(i); if ((POPC(allout)<mincon) && (next>=3)) { return;}
    if (nontriv5 && (POPC(allout)<=5) && (i>=5) && doomed(UPTO(i),deg2)) { earlyrej[0]++; return; } }
  
  mycycle=incycle[next];
  
  if (mycycle!=previouscycle)
    {
      for (i=previouscycle; i<mycycle; i++)
	{
	  outgoing[i]=EMPTYSET;
	  FORALLELEMENTS(setcycle[i],j) outgoing[i]|=bitgraph[j];
	  if (POPC(outgoing[i]&(~setcycle[i]))<mincon) return; // too few edges leaving the cycle
	  if (nontriv5 && (POPC(outgoing[i]&(~setcycle[i]))<=5) && doomed(setcycle[i],deg2)) { earlyrej[1]++; return; }
	  if (i==0) outgoing_upto[i]=outgoing[i]; else outgoing_upto[i]=outgoing_upto[i-1] | outgoing[i];
	  if (POPC(outgoing_upto[i]&setlargercycles[i])<mincon) return; // too few edges leaving the smaller cycles
	  if (nontriv5 && (POPC(outgoing_upto[i]&setlargercycles[i])<=5) && doomed(allvertices&(~setlargercycles[i]),deg2)) { earlyrej[2]++; return; }
	}
      if (mincon> (POPC(setcycle[mycycle]&withdeg3) + POPC((~setcycle[mycycle])&deg2))) return;
      if (nontriv5 && doomed(setcycle[mycycle],deg2)) { earlyrej[3]++; return; }
      // e.g. when the sequence is 5 29 6 and all edges of the 5cycle go to the 6cycle it is detected (in a cheap way) that the 29-cycle will never be
      // sufficiently connected to the rest
     }

   SETBIT(withdeg3,next);

#if (defined(GIRTH7))
   verticesfaraway=  faraway[next] & allowedstrong[ne][next];
#else
   verticesfaraway= (*set_OK_distances_pointer)(next) & deg2 & allowedstrong[ne][next];
#endif		     

#ifdef GIRTH4
   // Claude: only for girth 4 -- for girth at least 5 the neighbours of nb are at distance at most 3 from next and never in
   // verticesfaraway (measured: no effect at all for girth 5 and 6)
   nb = SMALLEST(bitgraph[previous_in2f[next]] & (~setcycle[mycycle]));

   if ((nb>0) && ((numberoddcycles<4) || ((cyclelengths[mycycle]&1)==0) || ((cyclelengths[incycle[nb]]&1)==0)) && (endtestf==(&endtest)))
     verticesfaraway &= (~bitgraph[nb]); // connecting to one of them would always lead to a better (maybe even even) 2-factor
   // only connecting two odd cycles of 4 or more and reducing the largest odd cycle by this might be allowed (although such a case might not exist for a canonical 2F)
#endif

   FORALLELEMENTS(verticesfaraway, i)
       {
	 i1=SMALLEST_CTZ(bitgraph[i]);
	 i2=LARGEST_CLZ(bitgraph[i]);
	 nextcycle=incycle[i];
	 if (((firstofcycle[nextcycle]==i) || NOT_SET(deg2,firstofcycle[nextcycle]) ||   (firstofcycle[nextcycle]==next)) // i is the first in its cycle or the first has already been connected
	     && ((nextcycle==0) || (cyclelengths[nextcycle]!=cyclelengths[nextcycle-1]) ||  NOT_SET(deg2,firstofcycle[nextcycle-1]) || (firstofcycle[nextcycle-1]==next))) // and due to symmetry one can not just go to the previous cycle
	   {
	     memcpy(allowedstrong[ne+1]+next,allowedstrong[ne]+next,(nv-next)*sizeof(LONGTYPE));
	     doit=1;
	     ADD_EDGE(next,i);
	     edgelevel[next]=edgelevel[i]=ne;
	     edgeset[ne]=bit(next)|bit(i);
	     numbertwofac[ne]=numberisopointers[ne]=numberisostructs[ne]=0;
	     globalwithdeg3=withdeg3 | bit(i);
	     globalwithdeg2=COMPL(globalwithdeg3);
	     if (!contradicting_twofac(next,i,globalwithdeg3))
	       {
		 if (ne<=isolimitne) 
		   {
		     // first the ones for which isostructs have already been created // they are all on the level one below
		     for (n=0;(n<numberisopointers[ne-1]) && doit;n++)
		       {
			 isostruct=the_isodatapointers[ne-1][n];
			 if (IS_SET(globalwithdeg3,isostruct->invnumber[isostruct->nextpos]))
			   {
			     if (smaller_rep_isostruct(isostruct->twofac, isostruct->nextpos, isostruct->numbered, isostruct->numbers_given,isostruct->number, isostruct->invnumber,
						isostruct->cyclength_van_top, isostruct->vertices_with_given_cyclength, isostruct->numbercycles)) doit=0;
			     }
			   else
			     { add_isodatapointer(isostruct, ne); } //Just add a pointer to the old struct
			 }
		     // then the ones for small edge numbers where one of the new vertices might be the first, so that no isostructs have already been created    
  
		     for (m=0;(m<ne) && doit;m++)
		       {
			 for (n=0;(n<numbertwofac[m]) && doit;n++)
			   {
			     if (smaller_rep_list(m,n,edgeset[ne])) doit=0;
			   }
		       }
		     // and finally the new 2-factors
		     for (n=0;(n<numbertwofac[ne]) && doit;n++)
			   {
			     if (smaller_rep_list(ne,n,globalwithdeg3)) doit=0;
			   }
  		   }

		 if (ne==finalne) 
		   {
		     // first the ones for which isostructs have already been created // they are all on the level one below
		     for (n=0;(n<numberisopointers[isolimitne]) && doit;n++)
		       {
			 isostruct=the_isodatapointers[isolimitne][n];
			 if (smaller_rep_isostruct(isostruct->twofac, isostruct->nextpos, isostruct->numbered, isostruct->numbers_given,isostruct->number, isostruct->invnumber,
						   isostruct->cyclength_van_top, isostruct->vertices_with_given_cyclength, isostruct->numbercycles)) doit=0;
			 }
		     // then the ones for small edge numbers where one of the new vertices might be the first, so that no isostructs have already been created

		     edges_above_isolimit=edgeset[ne];
		     for (j=isolimitne+1; j<ne; j++) edges_above_isolimit |= edgeset[j];
  
		     for (m=0;(m<=isolimitne) && doit;m++)
		       {
			 for (n=0;(n<numbertwofac[m]) && doit;n++)
			   {
			     if (smaller_rep_list(m,n,edges_above_isolimit)) doit=0;
			   }
		       }
		     // and finally the new 2-factors above isolimit
		     for (m=isolimitne+1;(m<=ne) && doit;m++)
		       {
			 for (n=0;(n<numbertwofac[m]) && doit;n++)
			   {
			     if (smaller_rep_list(m,n,globalwithdeg3)) doit=0;
			   }
		       }
  		   }

		 if (doit && !h7_doomed(next,i) && !h_doomed(next,i,globalwithdeg3))
		   {
		     // dist2		     
		     rem_i2=dist2bitgraph[i];
		     rem_n2=dist2bitgraph[next];
		     dist2bitgraph[i] |= bitgraph[next];
		     dist2bitgraph[next] |= bitgraph[i];
		     // end dist 2 now dist 3
		     rem_i3=dist3bitgraph[i];
		     rem_n3=dist3bitgraph[next];
		     dist3bitgraph[i] |= dist2bitgraph[next];
		     dist3bitgraph[next] |= dist2bitgraph[i];
		     remi1_3=dist3bitgraph[i1];
		     remi2_3=dist3bitgraph[i2];
		     dist3bitgraph[i1] |= dist2bitgraph[i];
		     dist3bitgraph[i2] |= dist2bitgraph[i];
		     remn1_3=dist3bitgraph[n1];
		     remn2_3=dist3bitgraph[n2];
		     dist3bitgraph[n1] |= dist2bitgraph[next];
		     dist3bitgraph[n2] |= dist2bitgraph[next];
		     //end dist 3
#if (defined(GIRTH7))
		     deg2_2=COMPL(globalwithdeg3);
		     FORALLELEMENTS2(deg2_2,ii) if ((faraway[ii]=((*set_OK_distances_pointer)(ii)&deg2_2))==EMPTYSET) { doit=0; }
		     if (doit)
#endif
		     construct_strong(withdeg3 | bit(i), mycycle, allout, next);
		     dist2bitgraph[i] = rem_i2;
		     dist2bitgraph[next] = rem_n2;
		     dist3bitgraph[i] = rem_i3;
		     dist3bitgraph[next] = rem_n3;
		     dist3bitgraph[n1] = remn1_3;
		     dist3bitgraph[n2] = remn2_3;
		     dist3bitgraph[i1] = remi1_3;
		     dist3bitgraph[i2] = remi2_3;
		   }
	       }// end no contradicting 2-factors
	     DEL_EDGE(next,i);
	     if (jumplevel>=0) { if (ne>=jumplevel) return; jumplevel= -1; } // the removed edge had number ne+1
	   }
       }

  return;
}


void make2factors_even(int cyclengths[], int numcycs, int nvsofar) 
 {
   int i, next;
   LONGTYPE remember;
   unsigned long int timesofar;

  if (nvsofar==nv) 
    {
      if (given_sequence)
	{
	  for (i=0; i<numcycs;i++) if (cyclengths[i]!=wanted_sequence[i]) return;
	 }
      maxlength_even=cyclengths[numcycs-1];
      init(cyclengths, numcycs);
      if (strong) construct_strong(EMPTYSET,0,EMPTYSET,0);
      else construct(EMPTYSET,0,EMPTYSET,0);
      times(&timestruct);
      timesofar= (unsigned long int) timestruct.tms_utime;
      if (!quiet)
	{
	  if (writeoutput) fprintf(stderr,"Wrote %lu graphs for this case, so %lu in total so far.\n",counter-remember_counter,counter);
	  else fprintf(stderr,"Generated %lu graphs for this case, so %lu in total so far.\n",counter-remember_counter,counter);
	}
      remember_counter=counter;
      if (!quiet)
	{
	  fprintf(stderr,"Time for this part: %.2lf seconds. ",((double)timesofar -(double)remember_time)/(double)sysconf(_SC_CLK_TCK));
	  fprintf(stderr,"Total time so far: %.2lf seconds. \n\n",((double)timesofar)/(double)sysconf(_SC_CLK_TCK));
	}
      remember_time=timesofar;
      return;
    }
  if (nvsofar>nv-girth) { return;}

   if (ODD(cyclengths[numcycs-1])) next=girth;
   else next=cyclengths[numcycs-1];

   if (ODD(next)) next++;

   for ( ;nvsofar+next<=nv; next+=2)
     {
       cyclengths[numcycs]=next;
       remember=refset_e;
       SETBIT2(refset_e,next);
       make2factors_even(cyclengths, numcycs+1, nvsofar+next);
       refset_e=remember;
       //DELBIT(refset_e,next); is not possible, as the bit might have been set before
     } 
   return;
 }


void make2factors_odd(int cyclengths[], int numcycs, int nvsofar, int localmax) // cycles in non-increasing length, but odd ones first
 {
   int i, next;
   LONGTYPE remember;
   unsigned long int timesofar;

   if (use_oddness_result && (numcycs>2)) return;
   
   if ((nvsofar==nv) && ((numcycs>2) || !strong))
     {
       if (given_sequence)
	 {
	   for (i=0; i<numcycs;i++) if (cyclengths[i]!=wanted_sequence[i]) { return; }
	 }
       maxlength_odd=localmax; maxlength_even=0;
       numberoddcycles=numcycs;
       init(cyclengths, numcycs);
       if (strong) construct_strong(EMPTYSET,0,EMPTYSET,0);
      else construct(EMPTYSET,0,EMPTYSET,0);
       times(&timestruct);
       timesofar= (unsigned long int) timestruct.tms_utime;
       if (!quiet)
	 {
	   if (writeoutput) fprintf(stderr,"Wrote %lu graphs for this case, so %lu in total so far.\n",counter-remember_counter,counter);
	   else fprintf(stderr,"Generated %lu graphs for this case, so %lu in total so far.\n",counter-remember_counter,counter);
	 }
       remember_counter=counter;
       if (!quiet)
	 {
	   fprintf(stderr,"Time for this part: %.2lf seconds. ",((double)timesofar -(double)remember_time)/(double)sysconf(_SC_CLK_TCK));
	   fprintf(stderr,"Total time so far: %.2lf seconds. \n\n",((double)timesofar)/(double)sysconf(_SC_CLK_TCK));
	 }
      remember_time=timesofar;
       return;
    }
   if (nvsofar>nv-girth) { return; }

   if (numcycs==0)  { if (EVEN(girth)) next=girth+1; else next=girth; }
   else next=cyclengths[numcycs-1];
 
 
   for ( ;next+nvsofar<=nv; next+=2)
     {
       cyclengths[numcycs]=next;
       remember=refset_o;
       SETBIT2(refset_o,next);
       make2factors_odd(cyclengths, numcycs+1, nvsofar+next, MAX(localmax,next));
       if ((nvsofar+next<=nv-girth) &&  (ODD(nvsofar))) // so EVEN(nvsofar+next)
	 {
	   maxlength_odd=MAX(localmax,next);
	   numberoddcycles=numcycs+1;
	   make2factors_even(cyclengths, numcycs+1, nvsofar+next);
	 }
       refset_o=remember;
     }
   return;
 }

void sort(int s[], int length) // stupid bubblesort, but for very small sequences...
{
  int i,j,buf;

  for (i=0;i<length-1;i++)
    for (j=length-1;j>i;j--)
      if (s[j]<s[j-1]) { buf=s[j]; s[j]=s[j-1]; s[j-1]=buf; }
}

int numbits(int n)
// stupid function so that the overkill log function and ceil don't have to be used and linked
// returns the number of bits necessary to represent n
{
  int i,j;

  n++;
  // in principle the test is (i-1)<n
  for (i=1, j=0; i<n; j++) i= (i<<1);
  return j;
}

int main(int argc, char *argv[])
{

  int i,j, fixed=0;
  int sum, buffer, bufferodd[KNOTEN/2], buffereven[KNOTEN/2], numbereven, numberodd;

#ifdef LARGE
  int maxnumcyc, cycrest, bitsum;

  for (bitsum=0,i=5;i<KNOTEN; i+=2) // max number of vertices is KNOTEN-4 anyway
    {
      cycrest=KNOTEN%i;
      if ((cycrest==0) || (cycrest>=4)) maxnumcyc=KNOTEN/i;
      else maxnumcyc= (KNOTEN/i) - 1;
      bit2value[i]=(((LONGTYPE)1)<<bitsum);
      bitsum+=numbits(maxnumcyc);
    }

   for (bitsum=0,i=4;i<KNOTEN; i+=2) // max number of vertices is KNOTEN-4 anyway
    {
      cycrest=KNOTEN%i;
      if ((cycrest==0) || (cycrest>=4)) maxnumcyc=KNOTEN/i;
      else maxnumcyc= (KNOTEN/i) - 1;
      bit2value[i]=(((LONGTYPE)1)<<bitsum);
      bitsum+=numbits(maxnumcyc);
    }
#endif

  fprintf(stderr,"minisnark %s\n",MINISNARK_VERSION);
  for (i=0; i<argc; i++) fprintf(stderr,"%s ",argv[i]);
  fprintf(stderr,"\n");

  endtestf=&endtest;
  
  if ((argc<3) || !isdigit(argv[1][0])) usage(argv[0]);

  nv=atoi(argv[1]);
  if (nv>KNOTEN-4) { fprintf(stderr,"Too many vertices, exit().\n"); exit(1); } // some reserve for some array sizes especially for edges 
  if (nv&1) { fprintf(stderr,"Odd vertex number -- seriously?\n"); exit(1); }
  nvh=nv/2;
  girth=atoi(argv[2]);
  if (girth<4) { fprintf(stderr,"Girth must be at least 4 -- this is used in some essential places. Exit(1).\n"); exit(1); }

#ifndef LARGE
  if (nv>40) { fprintf(stderr,"Too many vertices -- recompile with -DLARGE -- exit.\n"); exit(1); }
#endif
  
  finalne=nv/2;
  codef=output;
 
  for (i=3; i<argc; i++)
    {
      if (argv[i][0]=='S') { strong=1;  endtestf=endtest_strong; }
      else if (argv[i][0]=='N') nontriv5=1;
      else if (argv[i][0]=='b') use_backjump=0;
      else if (argv[i][0]=='w') fwdcheck=0;
      else if (argv[i][0]=='L') fc_lift=0;
      else if (argv[i][0]=='M') pm_use=0;
      else if (argv[i][0]=='E')
	{ if (isdigit(argv[i][1])) earlylevel=atoi(argv[i]+1); else usage(argv[0]); }
      else if (argv[i][0]=='K')
	{ if (isdigit(argv[i][1])) mrvlevel=atoi(argv[i]+1); else usage(argv[0]); }
      else if (argv[i][0]=='P')
	{ if (isdigit(argv[i][1])) pairlevel=atoi(argv[i]+1); else usage(argv[0]); }
      else
	if (argv[i][0]=='R') { codef=runtimecode; }
      else if (argv[i][0]=='T') { codef=tfccode; }
      else 	
      if (argv[i][0]=='n') writeoutput=0;
      else if (argv[i][0]=='c')
	{ if (isdigit(argv[i][1])) mincon=atoi(argv[i]+1); else usage(argv[0]); }
      else if (argv[i][0]=='q')
	{ quiet=1; }
      else if (argv[i][0]=='t')
	{  if (isdigit(argv[i][1])) testgap=atoi(argv[i]+1); else usage(argv[0]);  }
      else
	if ((argv[i][0]=='f') || (argv[i][0]=='F'))
	  {
	    if (given_sequence==1)
	      { fprintf(stderr,"Options f and F must be used at most once and never together. Exit.\n"); exit(1); }
	    if (argv[i][0]=='F') { endtestf=endtest_fix2fak; fixed=1; }
	    for (i++, sum=maxlength_odd=numberoddcycles=numbereven=numberodd=0; (i<argc) && (isdigit(argv[i][0])); i++)
	      { 
		buffer=atoi(argv[i]);
		if (buffer<girth) { fprintf(stderr,"Forced initial cycle contradicts girth -- exit.\n"); exit(1); }
		sum+=buffer;
		if (buffer&1) { bufferodd[numberodd]=buffer; numberodd++; }
		else { buffereven[numbereven]=buffer; numbereven++; }
	      }
	    if (nv!=sum) { fprintf(stderr,"Sum of cycle lengths doesn't equal chosen number of vertices. Exit.\n"); exit(1); }
	    sort(buffereven,numbereven); sort(bufferodd,numberodd);
	    for (j=0; j<numberodd; j++) wanted_sequence[j]=bufferodd[j];
	    for (j=0; j<numbereven; j++) wanted_sequence[numberodd+j]=buffereven[j];
	    given_sequence=1;
	    if ((i<argc) /*&& (!isdigit(argv[i][0]))*/) i--;
	  }
	else if (argv[i][0]=='m')
	  {
	    splitlevel=7;
	    i++; if ((i<argc) && isdigit(argv[i][0])) splitcounter=atoi(argv[i]); else usage(argv[0]);
	    i++; if ((i<argc) && isdigit(argv[i][0])) mod=atoi(argv[i]); else usage(argv[0]);
	    if ((splitcounter>=mod) || (mod<=0)) usage(argv[0]);
	  }
	else if (argv[i][0]=='j')
	  {
	    i++; if ((i<argc) && isdigit(argv[i][0])) presplitcounter=atoi(argv[i]); else usage(argv[0]);
	    i++; if ((i<argc) && isdigit(argv[i][0])) premod=atoi(argv[i]); else usage(argv[0]);
	    if ((presplitcounter>=premod) || (premod<=0)) usage(argv[0]);
	  }
	else  usage(argv[0]); 
    }

  if ((mrvlevel<KNOTEN) && strong)
    { fprintf(stderr,"Option K can not be combined with S. Exit.\n"); exit(1); }
  if ((mrvlevel<KNOTEN) && !fwdcheck)
    { fprintf(stderr,"Option K can not be combined with w. Exit.\n"); exit(1); }
  if ((mrvlevel<KNOTEN) && (codef==tfccode))
    { fprintf(stderr,"Option K can not be combined with T (the graphs are not written in the canonical labelling). Exit.\n"); exit(1); }
  if (mrvlevel<1) mrvlevel=1;
  if (pairlevel && (mrvlevel==KNOTEN))
    { fprintf(stderr,"Option P can only be used together with option K. Exit.\n"); exit(1); }

  if (strong)
    { if (fixed) { fprintf(stderr,"Not yet prepared to use options S and F together. Exit. \n"); exit(1); }
      if (splitlevel>0) splitlevel+=2;
    }

  if (splitlevel>0) fprintf(stderr,"Splitlevel=%d.\n",splitlevel);
  if (premod>1)
    { if (splitlevel<0) { fprintf(stderr,"Option j can only be used together with option m. Exit.\n"); exit(1); }
      presplitlevel=splitlevel-2;
      fprintf(stderr,"Pre-split on level %d: part %d of %d.\n",presplitlevel,presplitcounter,premod); }
  
  if (nontriv5 && (mincon<5)) mincon=5;
  if (mincon>girth) girth=mincon;
  // Claude: this must be done before the distance function is chosen -- before, with e.g. "g=4 c5" 4-cycles were constructed,
  // although girth 5 was assumed in the rest of the program (uninitialised entries of numbercycles[] were used -- crashes)
  //if (girth==3) set_OK_distances_pointer= &set_OK_distances_3;
  //else
#ifdef GIRTH4
  if (girth==4) set_OK_distances_pointer= &set_OK_distances_4;
  else
#endif
  if (girth==5) set_OK_distances_pointer= &set_OK_distances_5;
  else if (girth==6) set_OK_distances_pointer= &set_OK_distances_6;
  else set_OK_distances_pointer= &set_OK_distances_atleast7;

#ifdef GETSPLITS
  splitlevel=GETSPLITS;
#endif

   fprintf(stderr,"Testgap: %d\n",testgap);
    isolimitne=finalne-testgap;

 #ifdef GIRTH5
    if (girth<5)
      {
	fprintf(stderr,"Compiled without -DGIRTH4 -- so girth must be at least 5. For girth 4 compile with -DGIRTH4. Exit!\n");
	exit(1);
      }
#endif
#ifdef GIRTH6
    if (girth<6)
      {
	fprintf(stderr,"Compiled with -DGIRTH6 -- so girth must be at least 6. Exit!\n");
	exit(1);
      }
#endif
#ifdef GIRTH7
    if (girth<7)
      {
	fprintf(stderr,"Compiled with -DGIRTH7 -- so girth must be at least 7. Exit!\n");
	exit(1);
      }
#endif

  doublegirth=2*girth;

  if (use_oddness_result)
    {
      if ((mincon<4) || (nv>42))
	{ fprintf(stderr,"For these parameters it has not been proven that there are no snarks of oddness larger than 2. Ignoring option f.\n");
	  use_oddness_result=0;
	}
    }
      
  if (ODD(nv)) { fprintf(stderr,"Odd vertex number -- impossible.\n"); exit(0); }
  if (nv>63) { fprintf(stderr,"Too many vertices -- exit.\n"); exit(0); }

  allvertices= (((LONGTYPE)1)<<nv)-1;
  deg3nb_global[0][0]=deg3nb_global[0][1]=deg3nb_global[0][2]=deg3nb_global[0][3]=EMPTYSET;

  the_twofactors[0]=malloc(sizeof(LONGTYPE)*nv);
  if (the_twofactors[0]==NULL) { fprintf(stderr,"Do not get room for twofactors -- exit(1).\n"); exit(1); }
  the_cyclengths_van_top[0]=malloc(sizeof(NUMBERTYPE)*nv);
  if (the_cyclengths_van_top[0]==NULL) { fprintf(stderr,"Do not get room for 1 invariant of twofactors -- exit(1).\n"); exit(1); }
  the_vertices_with_given_cyclength[0]=malloc(sizeof(LONGTYPE)*nv);
  if (the_vertices_with_given_cyclength[0]==NULL) { fprintf(stderr,"Do not get room for 1 invariant of twofactors -- exit(1).\n"); exit(1); }
  maxnumbertwofactors[0]=1;
  
  for (i=1;i<=nv/2;i++)
    {
        the_twofactors[i]=malloc(sizeof(LONGTYPE)*nv*10);
	if (the_twofactors[i]==NULL) { fprintf(stderr,"Do not get room for twofactors -- exit(1).\n"); exit(1); }
	the_cyclengths_van_top[i]=malloc(sizeof(NUMBERTYPE)*nv*10);
	if (the_cyclengths_van_top[i]==NULL) { fprintf(stderr,"Do not get room for 10 invariants of twofactors -- exit(1).\n"); exit(1); }
	the_vertices_with_given_cyclength[i]=malloc(sizeof(LONGTYPE)*nv*10);
	if (the_vertices_with_given_cyclength[i]==NULL) { fprintf(stderr,"Do not get room for 10 invariants of twofactors -- exit(1).\n"); exit(1); }
	maxnumbertwofactors[i]=10;
    }

 for (i=0;i<=nv/2;i++)
    {
        the_isodatapointers[i]=malloc(sizeof(ISOSTRUCT *)*100);
	if (the_isodatapointers[i]==NULL) { fprintf(stderr,"Do not get room for isodata pointers -- exit(1).\n"); exit(1); }
	maxnumberisopointers[i]=100;
	the_isodatastructs[i]=malloc(sizeof(ISOSTRUCT)*100);
	if (the_isodatastructs[i]==NULL) { fprintf(stderr,"Do not get room for isodata structs -- exit(1).\n"); exit(1); }
	maxnumberisostructs[i]=100;
    }

  if ((codef==tfccode) && writeoutput) fputs("TFC1",stdout);
  make2factors_odd(cyclelengths,0,0,0);
  if ((codef==tfccode) && writeoutput) { tfc_flush(); putchar(0); }

  #ifdef GETSPLITS
  fprintf(stderr,"%lu branches at level %d.\n",numberbranches,splitlevel);
  #else

  if (writeoutput) fprintf(stderr,"\n Wrote altogether %lu graphs.\n",counter);
  else  fprintf(stderr,"\n Generated altogether %lu graphs.\n",counter);
  if (fwdcheck && !strong)
    { int d; fprintf(stderr,"forward checking: %lu tests (%lu restricted), %lu nodes pruned, %lu pairs lifted, distance:",fc_tests,fc_rtests,fc_prune,fc_lifted);
      for (d=0;d<KNOTEN;d++) if (fc_liftdist[d]) fprintf(stderr," %d:%lu",d,fc_liftdist[d]);
      fprintf(stderr,"\n"); }
  if (fwdcheck && !strong && pm_use) fprintf(stderr,"matching test: %lu nodes pruned\n",pm_prune);
  if (mrvlevel<KNOTEN) fprintf(stderr,"option K: %lu nodes from edge number %d on\n",mrv_nodes,mrvlevel);
  if (pairlevel) fprintf(stderr,"option P: %lu two-edge tests, %lu nodes pruned, %lu search nodes, %lu pairs of edges tested, gave up %lu times\n",
			  te_calls,te_prune,te_searchnodes,te_pairtests,te_gaveup);
#endif
  
   if (nontriv5)
     {
       fprintf(stderr,"Option N: %lu complete graphs tested, %lu rejected.\n",final_tested,final_rejected);
       fprintf(stderr,"Early rejections: prefix %lu, cycle %lu, first cycles %lu, current cycle %lu, two 5-cycles %lu, general search %lu of %lu tests.\n",
	       earlyrej[0],earlyrej[1],earlyrej[2],earlyrej[3],earlyrej[5],earlyrej[4],htests);
       fprintf(stderr,"Rejected complete graphs by level of the last edge inside a side of the cut (level: number):");
       for (i=0;i<=nv/2;i++) if (jumphist[i]) fprintf(stderr," %d:%lu",i,jumphist[i]);
       fprintf(stderr,"\nRejected complete graphs by size of the smaller side of the found cut (size: number):");
       for (i=0;i<=nv/2;i++) if (sidehist[i]) fprintf(stderr," %d:%lu",i,sidehist[i]);
       fprintf(stderr,"\n");
     }
   times(&timestruct);
   fprintf(stderr,"Total user time: %.2lf seconds. \n\n",(double)timestruct.tms_utime/(double)sysconf(_SC_CLK_TCK));

   

   return 0;

}
