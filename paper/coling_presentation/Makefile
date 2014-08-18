all: coling_presentation.pdf

clean::
	@rm -f *pdf *bbl *blg *log *aux *toc *nav *out *snm $(DEPS)

%.pdf: %.tex
	pdflatex $<
	pdflatex $<
