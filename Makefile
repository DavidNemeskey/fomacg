CPPFLAGS = -g -ggdb -Wall -Wno-write-strings -Wno-uninitialized -O3
CPP = g++
CXX = g++
LINKFOMA = -lfoma
LINKALL  = $(LINKFOMA) -lz
LEX = flex
all: fomacg fomacg_proc

fomacg:
	cd src; $(LEX) -8 fomacg2.l ; $(CXX) $(CPPFLAGS) -o ../fomacg2 lex.yy.c rule_condition_tree.cpp $(LINKALL); cd ..

fomacg_proc:
	cd src; $(CXX) $(CPPFLAGS) *.cpp -o ../fomacg_proc $(LINKFOMA) -lrt ; cd ..

clean:
	rm fomacg2 fomacg_proc
#-I/usr/local/include/ -I. 
