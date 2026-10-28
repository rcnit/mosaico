#include <stdio.h>
#include <stdlib.h>

#include "fleet_time.h"

static int failures = 0;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            fprintf(stderr, "%s:%d: FAILED: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                \
        }                                                              \
    } while (0)

static void parse_expect(const char *s, int64_t expected)
{
    int64_t got = -1;
    int rc = fleet_parse_iso8601(s, &got);
    if (rc != 0 || got != expected) {
        fprintf(stderr, "parse \"%s\": rc=%d got=%lld want=%lld\n", s, rc, (long long)got, (long long)expected);
        failures++;
    }
}

static void parse_reject(const char *s)
{
    int64_t got = 12345;
    int rc = fleet_parse_iso8601(s, &got);
    if (rc == 0 || got != 12345) {
        fprintf(stderr, "parse \"%s\" should be rejected\n", s);
        failures++;
    }
}

int main(void)
{
    /* Known epochs: 1970-01-01 and 2026-10-28T13:10:00Z. */
    parse_expect("1970-01-01T00:00:00Z", 0);
    parse_expect("2026-10-28T13:10:00Z", 1793193000);
    parse_expect("2000-02-29T00:00:00Z", 951782400); /* leap day */

    /* Offsets: 13:10 at UTC-03:00 is 16:10Z; 13:10 at +01:00 is 12:10Z. */
    parse_expect("2026-10-28T13:10:00-03:00", 1793203800);
    parse_expect("2026-10-28T13:10:00+01:00", 1793189400);

    parse_reject("");
    parse_reject("2026-10-28");
    parse_reject("2026-10-28 13:10:00Z");
    parse_reject("2026-13-28T13:10:00Z");
    parse_reject("2026-10-28T25:10:00Z");
    parse_reject("2026-10-28T13:10:00");
    parse_reject("2026-10-28T13:10:00.123Z");
    parse_reject("2026-10-28T13:10:00+0100");
    parse_reject("2026-1x-28T13:10:00Z");
    parse_reject(NULL);

    const int64_t gen = 1793193000;
    CHECK(!fleet_is_stale(gen, gen));
    CHECK(!fleet_is_stale(gen + FLEET_STALE_AFTER_S, gen));
    CHECK(fleet_is_stale(gen + FLEET_STALE_AFTER_S + 1, gen));
    CHECK(!fleet_is_stale(gen - FLEET_MAX_FUTURE_SKEW_S, gen));
    CHECK(fleet_is_stale(gen - FLEET_MAX_FUTURE_SKEW_S - 1, gen));

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return EXIT_FAILURE;
    }
    printf("fleet_time: all checks passed\n");
    return EXIT_SUCCESS;
}
