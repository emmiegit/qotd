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

.PHONY: all force debug forcedebug distclean clean

CC=gcc
FLAGS=-ansi -I. -pthread -Wall -Wextra
OBJECTS=arguments.o \
		config.o    \
		qotdd.o     \
		quotes.o    \
		sighandler.o
EXE=qotdd

all: $(EXE)

%.o: %.c
	$(CC) $(FLAGS) $(EXTRA_FLAGS) -c $< -o $@

$(EXE): $(OBJECTS)
	$(CC) $(FLAGS) $(EXTRA_FLAGS) $(OBJECTS) -o $(EXE)

force: clean $(EXE)

debug:
	make $(EXE) EXTRA_FLAGS='-g'

forcedebug: clean debug

distclean:
	rm -f *.o *.d *~ $(EXE) core core.* vgcore.*

clean:
	rm -f *.o $(EXE)

