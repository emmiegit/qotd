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

.PHONY: all clean

# Print options
GCC_0 = @echo '[CC] $@'; $(CC)
GCC_1 = $(CC)
GCC   = $(GCC_$(V))

LD_0  = @echo '[LD] $@'; $(CC)
LD_1  = $(CC)
LD    = $(LD_$(V))

DEP_0 = @echo '[DEP] $@'; $(CC)
DEP_1 = $(CC)
DEP   = $(DEP_$(V))

# Compile options
CC      := gcc
FLAGS   := -ansi -pipe
WARN    := -pedantic -Wall -Wextra
COMPILE := -I. -D_XOPEN_SOURCE=500 -DGITHASH='"$(shell git rev-parse --short HEAD)"'
LINKING :=

# Program sources
SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:.c=.o)
DEPENDS := $(SOURCES:.c=.d)
EXE     := qotdd

# Goal targets
all: CFLAGS += -O3
all: $(EXE)

release: CFLAGS += -fstack-protector-all
release: $(EXE)

debug: CFLAGS += -g -DDEBUG=1
debug: $(EXE)

profile: CFLAGS += -g -fprofile-arcs -ftest-coverage
profile: $(EXE)

# Primary targets
$(EXE): $(OBJECTS)
	$(LD) $(FLAGS) $(CFLAGS) $(LINKING) -o $@ $^

%.o: %.c
	$(GCC) $(FLAGS) $(CFLAGS) $(WARN) $(COMPILE) -c -o $@ $<

%.d: %.c
	$(DEP) $(CFLAGS) -MM $< > $@

# Utility targets
clean:
	rm -f *.o *.d *.gcda *.gcno *gcov

-include $(DEPENDS)

