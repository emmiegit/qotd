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

.PHONY: all check install-no-systemd install debug force distclean clean

CC=gcc
FLAGS=-ansi -I. -Wall -Wextra
OBJECTS=arguments.o \
		config.o    \
		qotdd.o     \
		quotes.o    \
		sighandler.o
EXE=qotdd
DESTDIR=

all: $(EXE)

%.o: %.c
	$(CC) $(FLAGS) $(EXTRA_FLAGS) -c -o $@ $<

$(EXE): $(OBJECTS)
	$(CC) $(FLAGS) $(EXTRA_FLAGS) -o $(EXE) $(OBJECTS)

check:
	[ -f "$(EXE)" ]
	[ -f quotes.txt ]
	[ -f qotd.conf ]
	[ -f qotd.service ]

install-no-systemd: $(EXE)
	install -D -m644 qotd.conf $(DESTDIR)/etc/qotd.conf
	install -D -m755 qotdd $(DESTDIR)/usr/bin/qotdd
	install -D -m644 quotes.txt $(DESTDIR)/usr/share/qotd/quotes.txt

install: install-no-systemd
	install -D -m644 qotd.service $(DESTDIR)/usr/lib/systemd/system/qotd.service

debug: clean
	make $(EXE) EXTRA_FLAGS='-g'

force: clean $(EXE)

distclean:
	rm -f *.o *.d *~ $(EXE) core core.* vgcore.*

clean:
	rm -f *.o $(EXE)

