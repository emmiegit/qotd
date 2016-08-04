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

.PHONY: all pdf install clean

# Constant declarations
V            := 0
PROGRAM_NAME := qotd
VERSION      := 0.9
EXE          := qotdd

# Directories
SRC_DIR      := src
MAN_DIR      := man

export		 V PROGRAM_NAME VERSION EXE

# Goal Targets
all:
	@make -C $(SRC_DIR)

release: update-versions
	@echo '[RELEASE]'
	@make -BC $(SRC_DIR) release

debug:
	@echo '[DEBUG]'
	@make -C $(SRC_DIR) debug

profile:
	@echo '[PROFILE]'
	@make -C $(SRC_DIR) profile

man:
	@make -C $(MAN_DIR) all

pdf:
	@make -C $(MAN_DIR) pdf

clean:
	@echo '[CLEAN]'
	@make -C $(SRC_DIR) clean
	@make -C $(MAN_DIR) clean

# Primary targets
update-versions:
	@sed -i '1 s/"$(PROGRAM_NAME) [^"]*"/"$(PROGRAM_NAME) $(VERSION)"/' $(wildcard $(MAN_DIR)/*.5 $(MAN_DIR)/*.8)
	@sed -i 's/VERSION_STRING .*/VERSION_STRING "$(VERSION)"/' $(SRC_DIR)/info.h

install:
	@echo '[INSTALL] $(ROOT)/usr/bin/qotdd'
	@install -D -m755 src/$(EXE) '$(ROOT)/usr/bin/qotdd'

	@echo '[INSTALL] $(ROOT)/etc/qotd.conf'
	@install -D -m644 misc/qotd.conf '$(ROOT)/etc/qotd.conf'

	@echo '[INSTALL] $(ROOT)/usr/share/qotd/quotes.txt'
	@install -D -m644 misc/quotes.txt '$(ROOT)/usr/share/qotd/quotes.txt'

ifeq ($(SYSTEMD),1)
	@echo '[INSTALL] $(ROOT)/usr/lib/systemd/system/qotd.service'
	@install -D -m644 misc/qotd.service '$(ROOT)/usr/lib/systemd/system/qotd.service'
endif

	@cd $(MAN_DIR); \
	for section in 5 8; do \
		for filename in *.$${section}.gz; do \
			echo "[INSTALL] $(ROOT)/usr/share/man/man$${section}/$${filename}"; \
			install -D -m644 "$${filename}" "$(ROOT)/usr/share/man/man$${section}/$${filename}"; \
		done \
	done

