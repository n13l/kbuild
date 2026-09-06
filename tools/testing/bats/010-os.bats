#!/usr/bin/env bats
#
# arch/os/<platform> — the platform layer kbuild carries for every tree that
# vendors it, and here tested where it lives.
#
# The runner (scripts/run-check.sh) builds the unit binaries into the object
# tree and exports each as <NAME>_BIN; this suite is where they are run,
# reported and counted, one case each. A unit that was not built — because its
# platform symbol is off, or (for the cmocka ones) because the toolchain has no
# libcmocka — skips with the reason, which is a configuration and not a failure.
#
# os_str and os_scan are pure and run on any build. os_io and os_entropy make
# the real calls, so they run where the build runs. test_os_sc is the in-process
# half (Linux, CONFIG_OS_SC): it arms the process and traps a call, and it is a
# standalone program that prints TAP, so this case asserts on its output rather
# than only its exit.

_OS_BATS_DIR="${BATS_TEST_DIRNAME}"
_OS_ROOT="$(cd "${_OS_BATS_DIR}/../../.." && pwd)"

# unit_bin <name> — the built binary, from what run-check.sh exported or the
# standalone object tree, or nothing if it was not built.
unit_bin() {
	local name="$1"
	local var
	# printf, not echo: run-check.sh folds the name with printf (no newline),
	# and tr -c here would turn echo's trailing newline into an underscore,
	# looking up TEST_OS_SCAN__BIN for the TEST_OS_SCAN_BIN it exported.
	var="$(printf '%s' "${name}" | tr '[:lower:]' '[:upper:]' | tr -c 'A-Z0-9_' '_')_BIN"
	local c
	for c in "${!var:-}" "${_OS_ROOT}/obj/tools/testing/selftests/units/${name}"; do
		[ -n "${c}" ] || continue
		[ -x "${c}" ] && { echo "${c}"; return 0; }
	done
	return 0
}

# A cmocka unit: run it, forward its output to the log on failure, and fail the
# case if any group failed (cmocka says "FAILED" and exits non-zero).
run_cmocka() {
	local name="$1" why="${2:-not built (needs CONFIG_CMOCKA and the platform symbol)}"
	local bin
	bin="$(unit_bin "${name}")"
	[ -n "${bin}" ] || skip "${why}"

	run "${bin}"
	[ "${status}" -ne 0 ] && echo "${output}" >&3
	[ "${status}" -eq 0 ]
	[[ "${output}" != *"FAILED"* ]]
}

@test "os: str helpers (arch/os/str.h)" {
	run_cmocka test_os_str
}

@test "os: scan — syscall sites in a run of code" {
	run_cmocka test_os_scan "not built (needs CONFIG_OS_SCAN + cmocka)"
}

@test "os: io — the system calls, round-tripped" {
	run_cmocka test_os_io "not built (needs CONFIG_OS_IO + cmocka)"
}

@test "os: entropy — getrandom's shape over the platform's call" {
	run_cmocka test_os_entropy "not built (needs CONFIG_OS_ENTROPY + cmocka)"
}

@test "os: sc — arm this process and dispatch the trap" {
	local bin
	bin="$(unit_bin test_os_sc)"
	[ -n "${bin}" ] || skip "not built (needs CONFIG_OS_SC; Linux only)"

	run "${bin}"
	[ "${status}" -ne 0 ] && echo "${output}" >&3
	[ "${status}" -eq 0 ]
	# Its own TAP: a plan line and no "not ok".
	[[ "${output}" == *"1..6"* ]]
	[[ "${output}" != *"not ok"* ]]
}
