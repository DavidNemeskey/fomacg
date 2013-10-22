all: apertium_cg.pdf

clean::
	@rm -f *pdf *bbl *blg *log *aux *toc *nav *out *snm $(DEPS)

%.pdf: %.tex %.bbl
	pdflatex $<
	pdflatex $<

%.aux: %.tex
	pdflatex $<

%.bbl: %.aux
	bibtex $<

