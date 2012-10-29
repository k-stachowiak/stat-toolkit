CXX = g++ -O0 -g --std=gnu++0x
LIBS = -lboost_math_tr1
DISTDIR = dist

.PHONY: all test cli doc

default: cli

# -------------
# Main targets.
# -------------

xfiles: xfiles.cpp
	$(CXX) -o xfiles xfiles.cpp

all: test cli doc

test: aggr_test histogram_test groupby_test aggr_array_test

cli: aggr histogram groupby tabularize pivot
	cp aggr $(DISTDIR)/
	cp histogram $(DISTDIR)/
	cp groupby $(DISTDIR)/
	cp tabularize $(DISTDIR)/
	cp pivot $(DISTDIR)/

doc: manual reference

# --------------
# Clean targets.
# --------------

clean: clean_test clean_cli clean_doc

clean_test:
	rm -f aggr_test
	rm -f histogram_test
	rm -f groupby_test

clean_cli:
	rm -f $(DISTDIR)/aggr
	rm -f $(DISTDIR)/histogram
	rm -f $(DISTDIR)/groupby
	rm -f $(DISTDIR)/tabularize
	rm -f $(DISTDIR)/pivot
	rm -f aggr
	rm -f histogram
	rm -f groupby 
	rm -f tabularize
	rm -f pivot

clean_doc:
	rm -f $(DISTDIR)/manual.pdf
	rm -f manual.aux
	rm -f manual.toc
	rm -f manual.log
	rm -f manual.pdf
	rm -f $(DISTDIR)/reference.pdf
	rm -f reference.pdf

# ---------------------------------
# The command line interface tools.
# ---------------------------------

aggr: aggr.cpp aggr.h
	$(CXX) $(LIBS) -o aggr aggr.cpp

histogram: histogram.cpp histogram.h
	$(CXX) $(LIBS) -o histogram histogram.cpp

groupby: groupby.cpp groupby.h aggr.h util.h
	$(CXX) $(LIBS) -o groupby groupby.cpp

tabularize: tabularize.cpp tabularize.h 
	$(CXX) $(LIBS) -o tabularize tabularize.cpp

pivot: pivot.cpp aggr_array.h util.h aggr.h
	$(CXX) $(LIBS) -o pivot pivot.cpp

# ------
# Tests.
# ------

aggr_test: aggr_test.cpp aggr.h
	$(CXX) -o aggr_test aggr_test.cpp $(LIBS) -lUnitTest++
	./aggr_test

histogram_test: histogram_test.cpp histogram.h
	$(CXX) -o histogram_test histogram_test.cpp $(LIBS) -lUnitTest++
	./histogram_test

groupby_test: groupby_test.cpp groupby.h aggr.h util.h
	$(CXX) -o groupby_test groupby_test.cpp $(LIBS) -lUnitTest++
	./groupby_test
	
aggr_array_test: aggr_array_test.cpp aggr_array.h util.h aggr.h
	$(CXX) -o aggr_array_test aggr_array_test.cpp $(LIBS) -lUnitTest++
	./aggr_array_test

# --------------
# Documentation.
# --------------

manual: manual.tex
	pdflatex manual.tex 
	pdflatex manual.tex 
	cp manual.pdf $(DISTDIR)/

reference: FORCE
	doxygen doxycfg
	cd reference && $(MAKE)
	cp reference/refman.pdf reference.pdf
	cp reference.pdf $(DISTDIR)/

FORCE:
