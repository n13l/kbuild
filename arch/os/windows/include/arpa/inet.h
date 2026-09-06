#ifndef OS_WINDOWS_COMPAT_ARPA_INET_H
#define OS_WINDOWS_COMPAT_ARPA_INET_H

/*
 * <arpa/inet.h> on Windows: inet_ntop() and inet_pton() are ws2tcpip's (Vista
 * and later, with the POSIX signatures), and the byte-order calls come with
 * <netinet/in.h>.
 */
#include <netinet/in.h>

#endif
