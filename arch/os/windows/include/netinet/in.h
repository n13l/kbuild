#ifndef OS_WINDOWS_COMPAT_NETINET_IN_H
#define OS_WINDOWS_COMPAT_NETINET_IN_H

/*
 * <netinet/in.h> on Windows: Winsock has everything in it — the address
 * structures, the IPPROTO_ numbers, htons() and the rest — under its own header
 * names, and none of the two typedefs below, which POSIX puts here and Winsock
 * never adopted. The byte-order calls are functions in ws2_32.dll, not macros,
 * which kbuild links on Windows (scripts/Makefile.shared).
 */
#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>

typedef uint32_t	in_addr_t;
typedef uint16_t	in_port_t;

#endif
