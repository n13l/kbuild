#include <string.h>

char *
__compat_strchrnul(const char *s, int in)
{
	char c = in;
	while (*s && (*s != c))
		s++;
	return (char *) s;
}
