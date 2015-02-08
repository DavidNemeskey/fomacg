CXXFLAGS = -g -ggdb -Wall -Wno-write-strings -Wno-uninitialized -O3 -std=c++11
CXX = g++
LINKFOMA = -lfoma
LINKALL  = $(LINKFOMA) -lz
RAPIDJSON = -Irapidjson
LEX = flex

SRCS = fomacg_common.cpp fomacg_converter.cpp fomacg_proc.cpp fomacg_rule_applier.cpp fomacg_stream_reader.cpp foma_extra.cpp rule_condition_tree.cpp

OBJS = $(SRCS:.cpp=.o)

all: fomacg fomacg-proc trie-test

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<  -o $@
fomacg:
	$(LEX) -8 fomacg2.l 
	$(CXX) $(CXXFLAGS) -o ../fomacg2 lex.yy.c rule_condition_tree.cpp fomacg_common.cpp foma_extra.cpp $(LDFLAGS) $(LINKALL)

fomacg-proc: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o ../fomacg-proc $(LDFLAGS) $(LINKFOMA) -lrt 

trie-test:
	$(CXX) $(CXXFLAGS) -o ../trie-test trie_test.cpp $(RAPIDJSON) $(LDFLAGS)

clean: 
	rm *.o *.yy.c
