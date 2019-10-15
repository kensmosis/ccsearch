CC := /usr/bin/g++
#CFLAGS := -g3 -pg -ggdb -m64 -pthread -fPIC
#CFLAGS := -g3 -ggdb -m64 -pthread -fPIC
CFLAGS := -O3 -fPIC

OBJDIR := ./obj
DOCDIR := ./doc
SRCDIR := ./src
TMPDIR := ./tmp

OBJECTS := OCFN.o OColl.o OFeature.o OConfig.o OAPI.o OPython.o OSearch.o

DOBJECTS := $(OBJECTS:%.o=$(OBJDIR)/%.o)

tgt: $(OBJDIR)/ccslib.so

docs: $(DOCDIR)/SearchAlgo.html $(DOCDIR)/SearchAlgo.pdf $(DOCDIR)/CCSearch.html $(DOCDIR)/CCSearch.pdf

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(OBJDIR)
	-rm -f $@
	$(CC) $(CFLAGS) -I$(SRCDIR) -I /usr/include/python3.5 -c $< -o $@

$(OBJDIR)/ccslib.so: $(DOBJECTS) $(OBJDIR)
	-rm -f $@
	$(CC) -shared -fPIC $(DOBJECTS) -o $@

$(DOCDIR)/SearchAlgo.html: $(SRCDIR)/SearchAlgo.md $(DOCDIR)
	-rm -f $@
	/usr/bin/pandoc $< --verbose --fail-if-warnings --mathml --wrap=preserve --toc --toc-depth=2 --strip-comments -o $@ --metadata pagetitle="Constrained Collection Search Algorithm" -s -f markdown+grid_tables

$(DOCDIR)/SearchAlgo.pdf: $(SRCDIR)/SearchAlgo.md $(TMPDIR) $(DOCDIR)
	-rm -f $@
	-rm -f $(TMPDIR)/SearchAlgo.*
	/usr/bin/pandoc $< -o $(TMPDIR)/SearchAlgo.tex --metadata pagetitle="Constrained Collection Search Algorithm" -s -f markdown+grid_tables 
	/usr/bin/pdflatex -output-directory $(TMPDIR) $(TMPDIR)/SearchAlgo.tex 
	mv $(TMPDIR)/SearchAlgo.pdf $@

$(DOCDIR)/CCSearch.html: $(SRCDIR)/CCSearch.md $(DOCDIR)
	-rm -f $@
	/usr/bin/pandoc $< --verbose --fail-if-warnings --mathml --wrap=preserve --toc --toc-depth=2 --strip-comments -o $@ --metadata pagetitle="Constrained Collection Search Code Description" -s -f markdown+grid_tables

$(DOCDIR)/CCSearch.pdf: $(SRCDIR)/CCSearch.md $(TMPDIR) $(DOCDIR)
	-rm -f $@
	-rm -f $(TMPDIR)/CCSearch.*
	/usr/bin/pandoc $< -o $(TMPDIR)/CCSearch.tex --metadata pagetitle="Constrained Collection Search Code Description" -s -f markdown+grid_tables 
	/usr/bin/pdflatex -output-directory $(TMPDIR) $(TMPDIR)/CCSearch.tex 
	mv $(TMPDIR)/CCSearch.pdf $@

$(OBJDIR):
	mkdir $@

$(DOCDIR):
	mkdir $@

$(TMPDIR):
	mkdir $@

clean:
	-rm -f $(OBJDIR)/*.o 
	-rm -f $(OBJDIR)/ccslib.so
	-rm -f $(DOCDIR)/SearchAlgo.html
	-rm -f $(DOCDIR)/SearchAlgo.pdf
	-rm -f $(DOCDIR)/CCSearch.html
	-rm -f $(DOCDIR)/CCSearch.pdf
	-rm -rf $(TMPDIR)

