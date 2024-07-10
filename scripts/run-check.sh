#!/bin/sh
#
# run-check.sh - Discover and run the project's tests, aggregate results.
#
# Usage: run-check.sh <objdir> <srcdir> [mode]
#
#   mode = unit  (default)  run unit tests + bats integration suites only,
#                           minus the bats cases tagged `slow`
#   mode = all              additionally run the `slow` bats cases and the
#                           performance benchmarks
#
# `make test` / `make kselftest` use the default (unit); `make check` passes
# "all". The perf benchmarks also require CONFIG_BENCHMARK=y to have been
# built (vendor-prepare); "all" mode only controls whether they are run.
#
# A bats file or case that takes minutes — a traffic matrix, a sweep over a
# whole capture — declares itself with bats' own tag syntax,
#
#   # bats file_tags=slow      (whole file)
#   # bats test_tags=slow      (the case that follows)
#
# which keeps `make test` a fast edit-compile-test loop and leaves the long
# runs to `make check`.
#
# Test layout (kernel-style, per package and per submodule):
#   <src>/tools/testing/selftests/units/   cmocka / unit test sources -> binaries
#   <src>/tools/testing/selftests/perf/    benchmarks (run only in "all" mode)
#   <src>/tools/testing/bats/*.bats        integration tests (bats)
#
# Unit binaries are built by kbuild into the object tree, mirroring the source
# path (e.g. obj/vendor/crypto/tools/testing/selftests/units/test_digest). This
# script enumerates them directly across the package and, when
# CONFIG_CHECK_SUBMODULES=y, its submodules under vendor/*, and exports each as
# <NAME>_BIN; the binaries themselves are run by each package's bats suite,
# which carries a dedicated case per unit (e.g. hpc's test_units.bats), so all
# results land in one TAP stream. Enumerating directly (rather than invoking
# each package's own chaining run-tests.sh) avoids double-running shared
# submodules.
#
# Exit codes: 0 = all passed, 1 = one or more failed.

OBJDIR="${1:-.}"
SRCDIR="${2:-.}"
MODE="${3:-unit}"	# "unit" (default): units + bats; "all": also perf benchmarks

pass=0; fail=0; skip=0; total=0

RED='\033[0;31m'; GRN='\033[0;32m'; YEL='\033[0;33m'; BLU='\033[0;34m'; RST='\033[0m'

# Label a path like .../[vendor/<name>/]tools/testing/... as "<name>" or
# "self". Submodule selftests may also be hosted at a relocated srctree path
# (e.g. crypto/tools/testing/... via a symlink); label those by the path
# component preceding tools/testing/, and the package's own root as "self".
label_of() {
	case "$1" in
		*/vendor/*/tools/testing/*)
			echo "$1" | sed 's|.*/vendor/\([^/]*\)/tools/testing/.*|\1|' ;;
		*/tools/testing/*)
			p=${1%/tools/testing/*}
			if [ "$p" = "$OBJDIR" ] || [ "$p" = "$SRCDIR" ]; then
				echo "self"
			else
				echo "${p##*/}"
			fi ;;
		*) echo "self" ;;
	esac
}

if [ -f "$OBJDIR/include/config/auto.conf" ]; then
	. "$OBJDIR/include/config/auto.conf" 2>/dev/null || true
fi

# Discover unit-test executables (test_*) under any selftests/units dir and
# export each as <NAME>_BIN. They are not run from here: each package's bats
# suite carries a dedicated case per unit, which is where they are run,
# reported and counted.
units=$(find "$OBJDIR" -type f -path '*/tools/testing/selftests/units/test_*' \
	! -name '*.o' ! -name '*.d' ! -name '*.cmd' 2>/dev/null | sort)
