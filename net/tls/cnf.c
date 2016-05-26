
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <mem/list.h>
#include <generic/opt/opt.h>
#include <generic/opt/conf.h>

#include <net/tls/proto.h>
#include <net/tls/conf.h>

struct cf_tls_rfc5705 cf_tls_rfc5705 = {
	.context = "OpenAAA",
	.label   = "EXPORTER_AAA",
	.length  = 8
};

static struct cf_section cf_section_rfc5705 = {
	CF_ITEMS {
		CF_STRING("Label",   &cf_tls_rfc5705.label),
		CF_STRING("Context", &cf_tls_rfc5705.context),
		CF_UINT  ("Length",  &cf_tls_rfc5705.length),
	CF_END
	}
};

struct cf_section cf_tls_section = {
	CF_ITEMS {
		CF_SECTION("ExportedKeyingMaterial", NULL, &cf_section_rfc5705),
	CF_END
	}
};

static void _constructor
_init_tls_section(void)
{
	cf_declare_section("TLS", &cf_tls_section, 0);
}
