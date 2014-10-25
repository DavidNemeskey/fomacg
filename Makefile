CXXFLAGS = -g -ggdb -Wall -Wno-write-strings -Wno-uninitialized -O3
CPP = g++
CXX = g++
LINKFOMA = -lfoma
LINKALL  = $(LINKFOMA) -lz

SUBDIRS = src

.PHONY: subdirs $(SUBDIRS)
     
subdirs: $(SUBDIRS)
     
$(SUBDIRS):
	$(MAKE) -C $@

clean:
	rm fomacg2 fomacg-proc
#-I/usr/local/include/ -I. 
