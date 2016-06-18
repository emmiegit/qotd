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
OBJ_EXT = o
DEP_EXT = d
SOURCES = $(wildcard $(SRC_DIR)/*.$(SRC_EXT))
OBJECTS = $(patsubst $(SRC_DIR)/%,$(BIN_DIR)/%,$(SOURCES:.$(SRC_EXT)=.$(OBJ_EXT)))
DEPS = $(OBJECTS:.$(OBJ_EXT)=.$(DEP_EXT))
EXE = $(BIN_DIR)/qotdd

# Man pages
MAN_SOURCES = $(wildcard $(MAN_DIR)/*.5 $(MAN_DIR)/*.8)
GZ_FILES = $(patsubst $(MAN_DIR)/%,$(DOC_DIR)/%,$(addsuffix .gz,$(MAN_SOURCES)))
PS_FILES = $(patsubst $(MAN_DIR)/%,$(DOC_DIR)/%,$(addsuffix .ps,$(MAN_SOURCES)))
PDF_TARGET = $(DOC_DIR)/qotd.pdf

# Targets
all: $(BUILD_DIR) $(DOC_DIR) $(EXE) $(GZ_FILES) $(PDF_TARGET)
pdf: $(PDF_TARGET)

# Program targets
release:
	@echo '[RELEASE]'
	@make clean all EXTRA_FLAGS='-fstack-protector-all'

$(BIN_DIR) $(DOC_DIR):
	@echo "[MKDIR] $$(basename $@)'
	@mkdir -p $@

$(BIN_DIR)/%.$(OBJ_EXT): $(BIN_DIR) $(SRC_DIR)/%.$(SRC_EXT)
	@echo "[CC] $$(basename $@)"
	@$(CC) $(FLAGS) $(WARN_FLAGS) $(INCLUDE) $(EXTRA_FLAGS) -c -o $@ $(word 2,$^)

$(EXE): $(OBJECTS)
	@echo "[LN] $$(basename $@)"
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
	@install -D -m644 misc/qotd.service '$(ROOT)/usr/libPseudPseudo.service'
endif

	@cd $(DOC_DIR); \
	for section in 5 8; do \
		for filename in *.$${section}.gz; do \
			echo "[INSTALL] $(ROOT)/usr/share/man/$${section}/$${filename}"; \
			install -D -m644 "$${filename}" "$(ROOT)/usr/share/man$${section}/$${filename}"; \
		done \
	done

# Documentation targets
$(DOC_DIR)/%.5.gz: $(DOC_DIR) $(MAN_DIR)/%.5
	@echo '[GZ] $@'
	@gzip -c $(word 2,$^) > $@

$(DOC_DIR)/%.8.gz: $(DOC_DIR) $(MAN_DIR)/%.8
	@echo '[GZ] $@'
	@gzip -c $(word 2,$^) > $@

$(DOC_DIR)/%.5.ps: $(DOC_DIR) $(MAN_DIR)/%.5
	@echo '[PS] $@'
	@groff -Tps -mandoc $(word 2,$^) > $@

$(DOC_DIR)/%.8.ps: $(DOC_DIR) $(MAN_DIR)/%.8
	@echo '[PS] $@'
	@groff -Tps -mandoc $(word 2,$^) > $@

$(PDF_TARGET): $(PS_FILES)
	@echo '[PDF] $@'
	@gs -q -sPAPERSIZE=letter -dNOPAUSE -dBATCH -sDEVICE=pdfwrite \
		-sOutputFile=$@ $^

# Pseudo targets
force: clean all
forcedebug: clean debug

debug:
	@echo '[DEBUG]'
	@make $(EXE) EXTRA_FLAGS='-g -Og'

clean:
	@echo '[RMDIR] $(BIN_DIR)'
	@rm -rf '$(BIN_DIR)'

	@echo '[RMDIR] $(DOC_DIR)'
	@rm -rf '$(DOC_DIR)'

-include: $(DEPS)
