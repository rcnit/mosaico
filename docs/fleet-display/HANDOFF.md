# Handoff: fleet display and assistant projects

Written 2026-10-28 by the cloud session `mosaico-b1` (repo `rcnit/mosaico`). Hand this to the local session running on Rafael's Mac, which can reach the Director and the other sessions. Read `DESIGN.md` in this folder for the full detail; this note is the summary and the to-do list.

## Where the work lives

- Repo: `rcnit/mosaico`, branch `claude/laughing-allen-7smsjy`, head `cf86c75` (or later).
- `docs/fleet-display/DESIGN.md`: design notes and decisions.
- `docs/fleet-display/fleet.schema.json`: the `fleet.json` v1 contract. The generator must validate against it before publishing.
- `docs/fleet-display/fixtures/fleet.sample.json`: synthetic feed that validates against the schema. No real fleet data.
- `firmware/fleet_display/`: device-side code, starting with `components/fleet_core/`.

Run `make test` in `firmware/fleet_display` to check the host tests.

## The two projects

**A. Fleet dashboard on the Tab5 (in progress, the priority).**

- The existing dashboard is artifact `ECiQgd6D5Hh1EaChnL6zaM`. It reads nine database collections (`needs`, `sessions`, `week`, `workstreams`, `rules`, `dates`, `decisions`, `index`, `meta`) and live-updates from them. Only the "Director" session writes the rows. Its database rule allows writes only to the `admin` role.
- The Tab5 can't run the web page. It needs a native LVGL UI that shows the Board view: Needs you, Sessions, This week, and Workstreams. The reference tabs are out of scope for v1.
- The data comes from a feed generator on the Mac. It writes `fleet.json` every few minutes via a launchd agent, and a read-only HTTP server on the LAN serves it.

**B. AI assistant, notes, and to-dos (planned, not started).**

- Target: the ESP-Mosaico (ESP32-S31, on preorder). ESP-Claw runs on it natively.
- The Core2 (classic ESP32) can't run ESP-Claw. It would need custom firmware, so it's not a placeholder for B.
- The tools (calendar, notes, to-dos) would sit on a backend that holds the Google OAuth, exposed over MCP. ESP-Claw is an MCP client.
- LLM choice: a Gemini API key with billing, through ESP-Claw's custom OpenAI-compatible provider. A Gemini subscription doesn't cover API calls. Verify the base URL and tool-calling quality before committing. ESP-Claw also accepts Anthropic-compatible endpoints.

## Hardware

