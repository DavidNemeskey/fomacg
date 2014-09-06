CXXFLAGS = -g -ggdb -Wall -Wno-write-strings -Wno-uninitialized -O3 -std=c++11
CXX = g++
LINKFOMA = -lfoma
LINKALL  = $(LINKFOMA) -lz
LEX = flex

SRCS = fomacg_common.cpp fomacg_converter.cpp fomacg_rule_applier.cpp \
       fomacg_stream_reader.cpp foma_extra.cpp rule_condition_tree.cpp \
       factorize.cpp fomacg_proc.cpp

OBJS = $(SRCS:.cpp=.o)

all: fomacg fomacg-proc factorize

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<  -o $@
fomacg:
	$(LEX) -8 fomacg2.l 
	$(CXX) $(CXXFLAGS) -o ../fomacg2 lex.yy.c rule_condition_tree.cpp \
	    fomacg_common.cpp foma_extra.cpp factorize.cpp $(LDFLAGS) $(LINKALL)

fomacg-proc: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o ../fomacg-proc $(LDFLAGS) $(LINKFOMA) -lrt 

factorize: $(OBJS)
	$(CXX) $(CXXFLAGS) -o ../factorize factorize.cpp factorize_main.cpp \
	    fomacg_common.cpp foma_extra.cpp $(LDFLAGS) $(LINKALL) -lrt

clean: 
	rm *.o *.yy.c
