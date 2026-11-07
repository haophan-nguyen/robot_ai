#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 [--dry-run|--execute] LOCAL_FILE [USER@]HOST /REMOTE/PATH" >&2
}

execute=false
case "${1:-}" in
    --dry-run) shift ;;
    --execute) execute=true; shift ;;
    -h|--help) usage; exit 0 ;;
esac

if (( $# != 3 )); then
    usage
    exit 2
fi

local_file="$1"
ssh_target="$2"
remote_path="$3"

if [[ ! -f "$local_file" || ! -r "$local_file" ]]; then
    echo "Local artifact must be a readable regular file: $local_file" >&2
    exit 2
fi
if [[ ! "$ssh_target" =~ ^([A-Za-z0-9_][A-Za-z0-9_.-]*@)?[A-Za-z0-9][A-Za-z0-9_.-]*$ ]]; then
    echo "Use an SSH host alias, hostname, or IPv4 address, optionally preceded by user@." >&2
    exit 2
fi
# Restrict remote paths to shell-safe characters for both OpenSSH copy protocols.
if [[ ! "$remote_path" =~ ^/[A-Za-z0-9._/-]+$ || "$remote_path" == */ || "$remote_path" =~ /(\.|\.\.)($|/) ]]; then
    echo "Use an absolute remote file path with letters, digits, dots, underscores, dashes, or slashes, with no . or .. components." >&2
    exit 2
fi

local_file="$(realpath -e -- "$local_file")"
copy=(scp -p -o StrictHostKeyChecking=yes -- "$local_file" "$ssh_target:$remote_path")
if [[ "$execute" == false ]]; then
    printf 'Dry run:'
    printf ' %q' "${copy[@]}"
    printf '\n'
    exit 0
fi

exec "${copy[@]}"
