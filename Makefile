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

.PHONY: all pdf install force forcedebug clean

# Constant declarations
PROGRAM_NAME = qotd
VERSION = 0.7

# Directories
SRC_DIR = src
MAN_DIR = man
BIN_DIR = bin
DOC_DIR = doc

# Compile options
CC = gcc
FLAGS = -ansi -pipe -O3 -D_XOPEN_SOURCE=500
WARN_FLAGS = -pedantic -Wall -Wextra
INCLUDE = -I.

# Program sources
SRC_EXT = c
HDR_EXT = h
OBJ_EXT = o
DEP_EXT = d
SOURCES = $(wildcard $(SRC_DIR)/*.$(SRC_EXT))
OBJECTS = $(patsubst $(SRC_DIR)/%,$(BIN_DIR)/%,$(SOURCES:.$(SRC_EXT)=.$(OBJ_EXT)))
DEPS = $(OBJECTS:.$(HDR_EXT)=.$(DEP_EXT))
EXE = $(BIN_DIR)/qotdd

# Man pages
MAN_SOURCES = $(wildcard $(MAN_DIR)/*.5 $(MAN_DIR)/*.8)
GZ_FILES = $(patsubst $(MAN_DIR)/%,$(DOC_DIR)/%,$(addsuffix .gz,$(MAN_SOURCES)))
PS_FILES = $(patsubst $(MAN_DIR)/%,$(DOC_DIR)/%,$(addsuffix .ps,$(MAN_SOURCES)))
PDF_TARGET = $(DOC_DIR)/$(PROGRAM_NAME).pdf

# Targets
all: $(EXE) $(GZ_FILES) $(PDF_TARGET)
pdf: $(PDF_TARGET)

# Program targets
update-versions:
	@sed -i '1 s/"$(PROGRAM_NAME) [^"]*"/"$(PROGRAM_NAME) $(VERSION)"/' $(MAN_SOURCES)
	@sed -i 's/VERSION_STRING .*/VERSION_STRING "$(VERSION)"/' $(SRC_DIR)/info.$(HDR_EXT)
	@make

release: update-versions
	@echo '[RELEASE]'
	@make clean all EXTRA_FLAGS='-fstack-protector-all'

$(BIN_DIR)/%.$(OBJ_EXT): $(SRC_DIR)/%.$(SRC_EXT)
	@mkdir -p $(BIN_DIR)
	@echo '[CC] $(notdir $@)'
	@$(CC) $(FLAGS) $(WARN_FLAGS) $(INCLUDE) $(EXTRA_FLAGS) -c -o $@ $<

$(EXE): $(OBJECTS)
	@echo '[LD] $(notdir $@)'
	@$(CC) $(FLAGS) $(WARN_FLAGS) $(INCLUDE) $(EXTRA_FLAGS) -o $(EXE) $^

install:
	@echo '[INSTALL] $$ROOT/usr/bin/qotdd'
	@install -D -m755 $(EXE) '$(ROOT)/usr/bin/qotdd'

	@echo '[INSTALL] $$ROOT/etc/qotd.conf'
	@install -D -m644 misc/qotd.conf '$(ROOT)/etc/qotd.conf'

	@echo '[INSTALL] $$ROOT/usr/share/qotd/quotes.txt'
	@install -D -m644 misc/quotes.txt '$(ROOT)/usr/share/qotd/quotes.txt'

ifeq ($(SYSTEMD),1)
	@echo '[INSTALL] $$ROOT/usr/lib/systemd/system/qotd.service'
	@install -D -m644 misc/qotd.service '$(ROOT)/usr/lib/systemd/system/qotd.service'
endif

	@cd $(DOC_DIR); \
	for section in 5 8; do \
		for filename in *.$${section}.gz; do \
			echo "[INSTALL] $(ROOT)/usr/share/man/man$${section}/$${filename}"; \
			install -D -m644 "$${filename}" "$(ROOT)/usr/share/man/man$${section}/$${filename}"; \
		done \
	done

# Documentation targets
$(DOC_DIR)/%.5.gz: $(MAN_DIR)/%.5
	@mkdir -p $(DOC_DIR)
	@echo '[GZ] $(notdir $@)'
	@gzip -c $< > $@

$(DOC_DIR)/%.8.gz: $(MAN_DIR)/%.8
	@mkdir -p $(DOC_DIR)
	@echo '[GZ] $(notdir $@)'
	@gzip -c $< > $@

$(DOC_DIR)/%.5.ps: $(MAN_DIR)/%.5
	@mkdir -p $(DOC_DIR)
	@echo '[PS] $(notdir $@)'
	@groff -Tps -mandoc $< > $@

$(DOC_DIR)/%.8.ps: $(MAN_DIR)/%.8
	@mkdir -p $(DOC_DIR)
	@echo '[PS] $(notdir $@)'
	@groff -Tps -mandoc $< > $@

$(PDF_TARGET): $(PS_FILES)
	@echo '[PDF] $(notdir $@)'
	@gs -q -sPAPERSIZE=letter -dNOPAUSE -dBATCH -sDEVICE=pdfwrite \
		-sOutputFile=$@ $^

# Utility targets
debug:
	@echo '[DEBUG]'
	@make $(EXE) EXTRA_FLAGS='-g -Og -DDEBUG=1'

clean:
	@echo '[RMDIR] $(BIN_DIR)'
	@rm -rf '$(BIN_DIR)'

	@echo '[RMDIR] $(DOC_DIR)'
	@rm -rf '$(DOC_DIR)'

-include: $(DEPS)
