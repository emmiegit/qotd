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

DEST_DIR =
SOURCE_FILES = $(wildcard *.5 *.8)
GZ_FILES = $(addsuffix .gz,$(SOURCE_FILES))
PS_FILES = $(addsuffix .ps,$(SOURCE_FILES))
PDF_TARGET = qotd.pdf

all: $(GZ_FILES)
pdf: $(PDF_TARGET)

%.5.gz: %.5
	@echo '[GZ] $@'
	@gzip -c $< > $@

%.8.gz: %.8
	@echo '[GZ] $@'
	@gzip -c $< > $@

%.5.ps: %.5
	@echo '[PS] $@'
	@groff -Tps -mandoc $< > $@

%.8.ps: %.8
	@echo '[PS] $@'
	@groff -Tps -mandoc $< > $@

$(PDF_TARGET): $(PS_FILES)
	@echo '[PDF] $@'
	@gs -q -sPAPERSIZE=letter -dNOPAUSE -dBATCH -sDEVICE=pdfwrite \
		-sOutputFile=$@ $^

install:
	@for section in 5 8; do \
		for filename in *.$${section}.gz; do \
			echo "[INSTALL] $${filename}"; \
			install -D -m644 "$${filename}" "$(DEST_DIR)/usr/share/man$${section}/$${filename}"; \
		done \
	done

uninstall:
	for section in 5 8; do \
		for filename in $(GZ_FILES); do \
			if [[ $${filename} == *.$${section} ]]; then \
				echo "[UNINSTALL] $${filename}"; \
				rm -f "$(DEST_DIR)/usr/share/man$${section}/$${filename}"; \
			fi \
		done \
	done

force: clean $(GZ_FILES)
forcepdf: clean $(PDF_FILES)

clean:
	@echo '[CLEAN]'
	@rm -f $(GZ_FILES) $(PS_FILES) $(PDF_TARGET)

