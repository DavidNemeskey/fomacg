CXXFLAGS = -g -ggdb -Wall -Wno-write-strings -Wno-uninitialized -O3
CPP = g++
CXX = g++
LINKFOMA = -lfoma
LINKALL  = $(LINKFOMA) -lz
LEX = flex

SUBDIRS = src

.PHONY: subdirs $(SUBDIRS)
     
subdirs: $(SUBDIRS)
     
$(SUBDIRS):
	$(MAKE) -C $@

clean:
	rm -f fomacg2 fomacg-proc factorize
#-I/usr/local/include/ -I. 