for exe in $units; do
	name=$(basename "$exe")
	abs=$(cd "$(dirname "$exe")" && pwd)/"$name"
	# Shell identifiers allow only [A-Za-z0-9_], so fold everything else
	# (e.g. the dash in `ub-patch`) to an underscore: UB_PATCH_BIN.
	varname=$(printf '%s' "$name" | tr '[:lower:]' '[:upper:]' | tr -c 'A-Z0-9_' '_')_BIN
	export "$varname=$abs"
done

# Discover per-primitive tool binaries (kbuild `testprogs-y`: digest, hkdf, ...)
# that live directly under a package's tools/ dir (not tools/testing/*). The
# crypto bats suites wrap these and locate them via <NAME>_BIN; export those
# vars here from the real object tree. Without this the suites fall back to a
# guess relative to their own srcdir, which misses the integrated layout (obj
# at the superproject root, e.g. obj/crypto/tools/hkdf) and skips every case.
tools=$(find "$OBJDIR" -type f -path '*/tools/*' ! -path '*/tools/testing/*' \
	-perm -u+x ! -name '*.o' ! -name '*.d' ! -name '*.cmd' ! -name '*.order' \
	2>/dev/null | sort)
for exe in $tools; do
	name=$(basename "$exe")
	abs=$(cd "$(dirname "$exe")" && pwd)/"$name"
	# Shell identifiers allow only [A-Za-z0-9_], so fold everything else
	# (e.g. the dash in `ub-patch`) to an underscore: UB_PATCH_BIN.
	varname=$(printf '%s' "$name" | tr '[:lower:]' '[:upper:]' | tr -c 'A-Z0-9_' '_')_BIN
	export "$varname=$abs"
done

# Run bats integration suites for the package and its submodules.
#
# Put the pinned, vendored bats-core on PATH so the suites run without a
# system-wide bats install. In the integrated un tree, vendor/bats-core is a
# symlink into the kbuild submodule (un depends on kbuild for build/test infra);
# kbuild, hpc and crypto each also carry their own copy for standalone runs.
# First match wins; a system bats still works if none are checked out. Purely
# additive — we prepend, never replace.
for _bc in \
	"$SRCDIR/vendor/bats-core" \
	"$SRCDIR/vendor/kbuild/vendor/bats-core" \
	"$SRCDIR/vendor/crypto/vendor/bats-core" \
	"$SRCDIR/vendor/hpc/vendor/bats-core"; do
	if [ -x "$_bc/bin/bats" ]; then
		PATH="$_bc/bin:$_bc/libexec:$PATH"; export PATH
		break
	fi
done

have_bats=1
command -v bats >/dev/null 2>&1 || have_bats=0

