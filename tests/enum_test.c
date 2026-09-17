#include "itrsng/enum.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)

static itrs_naptr_record_t naptr(uint16_t order, uint16_t pref, const char *flags,
                                 const char *service, const char *regexp) {
    itrs_naptr_record_t r = {.order = order, .preference = pref};
    snprintf(r.flags, sizeof(r.flags), "%s", flags);
    snprintf(r.service, sizeof(r.service), "%s", service);
    snprintf(r.regexp, sizeof(r.regexp), "%s", regexp);
    return r;
}

static int test_itrs_domain(void) {
    char domain[ITRS_ENUM_DOMAIN_MAX];
    CHECK(itrs_enum_domain_from_e164("+1 (801) 555-1212", ITRS_ENUM_DEFAULT_APEX, domain, sizeof(domain)) == 0);
    CHECK(strcmp(domain, "2.1.2.1.5.5.5.1.0.8.1.itrs.us.") == 0);
    return 0;
}

static int test_rfc6116_domain(void) {
    char domain[ITRS_ENUM_DOMAIN_MAX];
    CHECK(itrs_enum_domain_from_e164("+44-20-7946-0148", "e164.arpa", domain, sizeof(domain)) == 0);
    CHECK(strcmp(domain, "8.4.1.0.6.4.9.7.0.2.4.4.e164.arpa.") == 0);
    return 0;
}

static int test_rfc_style_sip_rewrite(void) {
    itrs_naptr_record_t records[] = {
        naptr(10, 11, "u", "E2U+sip", "!^(.*)$!sip:\\1@providerGW.example.com!"),
    };
    itrs_enum_result_t result;
    CHECK(itrs_enum_resolve_sip("+18015551212", ITRS_ENUM_DEFAULT_APEX, records, 1, &result) == 0);
    CHECK(strcmp(result.aus, "+18015551212") == 0);
    CHECK(strcmp(result.uri, "sip:+18015551212@providerGW.example.com") == 0);
    CHECK(result.order == 10 && result.preference == 11);
    return 0;
}

static int test_literal_sip_rewrite(void) {
    itrs_naptr_record_t records[] = {
        naptr(10, 101, "U", "e2u+sip", "!^.*$!sip:info@itrs.us!"),
    };
    itrs_enum_result_t result;
    CHECK(itrs_enum_resolve_sip("12025551212", "itrs.us.", records, 1, &result) == 0);
    CHECK(strcmp(result.uri, "sip:info@itrs.us") == 0);
    return 0;
}

static int test_order_then_preference(void) {
    itrs_naptr_record_t records[] = {
        naptr(20, 1, "u", "E2U+sip", "!^.*$!sip:later@example.invalid!"),
        naptr(10, 50, "u", "E2U+sip", "!^.*$!sip:second@example.invalid!"),
        naptr(10, 10, "u", "E2U+sip", "!^.*$!sip:first@example.invalid!"),
    };
    itrs_enum_result_t result;
    CHECK(itrs_enum_resolve_sip("+12025551212", ITRS_ENUM_DEFAULT_APEX, records, 3, &result) == 0);
    CHECK(strcmp(result.uri, "sip:first@example.invalid") == 0);
    CHECK(result.source_index == 2);
    return 0;
}

static int test_unsupported_records_are_skipped(void) {
    itrs_naptr_record_t records[] = {
        naptr(1, 1, "s", "E2U+sip", "!^.*$!sip:badflag@example.invalid!"),
        naptr(2, 1, "u", "E2U+h323", "!^.*$!sip:wrongservice@example.invalid!"),
        naptr(3, 1, "u", "E2U+sip", "!^.*$!sip:ok@example.invalid!"),
    };
    itrs_enum_result_t result;
    CHECK(itrs_enum_resolve_sip("+12025551212", ITRS_ENUM_DEFAULT_APEX, records, 3, &result) == 0);
    CHECK(strcmp(result.uri, "sip:ok@example.invalid") == 0);
    return 0;
}

static int test_nonmatching_rule_falls_through(void) {
    itrs_naptr_record_t records[] = {
        naptr(1, 1, "u", "E2U+sip", "!^\\+44123$!sip:no@example.invalid!"),
        naptr(2, 1, "u", "E2U+sip", "!^.*$!sips:yes@example.invalid!"),
    };
    itrs_enum_result_t result;
    CHECK(itrs_enum_resolve_sip("+12025551212", ITRS_ENUM_DEFAULT_APEX, records, 2, &result) == 0);
    CHECK(strcmp(result.uri, "sips:yes@example.invalid") == 0);
    return 0;
}

static int test_invalid_e164_fails_closed(void) {
    char aus[32];
    CHECK(itrs_enum_normalize_e164("+1-800-FLOWERS", aus, sizeof(aus)) == EINVAL);
    CHECK(itrs_enum_normalize_e164("+0123", aus, sizeof(aus)) == EINVAL);
    CHECK(itrs_enum_normalize_e164("+1234567890123456", aus, sizeof(aus)) == EOVERFLOW);
    return 0;
}

static int test_capacity_fails_closed(void) {
    itrs_naptr_record_t records[ITRS_ENUM_MAX_RECORDS + 1] = {0};
    itrs_enum_result_t result;
    CHECK(itrs_enum_resolve_sip("+12025551212", ITRS_ENUM_DEFAULT_APEX,
                                records, ITRS_ENUM_MAX_RECORDS + 1, &result) == EOVERFLOW);
    return 0;
}

int main(void) {
    int (*tests[])(void) = {
        test_itrs_domain, test_rfc6116_domain, test_rfc_style_sip_rewrite, test_literal_sip_rewrite,
        test_order_then_preference, test_unsupported_records_are_skipped,
        test_nonmatching_rule_falls_through, test_invalid_e164_fails_closed,
        test_capacity_fails_closed,
    };
    const char *names[] = {
        "iTRS ENUM domain", "RFC 6116 domain", "backreference SIP rewrite", "literal SIP rewrite",
        "NAPTR order/preference", "unsupported records skipped", "nonmatch fallthrough",
        "invalid E.164 rejected", "NAPTR capacity fails closed",
    };
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        int rc = tests[i]();
        if (rc != 0) return rc;
        printf("ok %zu - %s\n", i + 1, names[i]);
    }
    return 0;
}
