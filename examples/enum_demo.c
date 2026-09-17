#include "itrsng/enum.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    itrs_naptr_record_t records[2] = {0};
    records[0].order = 10; records[0].preference = 11;
    snprintf(records[0].flags, sizeof(records[0].flags), "u");
    snprintf(records[0].service, sizeof(records[0].service), "E2U+sip");
    snprintf(records[0].regexp, sizeof(records[0].regexp), "!^(.*)$!sip:\\1@providerGW.example.com!");
    records[1].order = 20; records[1].preference = 10;
    snprintf(records[1].flags, sizeof(records[1].flags), "u");
    snprintf(records[1].service, sizeof(records[1].service), "E2U+sip");
    snprintf(records[1].regexp, sizeof(records[1].regexp), "!^.*$!sip:fallback@example.com!");

    itrs_enum_result_t result;
    int rc = itrs_enum_resolve_sip("+18015551212", ITRS_ENUM_DEFAULT_APEX, records, 2, &result);
    if (rc != 0) return rc;
    printf("AUS=%s\nquery=%s\nuri=%s\norder=%u preference=%u\n",
           result.aus, result.query_domain, result.uri, result.order, result.preference);
    return 0;
}
