#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <mem/map.h>
#include <mem/stack.h>
#include <mem/alloca.h>

size_t
strval_units(const char *s)
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
