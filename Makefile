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

.PHONY: all release src man install-no-systemd install force debug forcedebug distclean clean

EXE=qotdd
DESTDIR=
MAN_DESTDIR=$(DESTDIR)/usr/share/man

all: src man

release: man
	make -C src release
	cp -f 'src/$(EXE)' .

$(EXE): src

src:
	make -C src
	cp -f 'src/$(EXE)' .

man:
	make -C man

install-no-systemd: $(EXE) man
	install -D -m755 qotdd $(DESTDIR)/usr/bin/qotdd
	install -D -m644 misc/qotd.conf $(DESTDIR)/etc/qotd.conf
	install -D -m644 misc/quotes.txt $(DESTDIR)/usr/share/qotd/quotes.txt
	make -C man install DESTDIR=$(MAN_DESTDIR)

install: install-no-systemd
	install -D -m644 misc/qotd.service $(DESTDIR)/usr/lib/systemd/system/qotd.service

force:
	make -C src force
	cp -f src/$(EXE) .

debug:
	make -C src debug
	cp -f src/$(EXE) .

forcedebug:
	make -C src forcedebug
	cp -f src/$(EXE) .

distclean:
	make -C src distclean
	rm -f $(EXE)

clean:
	make -C src clean
	rm -f $(EXE)

