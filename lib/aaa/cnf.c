#include <sys/compiler.h>
#include <sys/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <mem/list.h>
#include <generic/opt/opt.h>
#include <generic/opt/conf.h>

#include <net/tls/conf.h>

#include <aaa/lib.h>
#include <aaa/prv.h>

struct cf_aaa {
	unsigned int length;
} cf_aaa;

static struct cf_section _unused cf_section_aaa = {
	CF_ITEMS {
		CF_UINT  ("Length",  &cf_aaa.length),
	CF_END
	}
};

static struct cf_section aaa_section = {                                        
	CF_ITEMS {
		CF_SECTION("TLS", NULL, &cf_tls_section),
	CF_END
	}
};

static void _constructor
__init_aaa_section(void)
{
	cf_declare_section("AAA", &aaa_section, 0);
}

void
aaa_config_load(struct aaa *c)
{
	const char *file = getenv("OPENAAA_CONF");
	int rv;
	rv = cf_load(file);
	sys_dbg("config file load(%s):%d", file, rv);

	sys_dbg("tls.rfc5705.context=%s", cf_tls_rfc5705.context);
	sys_dbg("tls.rfc5705.label=%s", cf_tls_rfc5705.label);
	sys_dbg("tls.rfc5705.length=%d", cf_tls_rfc5705.length);

}
