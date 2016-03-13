# Makefile
#
# qotd - A simple QOTD daemon.
# Copyright (c) 2015-2016 Ammon Smith
# 
# qotd is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
# 
# qotd is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with qotd.  If not, see <http://www.gnu.org/licenses/>.
#

.PHONY: all pdf install uninstall force clean

DEST_DIR=
GZ_FILES=qotdd.8.gz \
		 qotd.conf.5.gz
PS_FILES=qotdd.8.ps \
		 qotd.conf.5.gz
PDF_TARGET=qotd.pdf

all: $(GZ_FILES)

pdf: $(PS_FILES)
	gs -q -sPAPERSIZE=letter -dNOPAUSE -dBATCH -sDEVICE=pdfwrite \
		-sOutputFile=$(PDF_TARGET) $(PS_FILES)

%.5.ps: %.5
	groff -Tps -mandoc $< > $<.ps

%.5.gz: %.5
	gzip -c $< > $<.gz

%.8.ps: %.8
	groff -Tps -mandoc $< > $<.ps

%.8.gz: %.8
	gzip -c $< > $<.gz

install:
	for section in 5 8; do \
		for fn in *.$${section}.gz; do \
			install -D -m644 "$${fn}" "$(DESTDIR)/man$${section}/$${fn}"; \
		done \
	done

uninstall:
	for section in 5 8; do \
		for fn in *.$${section}.gz; do \
			rm -f "$(DESTDIR)/man$${section}/$${fn}" \
		done \
	done

force: clean $(GZ_FILES)

clean:
	rm -f *~ *.gz *.ps *.pdf

