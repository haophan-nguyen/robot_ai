# Raspberry Pi ARM cross-compilation

The target for this setup is a Raspberry Pi 3 Model B running 32-bit Raspbian 12 (Bookworm). Its userspace is little-endian ARM EABI5 hard-float (`armhf`) with glibc 2.36 and loader `/lib/ld-linux-armhf.so.3`. Check the installed OS and userspace ABI before reusing this toolchain for another target.

```bash
# Read-only checks in an authorized Pi session
cat /etc/os-release
uname -srmo
dpkg --print-architecture
getconf LONG_BIT
getconf GNU_LIBC_VERSION
readelf -h -A -l /bin/sh
dpkg-query -W libc6 libstdc++6 libgcc-s1 libasound2 libsndfile1
```

## Install the WSL tools

On Ubuntu 22.04 WSL, install the GNU Linux ARM hard-float compiler, binutils, and the host build tools:

```bash
sudo apt-get update
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf \
    binutils-arm-linux-gnueabihf cmake make pkg-config openssh-client
arm-linux-gnueabihf-g++ -dumpmachine
arm-linux-gnueabihf-g++ -dumpfullversion
arm-linux-gnueabihf-ld --version
```

Task 1 verified `arm-linux-gnueabihf` with GCC/G++ 11.4.0, binutils 2.38, CMake 3.22.1, and pkg-config 0.29.2. The compiler runs on WSL and targets ARMv7-A with VFPv3-D16 and hard-float calling conventions. Choose the triplet based on the OS ABI, even if the processor also supports 64-bit instructions. [Ubuntu GCC cross package](https://packages.ubuntu.com/en/jammy/gcc-arm-linux-gnueabihf) · [GCC ARM options](https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html)

## Prepare the target sysroot

Reuse the Task 1 sysroot if it is present. The scripts accept any compatible sysroot path:

```bash
sysroot="$HOME/.local/share/robot-ai/toolchains/raspbian-bookworm-armhf/sysroot"
```

For a fresh setup, use an isolated, download-only APT cache in WSL with architecture `armhf`, suite `bookworm`, and the signed Raspbian and Raspberry Pi archives. Use trusted archive keyrings from the target or an official image. Task 1 used `http://raspbian.raspberrypi.com/raspbian` and `https://archive.raspberrypi.com/debian` with a separate `signed-by` keyring for each. The sysroot contains extracted target packages. WSL's own system libraries and the Pi's package installation are separate. [Raspberry Pi OS package sources](https://www.raspberrypi.com/documentation/computers/os.html)

| Packages | Task 1 versions and purpose |
| --- | --- |
| `libc6`, `libc6-dev`, `libc-dev-bin`, `linux-libc-dev` | Pi glibc revision `2.36-9+rpt2+deb12u13` and Bookworm kernel headers. Provides the loader, libc, development headers, and startup files. |
| `libstdc++6`, `libgcc-s1` | Pi revision `12.2.0-14+rpi1+deb12u1`. Supplies the target C++ and GCC runtimes. |
| `libasound2`, `libasound2-dev` | `1.2.8-1+rpt1`. Provides ALSA headers, shared library, and `alsa.pc`. |
| `libsndfile1`, `libsndfile1-dev` | `1.2.0-1+deb12u1`. Provides libsndfile headers, shared library, `sndfile.pc`, and dependencies on codec libraries. |

Use the observed target package versions for the loader and runtimes. If a newer patch replaced the installed glibc in the current index, obtain the exact version from the official archive and check its SHA-256 against authenticated APT metadata. Task 1 recovered the three exact glibc packages from the Raspberry Pi archive pool that way. Keep the dependencies APT resolves, including the libsndfile codec packages. The Task 1 sysroot has a `package-manifest.tsv` listing all 43 extracted packages.

For a fresh cache, prepare the paths below as your WSL user. Place the trusted public keyrings at `$apt_dir/keyrings/raspbian.gpg` and `$apt_dir/keyrings/raspberrypi.gpg`, and the three verified glibc packages in `$apt_dir/pinned`.

```bash
apt_dir="$(dirname -- "$sysroot")/apt"
mkdir -p "$apt_dir/keyrings" "$apt_dir/pinned"
printf 'deb [arch=armhf signed-by=%s] http://raspbian.raspberrypi.com/raspbian bookworm main\n' \
    "$apt_dir/keyrings/raspbian.gpg" > "$apt_dir/sources.list"
printf 'deb [arch=armhf signed-by=%s] https://archive.raspberrypi.com/debian bookworm main\n' \
    "$apt_dir/keyrings/raspberrypi.gpg" >> "$apt_dir/sources.list"
```

After placing the keyrings and pinned packages, download and extract the target packages:

```bash
mkdir -p "$apt_dir/lists/partial" "$apt_dir/cache/archives/partial"
touch "$apt_dir/status"
apt_opts=(-o APT::Architecture=armhf -o Dir::Etc::sourcelist="$apt_dir/sources.list"
    -o Dir::Etc::sourceparts=- -o Dir::State::status="$apt_dir/status"
    -o Dir::State::lists="$apt_dir/lists" -o Dir::Cache="$apt_dir/cache"
    -o Dir::Cache::archives="$apt_dir/cache/archives" -o Debug::NoLocking=true)
apt-get "${apt_opts[@]}" update
apt-get "${apt_opts[@]}" --download-only --no-install-recommends -y install \
    "$apt_dir"/pinned/*.deb libasound2-dev=1.2.8-1+rpt1 \
    libsndfile1-dev=1.2.0-1+deb12u1 libstdc++6=12.2.0-14+rpi1+deb12u1 \
    libgcc-s1=12.2.0-14+rpi1+deb12u1

# Use a fresh sysroot directory.
mkdir "$sysroot"
for package in "$apt_dir"/pinned/*.deb "$apt_dir"/cache/archives/*.deb; do
    dpkg-deb -x "$package" "$sysroot"
done
```

Make absolute library symlinks relative to their targets inside the sysroot. The Task 1 setup also added local linker names for the target C++ and GCC runtimes. For a fresh extraction:

```bash
while IFS= read -r -d '' link; do
    target="$sysroot$(readlink -- "$link")"
    [[ -e "$target" ]] || continue
    relative="$(realpath -s --relative-to="$(dirname -- "$link")" -- "$target")"
    ln -sfn -- "$relative" "$link"
done < <(find "$sysroot/lib" "$sysroot/usr/lib" -type l -lname '/*' -print0)

target_lib="$sysroot/usr/lib/arm-linux-gnueabihf"
ln -s libstdc++.so.6 "$target_lib/libstdc++.so"
ln -s ../../../lib/arm-linux-gnueabihf/libgcc_s.so.1 "$target_lib/libgcc_s.so"

env PKG_CONFIG_PATH= PKG_CONFIG_SYSROOT_DIR="$sysroot" \
    PKG_CONFIG_LIBDIR="$target_lib/pkgconfig:$sysroot/usr/share/pkgconfig" \
    pkg-config --modversion alsa sndfile
```

Task 1 returned ALSA 1.2.8 and libsndfile 1.2.0 and linked a temporary C++17 shared object against both sysroot libraries. Refresh or recheck the sysroot after target OS or runtime changes.

## Build and inspect an ARM artifact

From the repository root, use the existing M0 executable as the cross-build smoke target:

```bash
bash tools/scripts/build-armhf.sh --sysroot "$sysroot" --target ai_robot
file build-armhf/ai_robot
arm-linux-gnueabihf-readelf -h -A -l build-armhf/ai_robot
arm-linux-gnueabihf-readelf -d build-armhf/ai_robot
```

The [build script](../../tools/scripts/build-armhf.sh) builds all configured targets if `--target` is omitted. The default WSL Makefile build uses Release in `build-armhf/` with target tests disabled. Use `--build-dir` for another build tree and pass CMake configure arguments after `--`. Use a fresh build directory when changing sysroots. The [toolchain file](../../tools/cmake/arm-linux-gnueabihf.cmake) selects target startup files and libraries and limits CMake and pkg-config library discovery to the sysroot. Host tools such as CMake and pkg-config run in WSL. [CMake toolchain documentation](https://cmake.org/cmake/help/v3.16/manual/cmake-toolchains.7.html#cross-compiling-for-linux)

Task 2 built `ai_robot` successfully. `readelf` showed ELF32, little-endian ARMv7, EABI5 hard-float, VFP register arguments, and interpreter `/lib/ld-linux-armhf.so.3`, matching the Pi's userspace ABI. Native M0 CTest also passed. See the target smoke test below for the on-Pi result.

## Preview a deployment

Supply any local artifact, an SSH host alias or `user@host`, and an absolute remote file path. Dry run is the default and performs local input validation before printing the planned command.

```bash
bash tools/scripts/deploy.sh --dry-run build-armhf/ai_robot pi.example /home/pi/ai-robot/ai_robot
```

When a transfer is authorized, replace `--dry-run` with `--execute`. The [deploy script](../../tools/scripts/deploy.sh) uses OpenSSH configuration and authentication, requires a trusted host key and a writable existing destination directory, and preserves file mode. The remote path uses letters, digits, dots, underscores, dashes, and slashes. Dry run does not verify remote access or write permissions.

## Target smoke test

For an authorized deployment, create a dedicated directory under the target user's home. Preview the transfer, then explicitly execute it. Set the SSH destination to an alias or user and hostname configured for your target.

```bash
ssh_target=pi.example
remote_dir=$(ssh -o StrictHostKeyChecking=yes "$ssh_target" \
    'mktemp -d "$HOME/ai-robot-smoke.XXXXXXXX"')
bash tools/scripts/deploy.sh --dry-run build-armhf/ai_robot "$ssh_target" "$remote_dir/ai_robot"
bash tools/scripts/deploy.sh --execute build-armhf/ai_robot "$ssh_target" "$remote_dir/ai_robot"
sha256sum build-armhf/ai_robot
ssh -o StrictHostKeyChecking=yes "$ssh_target" "sha256sum -- '$remote_dir/ai_robot'"
ssh -o StrictHostKeyChecking=yes "$ssh_target" "'$remote_dir/ai_robot'"
```

Compare the checksums before running the last command. SSH returns the remote application's exit code when the command completes normally.

Task 3 passed on the first attempt on the Raspberry Pi 3 Model B running Bookworm armhf. The transferred file matched the WSL SHA-256 and had mode `755`. The executable printed `AI Robot: M0 foundation ready.`, returned `0`, and produced no stderr. This evidence covers the M0 ARM artifact and the transfer and execution path.