# run_bats_dir <bats-dir> <label> <package-root>
# Runs the suite from the owning package root (so its relative `source`/paths
# resolve) with absolute .bats paths.
run_bats_dir() {
	dir="$1"; lbl="$2"; root="$3"
	[ -d "$dir" ] || return
	adir=$(cd "$dir" && pwd)
	set -- "$adir"/*.bats
	[ -f "$1" ] || return
	if [ "$have_bats" -eq 0 ]; then
		printf "${YEL}  SKIP${RST}    [%s] bats suite (bats not installed)\n" "$lbl"
		total=$((total + 1)); skip=$((skip + 1)); return
	fi
	aroot=$(cd "$root" 2>/dev/null && pwd) || aroot="$adir"
	# Probe loadability: a suite that can't even be enumerated (e.g. missing
	# bats helper submodules, or a tool it sources) is gated, not a failure.
	if ! all=$(cd "$aroot" && bats --count "$@" 2>/dev/null); then
		printf "${YEL}  SKIP${RST}    [%s] bats suite (unmet prerequisites)\n" "$lbl"
		total=$((total + 1)); skip=$((skip + 1)); return
	fi
	# Long-running suites carry bats' own file/test tag `slow`:
	#
	#     # bats file_tags=slow
	#
	# and are deferred out of "unit" mode, so `make test` stays the fast
	# loop and `make check` is the one that runs everything. Count what
	# that leaves, both to say how many cases were held back and to keep
	# quiet about a directory it empties.
	deferred=0; tagf=
	if [ "$MODE" != "all" ]; then
		tagf="!slow"
		want=$(cd "$aroot" && bats --count --filter-tags "$tagf" "$@" 2>/dev/null) || want=$all
		deferred=$((all - want))
		if [ "$want" -eq 0 ]; then
			[ "$deferred" -gt 0 ] && printf \
			    "${YEL}  DEFER${RST}   [%s] bats suite, %d slow case(s) (run \`make check\`)\n" \
			    "$lbl" "$deferred"
			return
		fi
	fi
	printf "\n${BLU}=== [%s] bats ===${RST}\n" "$lbl"
	[ "$deferred" -gt 0 ] && printf \
	    "${YEL}  DEFER${RST}   %d slow case(s) held for \`make check\`\n" "$deferred"
	# The suite's output goes straight to the terminal, unbuffered and
	# unfiltered, so a long case shows progress while it runs rather than
	# arriving in one block when the file is over. That rules out parsing
	# the on-screen summary for counts — cursor control and all — so the
	# counts come from a TAP report written alongside it instead.
	#
	# Force --pretty on a terminal: with a report formatter attached bats
	# would otherwise fall back to TAP; when redirected we leave TAP, which
	# streams line by line anyway.
	fmt=; [ -t 1 ] && fmt=--pretty
	rep=$(mktemp -d "${TMPDIR:-/tmp}/run-check-bats.XXXXXX")
	(cd "$aroot" && bats $fmt ${tagf:+--filter-tags "$tagf"} \
		--report-formatter tap -o "$rep" "$@")
	rc=$?
	# Aggregate per-case counts from the TAP report.
	t=0; f=0; s=0
	if [ -f "$rep/report.tap" ]; then
		t=$(grep -c '^\(not \)\{0,1\}ok [0-9]' "$rep/report.tap")
		f=$(grep -c '^not ok [0-9]' "$rep/report.tap")
		s=$(grep -c '^ok [0-9].*# [Ss][Kk][Ii][Pp]' "$rep/report.tap")
	fi
	rm -rf "$rep"
	if [ "$rc" -ne 0 ] && [ "$f" -eq 0 ]; then
		# Suite failed without a parsable failing case (e.g. bats itself
		# died); count one failure so the report can't show all-green.
		t=$((t + 1)); f=$((f + 1))
	fi
	total=$((total + t)); fail=$((fail + f)); skip=$((skip + s))
	pass=$((pass + t - f - s))
	if [ "$rc" -ne 0 ]; then
		printf "${RED}  FAIL${RST}    [%s] bats suite\n" "$lbl"
	fi
}

# The package's own suite: label it the way label_of labels a package root
# ("self"), so unit and bats lines in one run agree. kbuild is shared infra —
# it must not hardcode any particular consumer's name here.
run_bats_dir "$SRCDIR/tools/testing/bats" "$(label_of "$SRCDIR/tools/testing/bats")" "$SRCDIR"
if [ "${CONFIG_CHECK_SUBMODULES}" = "y" ]; then
	for d in "$SRCDIR"/vendor/*/tools/testing/bats; do
		[ -d "$d" ] || continue
		sub=$(label_of "$d/x")
		run_bats_dir "$d" "$sub" "$SRCDIR/vendor/$sub"
	done
fi

# Performance benchmarks: run the TLS handshake benchmark (built by the
# vendor-prepare step) when enabled. It prints its own metrics table.
# Only in "all" mode (`make check`); `make test`/`make kselftest` skip these.
if [ "$MODE" = "all" ] && [ "${CONFIG_BENCHMARK}" = "y" ]; then
	# rustls-vs-OpenSSL comparison (Rust harness).
	bench="$OBJDIR/tools/testing/selftests/perf/tls-bench/release/tls-bench"
	total=$((total + 1))
	if [ -x "$bench" ]; then
		printf "\n${BLU}=== TLS handshake benchmark (rustls-ossl vs rustls-aws-lc) ===${RST}\n\n"
		if "$bench"; then pass=$((pass + 1)); else
			printf "${RED}  FAIL${RST}    tls-bench (exit $?)\n"; fail=$((fail + 1)); fi
	else
		printf "${YEL}  SKIP${RST}    tls-bench (not built)\n"; skip=$((skip + 1))
	fi

	# OpenSSL C-API handshake benchmark (static binary).
	ossl="$OBJDIR/tools/testing/selftests/perf/ossl-handshake/ossl-handshake"
	total=$((total + 1))
	if [ -x "$ossl" ]; then
		printf "\n${BLU}=== TLS handshake benchmark (OpenSSL C API, static) ===${RST}\n\n"
		if "$ossl" "$SRCDIR/vendor/crypto/vendor/rustls/test-ca"; then pass=$((pass + 1)); else
			printf "${RED}  FAIL${RST}    ossl-handshake (exit $?)\n"; fail=$((fail + 1)); fi
	else
		printf "${YEL}  SKIP${RST}    ossl-handshake (not built)\n"; skip=$((skip + 1))
	fi

	# Passive TLS decode probe (OpenSSL C API, static) from pcap+keylog.
	pd="$OBJDIR/tools/testing/selftests/perf/probe/ossl/ossl-probe"
	total=$((total + 1))
	if [ -x "$pd" ]; then
		printf "\n${BLU}=== Passive TLS decode probe (OpenSSL C API, from pcap+keylog) ===${RST}\n\n"
		if "$pd" bench "$SRCDIR/tools/testing/tls/fixture/pcap"; then
			pass=$((pass + 1)); else
			printf "${RED}  FAIL${RST}    ossl-probe (exit $?)\n"; fail=$((fail + 1)); fi
	else
		printf "${YEL}  SKIP${RST}    ossl-probe (not built)\n"; skip=$((skip + 1))
	fi

	# Passive TLS decode probe (rustls/aws-lc-rs) from the same fixtures.
	rp="$OBJDIR/tools/testing/selftests/perf/probe/rustls/release/rustls-probe"
	total=$((total + 1))
	if [ -x "$rp" ]; then
		printf "\n${BLU}=== Passive TLS decode probe (rustls/aws-lc-rs, from pcap+keylog) ===${RST}\n\n"
		if "$rp" bench "$SRCDIR/tools/testing/tls/fixture/pcap"; then
			pass=$((pass + 1)); else
			printf "${RED}  FAIL${RST}    rustls-probe (exit $?)\n"; fail=$((fail + 1)); fi
	else
		printf "${YEL}  SKIP${RST}    rustls-probe (not built)\n"; skip=$((skip + 1))
	fi

	# un-native feed template (no decode; proves the traffic-feed path works).
	up="$OBJDIR/tools/testing/selftests/perf/probe/un/un-probe"
	total=$((total + 1))
	if [ -x "$up" ]; then
		printf "\n${BLU}=== Passive TLS feed template (un-native, no decode) ===${RST}\n\n"
		if "$up" bench "$SRCDIR/tools/testing/tls/fixture/pcap"; then
			pass=$((pass + 1)); else
			printf "${RED}  FAIL${RST}    un-probe (exit $?)\n"; fail=$((fail + 1)); fi
	else
		printf "${YEL}  SKIP${RST}    un-probe (not built)\n"; skip=$((skip + 1))
	fi
fi

echo ""
printf "Total: %d  Passed: ${GRN}%d${RST}  Failed: ${RED}%d${RST}  Skipped: ${YEL}%d${RST}\n" \
	"$total" "$pass" "$fail" "$skip"
echo ""
[ $fail -gt 0 ] && exit 1
exit 0
