#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 --sysroot DIR [--build-dir DIR] [--target NAME] [-- CMAKE_ARGS...]" >&2
}

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
toolchain="$repo_root/tools/cmake/arm-linux-gnueabihf.cmake"
sysroot=""
build_dir="$repo_root/build-armhf"
target_args=()

while (( $# > 0 )); do
    case "$1" in
        --sysroot|--build-dir|--target)
            if (( $# < 2 )) || [[ -z "$2" || "$2" == -* ]]; then
                usage
                exit 2
            fi
            case "$1" in
                --sysroot) sysroot="$2" ;;
                --build-dir) build_dir="$2" ;;
                --target) target_args=(--target "$2") ;;
            esac
            shift 2
            ;;
        --)
            shift
            break
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            usage
            exit 2
            ;;
    esac
done

if [[ ! -d "$sysroot" ]]; then
    echo "Provide an existing armhf sysroot with --sysroot DIR." >&2
    exit 2
fi
sysroot="$(realpath -e -- "$sysroot")"
build_dir="$(realpath -m -- "$build_dir")"

cache="$build_dir/CMakeCache.txt"
if [[ -f "$cache" ]] && {
    ! grep -Fxq "CMAKE_TOOLCHAIN_FILE:FILEPATH=$toolchain" "$cache" ||
    ! grep -Fxq "AI_ROBOT_ARMHF_SYSROOT:PATH=$sysroot" "$cache"
}; then
    echo "Build directory uses a different toolchain or sysroot. Choose a fresh --build-dir." >&2
    exit 2
fi

cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF "$@" \
    -S "$repo_root" -B "$build_dir" \
    "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=$toolchain" \
    "-DAI_ROBOT_ARMHF_SYSROOT:PATH=$sysroot"
cmake --build "$build_dir" --parallel "${target_args[@]}"
