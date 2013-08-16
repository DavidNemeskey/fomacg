CPPFLAGS = -g -ggdb -Wall -Wno-write-strings -Wno-uninitialized -O3
CPP = g++
CXX = g++
LINKFLAGS = -lfoma -lz
LEX = flex
all: fomacg

fomacg:
	cd src; $(LEX) -8 fomacg2.l ; $(CXX) $(CPPFLAGS) -o ../fomacg2 lex.yy.c rule_condition_tree.c $(LINKFLAGS); cd ..

clean:
	rm fomacg2
