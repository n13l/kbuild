#!/bin/sh
# Configure OpenSSL vendor library based on Kbuild CONFIG_ variables.
#
# Usage: configure-openssl.sh <srctree> <objtree> <auto.conf>
#
# This script reads CONFIG_ variables from the Kbuild auto.conf file
# and translates them into OpenSSL ./Configure arguments, then runs
# OpenSSL's Configure in the output directory.

set -e

srctree="$1"
objtree="$2"
autoconf="$3"

if [ -z "$srctree" ] || [ -z "$objtree" ] || [ -z "$autoconf" ]; then
	echo "Usage: $0 <srctree> <objtree> <auto.conf>" >&2
	exit 1
fi

srctree=$(cd "$srctree" && pwd)
objtree=$(cd "$objtree" && pwd)

OPENSSL_SRC="${srctree}/vendor/openssl"
OPENSSL_OUT="${objtree}/vendor/openssl"

if [ ! -f "${OPENSSL_SRC}/Configure" ]; then
	echo "  ERROR: OpenSSL source not found at ${OPENSSL_SRC}" >&2
	exit 1
fi

. "$autoconf" 2>/dev/null || true

ossl_target=""
case "${ARCH}" in
	x86_64|x86)
		if [ "${CONFIG_X86_64}" = "y" ] || [ "${ARCH}" = "x86_64" ]; then
			ossl_target="linux-x86_64"
		else
			ossl_target="linux-elf"
		fi
		;;
	arm64)
		ossl_target="linux-aarch64"
		;;
	arm)
		ossl_target="linux-armv4"
		;;
	s390)
		ossl_target="linux64-s390x"
		;;
	powerpc)
		ossl_target="linux-ppc64le"
		;;
	*)
		ossl_target="linux-generic64"
		;;
esac

if [ "${PLATFORM}" = "darwin" ]; then
	case "${ARCH}" in
		arm64) ossl_target="darwin64-arm64-cc" ;;
		x86_64|x86) ossl_target="darwin64-x86_64-cc" ;;
		*) ossl_target="darwin64-arm64-cc" ;;
	esac
fi

OSSL_ARGS=""

if [ "${CONFIG_DEBUG_INFO}" = "y" ]; then
	OSSL_ARGS="${OSSL_ARGS} --debug"
fi

if [ "${CONFIG_CC_OPTIMIZE_FOR_SIZE}" = "y" ]; then
	OSSL_ARGS="${OSSL_ARGS} -Os"
fi

if [ "${CONFIG_OPENSSL_NO_ASM}" = "y" ]; then
	OSSL_ARGS="${OSSL_ARGS} no-asm"
fi

if [ "${CONFIG_OPENSSL_NO_SHARED}" != "y" ]; then
	OSSL_ARGS="${OSSL_ARGS} no-shared"
fi

if [ "${CONFIG_OPENSSL_NO_TESTS}" != "n" ]; then
	OSSL_ARGS="${OSSL_ARGS} no-tests"
fi

if [ "${CONFIG_OPENSSL_FIPS}" = "y" ]; then
	OSSL_ARGS="${OSSL_ARGS} enable-fips"
fi

if [ -n "${CROSS_COMPILE}" ]; then
	OSSL_ARGS="${OSSL_ARGS} --cross-compile-prefix=${CROSS_COMPILE}"
fi

if [ -n "${CONFIG_OPENSSL_EXTRA_ARGS}" ]; then
	extra=$(echo "${CONFIG_OPENSSL_EXTRA_ARGS}" | sed 's/^"//;s/"$//')
	OSSL_ARGS="${OSSL_ARGS} ${extra}"
fi

OSSL_ARGS="${OSSL_ARGS} --prefix=${OPENSSL_OUT}/install"
OSSL_ARGS="${OSSL_ARGS} --openssldir=${OPENSSL_OUT}/ssl"

mkdir -p "${OPENSSL_OUT}"

stamp="${OPENSSL_OUT}/.configured"
args_hash=$(echo "${ossl_target} ${OSSL_ARGS}" | sha1sum | cut -d' ' -f1)

if [ -f "${stamp}" ] && [ -f "${OPENSSL_OUT}/Makefile" ] && \
   [ "$(cat "${stamp}")" = "${args_hash}" ]; then
	exit 0
fi

echo "  CONFIG  vendor/openssl (${ossl_target})"

cd "${OPENSSL_OUT}"
if [ "${KBUILD_VERBOSE}" = "1" ]; then
	"${OPENSSL_SRC}/Configure" ${ossl_target} ${OSSL_ARGS}
else
	"${OPENSSL_SRC}/Configure" ${ossl_target} ${OSSL_ARGS} > /dev/null 2>&1
fi

echo "${args_hash}" > "${stamp}"

# Generate SHA assembly files for the target architecture
asm_stamp="${OPENSSL_OUT}/.asm_generated"
if [ ! -f "${asm_stamp}" ] || [ "${stamp}" -nt "${asm_stamp}" ]; then
	echo "  GEN     vendor/openssl (asm-${ARCH})"
	cd "${OPENSSL_OUT}"
	case "${ARCH}" in
		x86_64|x86)
			make -f Makefile crypto/sha/keccak1600-x86_64.s \
				crypto/sha/sha1-x86_64.s \
				crypto/sha/sha256-x86_64.s \
				crypto/sha/sha512-x86_64.s \
				2>&1 | if [ "${KBUILD_VERBOSE}" != "1" ]; then cat > /dev/null; else cat; fi
			;;
		arm64)
			make -f Makefile crypto/sha/keccak1600-armv8.S \
				crypto/sha/sha1-armv8.S \
				crypto/sha/sha256-armv8.S \
				crypto/sha/sha512-armv8.S \
				2>&1 | if [ "${KBUILD_VERBOSE}" != "1" ]; then cat > /dev/null; else cat; fi
			;;
		arm)
			make -f Makefile crypto/sha/sha1-armv4-large.S \
				crypto/sha/sha256-armv4.S \
				crypto/sha/sha512-armv4.S \
				crypto/sha/keccak1600-armv4.S \
				2>&1 | if [ "${KBUILD_VERBOSE}" != "1" ]; then cat > /dev/null; else cat; fi
			;;
		s390)
			make -f Makefile crypto/sha/sha1-s390x.S \
				crypto/sha/sha256-s390x.S \
				crypto/sha/sha512-s390x.S \
				crypto/sha/keccak1600-s390x.S \
				2>&1 | if [ "${KBUILD_VERBOSE}" != "1" ]; then cat > /dev/null; else cat; fi
			;;
		powerpc)
			make -f Makefile crypto/sha/sha1-ppc.s \
				crypto/sha/sha256-ppc.s \
				crypto/sha/sha512-ppc.s \
				crypto/sha/keccak1600-ppc64.s \
				2>&1 | if [ "${KBUILD_VERBOSE}" != "1" ]; then cat > /dev/null; else cat; fi
			;;
	esac
	touch "${asm_stamp}"
fi
