CXX = g++ -O0 -g --std=gnu++0x
LIBS = -lboost_math_tr1 -lUnitTest++
DISTDIR = dist

.PHONY: all test cli doc

# -------------
# Main targets.
# -------------

all: test cli

test: aggr_test histogram_test groupby_test

cli: aggr histogram groupby tabularize 
	cp aggr $(DISTDIR)/
	cp histogram $(DISTDIR)/
	cp groupby $(DISTDIR)/
	cp tabularize $(DISTDIR)/

doc: manual.pdf reference.pdf
	cp manual.pdf $(DISTDIR)/
	cp reference.pdf $(DISTDIR)/

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
	rm -f aggr
	rm -f histogram
	rm -f groupby 
	rm -f tabularize

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

groupby: groupby.cpp groupby.h aggr.h
	$(CXX) $(LIBS) -o groupby groupby.cpp

tabularize: tabularize.cpp tabularize.h 
	$(CXX) $(LIBS) -o tabularize tabularize.cpp

# ------
# Tests.
# ------

aggr_test: aggr_test.cpp aggr.h
	$(CXX) -o aggr_test aggr_test.cpp $(LIBS)
	./aggr_test

histogram_test: histogram_test.cpp histogram.h
	$(CXX) -o histogram_test histogram_test.cpp $(LIBS)
	./histogram_test

groupby_test: groupby_test.cpp groupby.h aggr.h
	$(CXX) -o groupby_test groupby_test.cpp $(LIBS)
	./groupby_test

# --------------
# Documentation.
# --------------

manual.pdf: manual.tex
	pdflatex manual.tex 
	pdflatex manual.tex 

reference.pdf: FORCE
	doxygen doxycfg
	cd reference && $(MAKE)
	cp reference/refman.pdf reference.pdf

FORCE:
