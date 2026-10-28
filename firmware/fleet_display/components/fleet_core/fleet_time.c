#include "fleet_time.h"

#include <stddef.h>
#include <string.h>

static bool read_digits(const char *p, int n, int *out)
{
    int v = 0;
    for (int i = 0; i < n; i++) {
        if (p[i] < '0' || p[i] > '9') {
            return false;
        }
        v = v * 10 + (p[i] - '0');
    }
    *out = v;
    return true;
}

/* Days since 1970-01-01 for a proleptic Gregorian date (Howard Hinnant's algorithm). */
static int64_t days_from_civil(int y, unsigned m, unsigned d)
{
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int64_t)doe - 719468;
}

int fleet_parse_iso8601(const char *s, int64_t *epoch_out)
{
    if (s == NULL || epoch_out == NULL) {
        return -1;
    }
    /* Shortest valid form is "YYYY-MM-DDTHH:MM:SSZ" (20 chars). */
    size_t len = strlen(s);
    if (len < 20 || s[4] != '-' || s[7] != '-' || s[10] != 'T' || s[13] != ':' || s[16] != ':') {
        return -1;
    }

    int year, month, day, hour, min, sec;
    if (!read_digits(s, 4, &year) || !read_digits(s + 5, 2, &month) || !read_digits(s + 8, 2, &day) ||
        !read_digits(s + 11, 2, &hour) || !read_digits(s + 14, 2, &min) || !read_digits(s + 17, 2, &sec)) {
        return -1;
    }
    if (month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || min > 59 || sec > 60) {
        return -1;
    }

    /* Offset from UTC in seconds, parsed from the tail. */
    int offset_s = 0;
    const char *tail = s + 19;
    if (*tail == 'Z' && tail[1] == '\0') {
        offset_s = 0;
    } else if ((*tail == '+' || *tail == '-') && len == 25 && tail[3] == ':') {
        int oh, om;
        if (!read_digits(tail + 1, 2, &oh) || !read_digits(tail + 4, 2, &om) || oh > 23 || om > 59) {
            return -1;
        }
        offset_s = (oh * 3600 + om * 60) * (*tail == '-' ? -1 : 1);
    } else {
        return -1;
    }

    int64_t days = days_from_civil(year, (unsigned)month, (unsigned)day);
    int64_t local = days * 86400 + hour * 3600 + min * 60 + sec;
    *epoch_out = local - offset_s;
    return 0;
}

bool fleet_is_stale(int64_t now_epoch, int64_t generated_epoch)
{
    if (generated_epoch > now_epoch + FLEET_MAX_FUTURE_SKEW_S) {
        return true;
    }
    return now_epoch - generated_epoch > FLEET_STALE_AFTER_S;
}
