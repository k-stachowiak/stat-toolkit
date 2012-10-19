CXX = g++ -O0 -g --std=gnu++0x
LIBS = -lboost_math_tr1 -lUnitTest++
DISTDIR = dist

# -------------
# Main targets.
# -------------

all: test cli doc

test: aggr_test histogram_test
	./aggr_test
	./histogram_test

cli: aggr histogram groupby tabularize 
	cp aggr $(DISTDIR)/
	cp histogram $(DISTDIR)/
	cp groupby $(DISTDIR)/
	cp tabularize $(DISTDIR)/

doc: doc.pdf
	cp doc.pdf $(DISTDIR)/

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
	rm -f $(DISTDIR)/tabularize
	rm -f aggr
	rm -f histogram
	rm -f groupby 
	rm -f tabularize

clean_doc:
	rm -f $(DISTDIR)/doc.pdf
	rm -f doc.aux
	rm -f doc.log
	rm -f doc.pdf

# ---------------------------------
# The command line interface tools.
# ---------------------------------

aggr: aggr.cpp aggr.h
	$(CXX) $(LIBS) -o aggr aggr.cpp

histogram: histogram.cpp histogram.h
	$(CXX) $(LIBS) -o histogram histogram.cpp

groupby: groupby.cpp aggr.h
	$(CXX) $(LIBS) -o groupby groupby.cpp

tabularize: tabularize.cpp tabularize.h 
	$(CXX) $(LIBS) -o tabularize tabularize.cpp

# ------
# Tests.
# ------

aggr_test: aggr_test.cpp aggr.h
	$(CXX) -o aggr_test aggr_test.cpp $(LIBS)

histogram_test: histogram_test.cpp histogram.h
	$(CXX) -o histogram_test histogram_test.cpp $(LIBS)

# --------------
# Documentation.
# --------------

doc.pdf: doc.tex
	pdflatex doc.tex 
	pdflatex doc.tex 

