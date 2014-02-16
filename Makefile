CXX = g++ -O2 --std=c++11
HC = ghc --make
LIBS = -lboost_math_tr1
DISTDIR = dist
TEX = pdflatex
TEXLOG = latex.log

.PHONY: all test cli doc

default: cli

# -------------
# Main targets.
# -------------

xfiles: xfiles.cpp
	$(CXX) -o xfiles xfiles.cpp

all: test cli doc
	./check.sh
	./loc.sh

test: aggr_test histogram_test

cli: aggr histogram groupby pivot
	cp aggr $(DISTDIR)/
	cp histogram $(DISTDIR)/
	cp groupby $(DISTDIR)/
	cp pivot $(DISTDIR)/
	cp LICENSE $(DISTDIR)/

doc: manual

# --------------
# Clean targets.
# --------------

clean: clean_test clean_cli clean_doc

clean_test:
	rm -f aggr_test
	rm -f histogram_test

clean_cli:
	rm -f $(DISTDIR)/aggr
	rm -f $(DISTDIR)/histogram
	rm -f $(DISTDIR)/groupby
	rm -f $(DISTDIR)/pivot
	rm -f $(DISTDIR)/LICENSE
	rm -f aggr
	rm -f histogram
	rm -f groupby 
	rm -f pivot
	rm -f xfiles
	rm -f *.o *.hi

clean_doc:
	rm -f $(DISTDIR)/manual.pdf
	rm -f $(DISTDIR)/FDL
	rm -f manual.aux
	rm -f manual.toc
	rm -f manual.log
	rm -f manual.pdf

# ---------------------------------
# The command line interface tools.
# ---------------------------------

aggr: aggr.cpp aggr.h
	$(CXX) $(LIBS) -o aggr aggr.cpp

histogram: histogram.cpp histogram.h
	$(CXX) $(LIBS) -o histogram histogram.cpp

groupby: groupby.cpp groupby.h aggr.h util.h
	$(CXX) $(LIBS) -o groupby groupby.cpp

pivot: pivot.cpp util.h groupby.h aggr.h
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

# --------------
# Documentation.
# --------------

manual: manual.tex
	rm -f $(TEXLOG)
	$(TEX) manual.tex > $(TEXLOG)
	$(TEX) manual.tex > $(TEXLOG)
	cp manual.pdf $(DISTDIR)/
	cp FDL $(DISTDIR)/

