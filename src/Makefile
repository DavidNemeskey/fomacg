CXXFLAGS = -g -ggdb -Wall -Wno-write-strings -Wno-uninitialized -O3
CXX = g++
LINKFOMA = -lfoma
LINKALL  = $(LINKFOMA) -lz
LEX = flex

SRCS = fomacg_common.cpp fomacg_converter.cpp fomacg_proc.cpp fomacg_rule_applier.cpp fomacg_stream_reader.cpp foma_extra.cpp rule_condition_tree.cpp

OBJS = $(SRCS:.cpp=.o)

all: fomacg fomacg_proc

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<  -o $@
fomacg:
	$(LEX) -8 fomacg2.l 
	$(CXX) $(CXXFLAGS) -o ../fomacg2 lex.yy.c rule_condition_tree.cpp foma_extra.cpp $(LDFLAGS) $(LINKALL)

fomacg_proc: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o ../fomacg_proc $(LDFLAGS) $(LINKFOMA) -lrt 

clean: 
	rm *.o *.yy.c
