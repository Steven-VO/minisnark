# Makefile for minisnark 1.1
#
#   make            builds minisnark (girth >= 5) and the versions for girth 4, >= 6 and >= 7
#   make LARGE=1    the same for more than 40 vertices (at most 60)
#   make CC=gcc     use gcc instead of clang

CC     = clang
CFLAGS = -O3 -march=native

ifdef LARGE
  CFLAGS += -DLARGE
endif

PROGS = minisnark minisnark_g4 minisnark_g6 minisnark_g7

all: $(PROGS)

minisnark: minisnark.c
	$(CC) $(CFLAGS) -o $@ $<

minisnark_g4: minisnark.c
	$(CC) $(CFLAGS) -DGIRTH4 -o $@ $<

minisnark_g6: minisnark.c
	$(CC) $(CFLAGS) -DGIRTH6 -o $@ $<

minisnark_g7: minisnark.c
	$(CC) $(CFLAGS) -DGIRTH7 -o $@ $<

clean:
	rm -f $(PROGS)

.PHONY: all clean
