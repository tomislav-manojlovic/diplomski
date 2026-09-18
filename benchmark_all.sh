#!/usr/bin/env bash

set -e

MACHINE="${1:-unknown}"

for script in ./scripts/run*.sh; do
	"$script" "$MACHINE"
done
