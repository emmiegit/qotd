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

EXE = src/qotdd
DEST_DIR=

all: src man

release: man
	make -C src release

$(EXE): src

src:
	make -C src

man:
	make -C man

install-no-systemd: $(EXE) man
	install -D -m755 qotdd $(DEST_DIR)/usr/bin/qotdd
	install -D -m644 misc/qotd.conf $(DEST_DIR)/etc/qotd.conf
	install -D -m644 misc/quotes.txt $(DEST_DIR)/usr/share/qotd/quotes.txt
	@make -C man install DEST_DIR=$(DEST_DIR)

install: install-no-systemd
	install -D -m644 misc/qotd.service $(DEST_DIR)/usr/lib/systemd/system/qotd.service

force:
	make -C src force

debug:
	make -C src debug

forcedebug:
	make -C src forcedebug

distclean:
	make -C src distclean
	rm -f $(EXE)

clean:
	make -C src clean
	rm -f $(EXE)

