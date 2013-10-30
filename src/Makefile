CXXFLAGS = -g -ggdb -Wall -Wno-write-strings -Wno-uninitialized -O3
CXX = g++
LINKFOMA = -lfoma
LINKALL  = $(LINKFOMA) -lz
LEX = flex

all: fomacg fomacg_proc

fomacg:
	$(LEX) -8 fomacg2.l 
	$(CXX) $(CXXFLAGS) -o ../fomacg2 lex.yy.c rule_condition_tree.cpp foma_extra.cpp $(LINKALL)

fomacg_proc:
	$(CXX) $(CPPFLAGS) *.cpp -o ../fomacg_proc $(LINKFOMA) -lrt 


