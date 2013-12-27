# Makefile for Sunfish

PROG = sunfish evcsv cut ins change disp kisen2csa
SRC = book.cpp data.cpp evaluate.cpp filelist.cpp \
generate.cpp hash.cpp kifu.cpp learn.cpp learn2.cpp \
shogi.cpp shogi2.cpp think.cpp match.cpp param.cpp \
csa.cpp effect.cpp limit.cpp string.cpp uint96.cpp\
gencheck.cpp tree.cpp thread.cpp mate.cpp consultation.cpp \
evaldiff.cpp debug.cpp
SRC2 = sunfish.cpp evcsv.cpp kisen2csa.cpp cut.cpp ins.cpp change.cpp disp.cpp offset.cpp
OBJS=$(patsubst %.cpp,%.o,$(SRC))
OBJS2=$(patsubst %.cpp,%.o,$(SRC2))
DEPENDS=$(patsubst %.cpp,%.d,$(SRC))
DEPENDS2=$(patsubst %.cpp,%.d,$(SRC2))

# Compile option
override OPTN+= -lrt
override CFLAGS+= -Wall
override CFLAGS+= -W
#override CFLAGS+= -O1
#override CFLAGS+= -O2
override CFLAGS+= -O3
#override CFLAGS+= -mtune=core2
#override CFLAGS+= -g
override CFLAGS+= -DNDEBUG
override CFLAGS+= -DNLEARN
override CFLAGS+= -DTDEBUG

#override OPTN+= -pg
#override CFLAGS+= -pg

# Compiler
GPP = g++
GCC = gcc

# Executable files
.PHONY: all
all: $(PROG)

# PGO
.PHONY: pgo
pgo:
	$(MAKE) clean
	$(MAKE) sunfish 'OPTN=-fprofile-generate' 'CFLAGS=-fprofile-generate'
	./sunfish -s -g -q
	$(MAKE) clean
	$(MAKE) sunfish 'OPTN=-fprofile-use' 'CFLAGS=-fprofile-use'
	$(MAKE)

sunfish: sunfish.o $(OBJS) SFMT/SFMT.o
	$(GPP) -o $@ $^ $(OPTN)

evcsv: evcsv.o offset.o evaluate.o data.o uint96.o SFMT/SFMT.o
	$(GPP) -o $@ $^

cut: cut.o offset.o evaluate.o data.o uint96.o SFMT/SFMT.o
	$(GPP) -o $@ $^

ins: ins.o offset.o evaluate.o data.o uint96.o SFMT/SFMT.o
	$(GPP) -o $@ $^

change: change.o offset.o evaluate.o data.o uint96.o SFMT/SFMT.o
	$(GPP) -o $@ $^

disp: disp.o offset.o evaluate.o data.o uint96.o SFMT/SFMT.o
	$(GPP) -o $@ $^

kisen2csa: kisen2csa.o $(OBJS) SFMT/SFMT.o
	$(GPP) -o $@ $^ $(OPTN)

# Object files
SFMT/SFMT.o: SFMT/SFMT.c
	make -C SFMT

.cpp.o:
	$(GPP) $(CFLAGS) -o $@ -c $<

.c.o:
	$(GCC) $(CFLAGS) -o $@ -c $<

.PHONY: clean l_init s_init depend

# Clean
clean:
	-$(RM) $(PROG) $(OBJS) $(OBJS2) $(DEPENDS) $(DEPENDS2)

# Initialization
l_init:
	-$(RM) detail evinfo pverr* pvmove* *.gr */*.select

s_init:
	-$(RM) agreement csalog.csv tdebug debug.csa edebug

# Dependence relation
%.d: %.cpp
	@set -e; $(CC) -MM $(CPPFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
		[ -s $@ ] || rm -f $@

-include $(DEPENDS) $(DEPENDS2)
