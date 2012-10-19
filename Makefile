CXX = g++ -O0 -g -std=gnu++11
LIBS = -lboost_math_tr1
DISTDIR = dist

all: histogram groupby tabularize doc.pdf
	cp histogram $(DISTDIR)/
	cp groupby $(DISTDIR)/
	cp tabularize $(DISTDIR)/
	cp doc.pdf $(DISTDIR)/

histogram: histogram.cpp histogram.h
	$(CXX) $(LIBS) -o histogram histogram.cpp

groupby: groupby.cpp aggr.h
	$(CXX) $(LIBS) -o groupby groupby.cpp

tabularize: tabularize.cpp tabularize.h 
	$(CXX) $(LIBS) -o tabularize tabularize.cpp

doc.pdf: doc.tex
	pdflatex doc.tex 
	pdflatex doc.tex 

clean:
	rm -f $(DISTDIR)/histogram
	rm -f $(DISTDIR)/groupby
	rm -f $(DISTDIR)/tabularize
	rm -f $(DISTDIR)/doc.pdf
	rm -f histogram
	rm -f groupby 
	rm -f tabularize
	rm -f doc.aux
	rm -f doc.log
	rm -f doc.pdf
