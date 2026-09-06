#!/bin/sh
# Check ncurses compatibility

# What library to link
ldflags()
{
	pkg-config --libs ncursesw 2>/dev/null && exit
	pkg-config --libs ncurses 2>/dev/null && exit
	# No pkg-config: ask the compiler by trying an actual link.
	#
	# This used to probe with -print-file-name, which only ever names a
	# library that sits in the compiler's own search path as a real file.
	# On macOS the system ncurses is the SDK stub plus the dyld shared
	# cache, so no candidate ever matched, we printed nothing, and mconf
	# was linked with no -l at all -- the whole of curses came back
	# undefined at link time. A link probe answers the question we
	# actually have, and works the same way everywhere.
	#
	# eval, because ccflags quotes the header for the make command line
	# it is normally pasted into (-DCURSES_LOC="<curses.h>"); expanding
	# it here without a round of shell parsing would hand the compiler
	# the quote characters themselves.
	cflags=$(ccflags | tr '\n' ' ')
	for lib in ncursesw ncurses curses ; do
		if eval "$cc $cflags -x c - -l${lib} -o $tmp" 2>/dev/null <<-EOF
			#include CURSES_LOC
			int main(void) { initscr(); return 0; }
		EOF
		then
			echo "-l${lib}"
			exit
		fi
	done
	exit 1
}

# Where is ncurses.h?
ccflags()
{
	if pkg-config --cflags ncursesw 2>/dev/null; then
		echo '-DCURSES_LOC="<ncurses.h>" -DNCURSES_WIDECHAR=1'
	elif pkg-config --cflags ncurses 2>/dev/null; then
		echo '-DCURSES_LOC="<ncurses.h>"'
	elif [ -f /usr/include/ncursesw/curses.h ]; then
		echo '-I/usr/include/ncursesw -DCURSES_LOC="<curses.h>"'
		echo ' -DNCURSES_WIDECHAR=1'
	elif [ -f /usr/include/ncurses/ncurses.h ]; then
		echo '-I/usr/include/ncurses -DCURSES_LOC="<ncurses.h>"'
	elif [ -f /usr/include/ncurses/curses.h ]; then
		echo '-I/usr/include/ncurses -DCURSES_LOC="<curses.h>"'
	elif [ -f /usr/include/ncurses.h ]; then
		echo '-DCURSES_LOC="<ncurses.h>"'
	else
		echo '-DCURSES_LOC="<curses.h>"'
	fi
}

# Temp file, try to clean up after us
tmp=.lxdialog.tmp
trap "rm -f $tmp" 0 1 2 3 15

# Check if we can link to ncurses
check() {
        $cc -x c - -o $tmp 2>/dev/null <<'EOF'
#include CURSES_LOC
main() {}
EOF
	if [ $? != 0 ]; then
	    exit 0
	    echo " *** Unable to find the ncurses libraries or the"       1>&2
	    echo " *** required header files."                            1>&2
	    echo " *** 'make menuconfig' requires the ncurses libraries." 1>&2
	    echo " *** "                                                  1>&2
	    echo " *** Install ncurses (ncurses-devel) and try again."    1>&2
	    echo " *** "                                                  1>&2
	    exit 1
	fi
}

usage() {
	printf "Usage: $0 [-check compiler options|-ccflags|-ldflags compiler options]\n"
}

if [ $# -eq 0 ]; then
	usage
	exit 1
fi

cc=""
case "$1" in
	"-check")
		shift
		cc="$@"
		check
		;;
	"-ccflags")
		ccflags
		;;
	"-ldflags")
		shift
		cc="$@"
		ldflags
		;;
	"*")
		usage
		exit 1
		;;
esac
