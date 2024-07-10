
#include <arch/os/linux/io/io.h>

char		*optarg;
int		optind = 1;
int		opterr = 1;
int		optopt = '?';

static char	io_stdout_stream;
static char	io_stderr_stream;

void		*stdout = &io_stdout_stream;
void		*stderr = &io_stderr_stream;

int
fileno(void *stream)
{
	if (stream == stdout)
		return 1;
	if (stream == stderr)
		return 2;
	return -1;
}