- Terminal Tab5 Kit K145 (ESP32-P4, 5" 1280×720 touch, Wi-Fi 6 via the C6, removable battery). Arrived or arriving 2026-10-28. The target for project A.
- Tab5 Keyboard A164 (70-key I2C). Not needed for v1. Its attachment to the K145 kit is unverified.
- Core2 V1.1: placeholder only. Rafael decided not to use it as an early test.
- ESP-Mosaico: preorder, expected later.

ESP-Claw's supported chips (per its docs): ESP32-S3, ESP32-P4, ESP32-C5, ESP32-S31. The Tab5 isn't on its board list that I checked, so B on the Tab5 would need board adaptation. A doesn't need ESP-Claw.

## Decisions Rafael has made

1. Firmware work starts now, on the Tab5.
2. LAN-only. The server binds to the LAN interface and the router does not forward the port. No bearer token.
3. No Core2 test device.
4. `NEEDS_YOU.md` format is proposed, pending the Director's confirmation. See below.

## Feed contract (`fleet.json` v1)

- Top level: `schema` (1), `generated_at` (ISO 8601 with offset), `clocks` (labels only, the device uses SNTP and POSIX TZ), `needs_you`, `sessions`, `decisions_recent`, `pending`, `ledger_tail`, `counts`.
- Size cap: 16 KB. Every field has a length or count limit in the schema.
- Content rules (hard): no secrets, no keys, tokens, or Keychain values. No BIT results or train-data figures (no verdicts, hashes, sizes, timings, or resource numbers). No sealed-period detail (DECISIONS 63). Profanity trimmed. The device is read-only.
- The generator uses an allowlist of fields, not a denylist. It validates before publishing and keeps the last good file if validation fails. Its content scan is a backstop.

## Proposed `NEEDS_YOU.md` format (pending the Director)

- The Director rewrites the whole file on each run.
- First line: `updated: <ISO 8601 timestamp with offset>`.
- Then one item per line: `- <urgent|today|later> | <where> | <text>`. `where` ≤ 60 characters; `text` ≤ 160.
- Blank lines and `#` lines are ignored. Malformed lines are dropped with a warning.
- If the file is missing or its `updated` is stale, `needs_you` is empty and the Needs section shows STALE.

## Firmware status (what's built)

`firmware/fleet_display/components/fleet_core/`:

- `fleet_parse_iso8601()`: parses `YYYY-MM-DDTHH:MM:SS` followed by `Z` or `±HH:MM`. Rejects fractional seconds and anything malformed. Returns Unix epoch seconds.
- `fleet_is_stale()`: stale if `generated_at` is more than `FLEET_STALE_AFTER_S` (600 s) old, or more than `FLEET_MAX_FUTURE_SKEW_S` (300 s) ahead of now.
- Tests: `test/test_fleet_time.c`, run by `make test`. Built with `-std=c11 -Wall -Wextra -Werror -pedantic`. All checks pass. Expected epochs were cross-checked against Python: `2026-10-28T13:10:00Z` = 1793193000.

Not built yet: anything that touches the device hardware.

## To-do, in order

**Mac side (needs Rafael's OK for anything installed or run):**

1. Write the feed generator. Read the governance files: `DECISIONS.md` (recent entries and Pending), `LEDGER.md` (last N rows), `DIRECTOR_HANDOFF.md` (session table), and `NEEDS_YOU.md`. Build `fleet.json` per the schema, applying the content rules.
2. Serve it. Read-only HTTP on the LAN interface, with an ETag. Python's `http.server` doesn't send one by default, so either add it or write a small server. Don't use NAS A or the company NAS.
3. Get the Director to confirm the `NEEDS_YOU.md` format, and to write the file on each run.
4. Code review the generator before the launchd agent is installed. The Code Reviewer should check it.
5. Install the launchd agent (a separate Rafael OK).

**Device side (after the Tab5 is on the bench):**

6. Check the official M5Stack Tab5 docs for the ESP-IDF version and the BSP. A third-party blog recommends ESP-IDF 5.4.2, but that's unverified. The component registry has `espressif/m5stack_tab5` with LVGL examples.
7. Check the Tab5's display revision. ESPHome supports only the original ILI9881C driver for now; newer revisions use ST7123 or ST7121.
8. Install ESP-IDF on the Mac, not in the cloud container (it has no ESP-IDF and no room for the multi-GB install).
9. Bring up Wi-Fi, SNTP, and an HTTP fetch with ETag and `If-None-Match`.
10. Parse the JSON with the ESP-IDF cJSON component into fixed-size buffers, sized for the 16 KB cap.
11. Wire `fleet_is_stale()` to show STALE.
12. Build the LVGL Board UI: green-on-black Matrix look, blinking cursor, code rain at low frame rate. Turn the rain off when idle.

**Decisions still open:**

- Should the Director's refresh of the dashboard artifact run as a scheduled task? If so, at what cadence? The Director's writes go through the artifact database, so this is the piece that keeps the web dashboard live without a session open.
- Project B: write its own plan once the Mosaico arrives. Decide the backend host and the Google Cloud project for Calendar.

## Notes for whoever picks this up

- The cloud session can't see the Mac, its files, or local sessions. Use the Mac session for anything under `~/WIP/...`.
- Commits: follow the repo's conventions. No model names in commit messages or PR text. Commit and push to `claude/laughing-allen-7smsjy`. Don't open a PR unless asked.
- The Director's brief is the requirements source for the feed. If the brief and this note disagree, the brief wins; tell Rafael.
- Don't put real fleet data in the repo. Use the synthetic fixtures.
