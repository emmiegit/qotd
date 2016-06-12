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
SOURCE_FILES = $(wildcard *.5 *.8)
GZ_FILES = $(addsuffix .gz,$(SOURCE_FILES))
PS_FILES = $(addsuffix .ps,$(SOURCE_FILES))
PDF_TARGET = qotd.pdf

all: $(GZ_FILES)
pdf: $(PDF_TARGET)

$(PS_FILES): $(SOURCE_FILES)
	groff -Tps -mandoc $< > $@

$(PDF_TARGET): $(PS_FILES)
	gs -q -sPAPERSIZE=letter -dNOPAUSE -dBATCH -sDEVICE=pdfwrite \
		-sOutputFile=$@ $^

$(GZ_FILES): $(SOURCE_FILES)
	gzip -kf $<

install:
	for section in 5 8; do \
		for filename in *.$${section}.gz; do \
			install -D -m644 "$${fn}" "$(DEST_DIR)/man$${section}/$${filename}"; \
		done \
	done

uninstall:
	for section in 5 8; do \
		for filename in $(GZ_FILES); do \
			if [[ $${filename} == *.$${section} ]]; then \
				rm -f "$(DEST_DIR)/man$${section}/$${filename}"; \
			fi \
		done \
	done

force: clean $(GZ_FILES)
forcepdf: clean $(PDF_FILES)

clean:
	rm -f $(GZ_FILES) $(PS_FILES) $(PDF_TARGET)

