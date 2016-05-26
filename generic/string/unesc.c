
#include <sys/compiler.h>
#include <sys/log.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <mem/map.h>
#include <mem/stack.h>
#include <mem/alloc.h>

#include <generic/strings.h>

char *
str_unesc(char *d, const char *s)
{
	while (*s) {
		if (*s == '\\')
			switch (s[1]) {
			case 'a': *d++ = '\a'; s += 2; break;
			case 'b': *d++ = '\b'; s += 2; break;
			case 'f': *d++ = '\f'; s += 2; break;
			case 'n': *d++ = '\n'; s += 2; break;
			case 'r': *d++ = '\r'; s += 2; break;
			case 't': *d++ = '\t'; s += 2; break;
			case 'v': *d++ = '\v'; s += 2; break;
			case '\?': *d++ = '\?'; s += 2; break;
			case '\'': *d++ = '\''; s += 2; break;
			case '\"': *d++ = '\"'; s += 2; break;
			case '\\': *d++ = '\\'; s += 2; break;
			case 'x':
				if (!isdigit((int)s[2])) {
					s++;
				} else {
					char *p;
					uint v = strtoul(s + 2, &p, 16);
					if (v <= 255)
						*d++ = v;
					else
						sys_dbg("hex escape sequence out of range");
					s = (char *)p;
				}
			break;
			default:
				if (s[1] >= '0' && s[1] <= '7') {
					uint v = s[1] - '0';
					s += 2;
					for (uint i = 0; i < 2 && *s >= '0' && *s <= '7'; s++, i++)
						v = (v << 3) + *s - '0';
					if (v <= 255)
						*d++ = v;
					else
						sys_dbg("octal escape sequence out of range");
				} else
					*d++ = *s++;
			break;
			} 
		else
			*d++ = *s++;
	}

	*d = 0;
	return d;
}

size_t
value_units(const char *s)
{
	char m;
	double v = atof(s);
	m = s[strlen(s) - 1];
	switch (m) {
	case 'G':
	case 'g':
	default:
		v *= 1024*1024;
		break;
	case 'M':
	case 'm':
		v *= 1024;
		break;
	case 'K':
	case 'k':
		v *= 1;
		break;
	}
	return (size_t)v;
}
