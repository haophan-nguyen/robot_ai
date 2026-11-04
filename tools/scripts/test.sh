#!/usr/bin/env bash
set -euo pipefail

if (( $# != 0 )); then
    echo "Usage: $0" >&2
    exit 2
fi

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"

bash "$repo_root/tools/scripts/build.sh"
ctest --test-dir "$repo_root/build" --output-on-failure --no-tests=error
