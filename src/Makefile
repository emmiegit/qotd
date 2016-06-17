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

.PHONY: all release force debug forcedebug distclean clean

CC = gcc
FLAGS = -ansi -pipe -O3 -D_XOPEN_SOURCE=500
WARN_FLAGS = -pedantic -Wall -Wextra
INCLUDE = -I.

SRCEXT = c
OBJEXT = o
DEPEXT = d
SOURCES = $(wildcard *.$(SRCEXT))
OBJECTS = $(SOURCES:.$(SRCEXT)=.$(OBJEXT))
DEPS = $(OBJECTS:.$(OBJEXT)=.$(DEPEXT))
EXE = qotdd

all: $(EXE)

release:
	@echo '[RELEASE]'
	@make clean all EXTRA_FLAGS='-fstack-protector-all'

%.$(OBJEXT): %.$(SRCEXT)
	@echo '[CC] $@'
	@$(CC) $(FLAGS) $(WARN_FLAGS) $(INCLUDE) $(EXTRA_FLAGS) -c -o $@ $<

$(EXE): $(OBJECTS)
	@echo '[LN] $@'
	@$(CC) $(FLAGS) $(WARN_FLAGS) $(INCLUDE) $(EXTRA_FLAGS) $(OBJECTS) -o $(EXE)

force: clean $(EXE)

debug:
	@echo '[DEBUG]'
	@make $(EXE) EXTRA_FLAGS='-Og -g'

forcedebug: clean debug

distclean:
	@echo '[DISTCLEAN]'
	@rm -f *.$(OBJEXT) $(EXE) core core.* vgcore.*

clean:
	@echo '[CLEAN]'
	@rm -f *.$(OBJEXT) $(EXE)

-include: $(DEPS)
