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
VERSION      := 0.7

# Directories
SRC_DIR      := src
MAN_DIR      := man

# Goal Targets
all:
	@make -C $(SRC_DIR) V=$(V)

release: update-versions
	@echo '[RELEASE]'
	@make -C $(SRC_DIR) release V=$(V)

debug:
	@echo '[DEBUG]'
	@make -C $(SRC_DIR) debug V=$(V)

profile:
	@echo '[PROFILE]'
	@make -C $(SRC_DIR) profile V=$(V)

man:
	@make -C $(MAN_DIR) all V=$(V)

pdf:
	@make -C $(MAN_DIR) pdf V=$(V)

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
	@install -D -m755 $(EXE) '$(ROOT)/usr/bin/qotdd'

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

