#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Feed is considered stale when generated_at is older than this (DESIGN.md). */
#define FLEET_STALE_AFTER_S 600

/* A generated_at this far in the future means the clocks disagree; treat as stale. */
#define FLEET_MAX_FUTURE_SKEW_S 300

/*
 * Parse an ISO 8601 timestamp of the form YYYY-MM-DDTHH:MM:SS followed by
 * either "Z" or a numeric offset +HH:MM / -HH:MM. Fractional seconds are not
 * accepted. Returns 0 on success and writes Unix epoch seconds to *epoch_out;
 * returns -1 on any malformed input and leaves *epoch_out untouched.
 */
int fleet_parse_iso8601(const char *s, int64_t *epoch_out);

/*
 * True when the feed should be shown as STALE: its generated_at is older than
 * FLEET_STALE_AFTER_S, or is more than FLEET_MAX_FUTURE_SKEW_S ahead of now.
 */
bool fleet_is_stale(int64_t now_epoch, int64_t generated_epoch);
