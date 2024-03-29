# Makefile
#
# qotd - A simple QOTD daemon.
# Copyright (c) 2015-2016 Emmie Smith
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

.SUFFIXES:
.PHONY: all release pdf clean

# Programs
V           ?= 0
GZIP        ?= gzip
TROFF       ?= groff
GHOSTSCRIPT ?= gs

# Print options
GZ_0  = @echo '[GZ] $@'; $(GZIP)
GZ_1  = $(GZIP)
GZ    = $(GZ_$(V))

PS_0  = @echo '[PS] $@'; $(TROFF)
PS_1  = $(TROFF)
PS    = $(PS_$(V))

PDF_0 = @echo '[PDF] $@'; $(GHOSTSCRIPT)
PDF_1 = $(GHOSTSCRIPT)
PDF   = $(PDF_$(V))

# Flags
PS_FLAGS    := -Tps -mandoc
PDF_FLAGS   := -q -sPAPERSIZE=letter -dNOPAUSE -dBATCH -sDEVICE=pdfwrite

# Documentation sources
SOURCES     := $(wildcard *.5 *.8)
GZ_FILES    := $(addsuffix .gz,$(SOURCES))
PS_FILES    := $(addsuffix .ps,$(SOURCES))
PDF_TARGET  := qotd.pdf

# Goal targets
ifeq ($(shell uname),Darwin)
all: $(GZ_FILES)
release: $(GZ_FILES)
else
all: $(GZ_FILES) $(PDF_TARGET)
release: $(GZ_FILES) $(PDF_TARGET)
pdf: $(PDF_TARGET)
endif

# Primary targets
%.5.gz: %.5
	$(GZ) -c $< > $@

%.8.gz: %.8
	$(GZ) -c $< > $@

%.5.ps: %.5
	$(PS) $(PS_FLAGS) $< > $@

%.8.ps: %.8
	$(PS) $(PS_FLAGS) $< > $@

$(PDF_TARGET): $(PS_FILES)
	$(PDF) $(PDF_FLAGS) -sOutputFile=$@ $^

# utility targets
clean:
	rm -f *.gz *.ps *.pdf

