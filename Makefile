# Makefile
#
# qotd - A simple QOTD server.
# Copyright (c) 2015 Ammon Smith
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

CC=gcc
FLAGS=-Wall -Wextra -ansi -I.
OBJECTS=arguments.o \
		qotdd.o     \
		sighandler.o

all: qotdd

%.o: %.c
	$(CC) $(FLAGS) $(EXTRA_FLAGS) -c -o $@ $<

qotdd: $(OBJECTS)
	$(CC) $(FLAGS) $(EXTRA_FLAGS) -o qotdd $(OBJECTS)


clean:
	rm -f *.o *.d *~ core

