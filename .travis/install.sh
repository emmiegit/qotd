#!/bin/bash
set -euo pipefail

case "$TRAVIS_OS_NAME" in
	linux)
		sudo apt install ghostscript
		;;
esac

