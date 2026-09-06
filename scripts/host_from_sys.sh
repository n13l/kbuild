#!/bin/sh
#
# The platform a uname -s or a target triple names, in the one spelling the
# rest of kbuild uses: it is PLATFORM, the directory under arch/os/, and the
# value Kconfig compares against.
#
#   x86_64-w64-mingw32       gcc's mingw-w64 triple          windows
#   x86_64-w64-windows-gnu   clang's spelling of the same    windows
#   aarch64-pc-windows-msvc  clang-cl / MSVC ABI             windows
#   MINGW64_NT-10.0, MSYS_NT-10.0, CYGWIN_NT-10.0            windows
#
# Cygwin is a POSIX layer over Windows and a program built for it links
# cygwin1.dll rather than the Windows API; it is listed so that a build on one
# is told it is on Windows, which is the platform layer it gets.
set -e
opt=$(echo "$*" | tr '[:upper:]' '[:lower:]')
for arg in $opt ; do
	case $arg in
	*mingw*)   printf "windows\n"; ;;
	*windows*) printf "windows\n"; ;;
	*msys*)    printf "windows\n"; ;;
	*cygwin*)  printf "windows\n"; ;;
	*cygnus*)  printf "windows\n"; ;;
	*linux*)   printf "linux\n"; ;;
	*darwin*)  printf "macos\n"; ;;
	*macos*)   printf "macos\n"; ;;
	*390)      printf "os390\n"; ;;
	*)
	esac
done
