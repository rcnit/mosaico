# Fleet display on ESP32: design notes

Status: draft, written 2026-10-28 from the Director's brief. Nothing installed on the Mac; no firmware flashed.

## Pieces

1. **Feed generator (Mac).** Builds `fleet.json` every few minutes from the governance files. Run by a launchd agent (needs Rafael's OK). Not yet written.
2. **Serving (home LAN only).** Read-only HTTP on the Mac, or an MQTT retained topic. See the recommendation below.
3. **Device.** Polls the feed, validates it, renders the Board view. Read-only.

The contract is `fleet.schema.json` in this folder. `fixtures/fleet.sample.json` is synthetic and validates against it.

## Hardware

- **Tab5 (ESP32-P4, 5" 1280×720 TFT, Wi-Fi via C6):** best fit for the Matrix look and the Board layout. It is the target.
- **Core2 (classic ESP32, 320×240):** can run a plain HTTP fetch and render. It can't run ESP-Claw, but this display doesn't need it. Fine as a first test; the Board layout will be cramped.
- **E-paper:** the brief mentions it for a wall display. Not recommended for Matrix green-on-black with a blinking cursor and code rain. Park it.

## Serving: recommendation

Prefer the **HTTP option**:

- Simpler than running an MQTT broker, and no broker to open to the LAN.
- Device polls every 30–60 s, uses `If-None-Match` (ETag from a hash of the file) so unchanged polls are cheap.
- The stock `python -m http.server` sends no ETag, so the generator or a small server must set one.

Security, which the brief doesn't cover yet:

- Plain HTTP with no auth exposes the feed to anything on the home network. Bind to the LAN interface only, and require a shared bearer token that the device holds. Rotate it if the device is lost.
- Don't expose the port through the router. Keep it off any guest or IoT network the Mac can reach.
- The device stores Wi-Fi credentials and the token in flash, which can be extracted with physical access. Acceptable for a home display; don't reuse a token that unlocks anything else.

## Generator rules

- **Allowlist, not denylist.** Build each output field from named sources. Don't copy whole rows or files through.
- **Fail closed.** Before publishing, validate against `fleet.schema.json` (size ≤ 16 KB too). On failure, keep the last good file and let it go stale. Never publish a partial file.
- **Content scan before publish.** Reject the file if it matches patterns for keys, tokens, or long hex strings, and for the BIT and sealed-period terms the brief lists. This is a backstop, not the main control.
- **Timestamps.** Emit ISO 8601 with offsets in `generated_at`. The device sets its clock by SNTP and uses POSIX TZ strings; the `clocks` field is just a label, so the device should not depend on it.
- **Staleness.** The device compares `generated_at` to its own clock and shows STALE after about 10 minutes. A crashed generator therefore shows as STALE, not as old data.

## Device behaviour

- Fetch with ETag; parse with a fixed-size JSON buffer sized for 16 KB; reject any response that fails parsing and keep showing the last good state.
- Read-only. Nothing on the screen counts as Rafael's OK for a gated action. The brief's keyboard-to-inbox idea is a new write path. Keep it out of v1, and give it its own design if it's wanted later.
- Matrix skin: green-on-black, a blinking cursor, code rain at low frame rate. Turn rain off when the device is idle to save power and avoid burn-in.

## Decisions

1. **Firmware work starts now** on the Tab5. The launchd agent and server still wait for the Mac custodian.
2. **LAN-only, no bearer token.** The server binds to the LAN interface and the router doesn't forward the port.
3. **No Core2 test device.** The Tab5 arrives 2026-10-28.
4. **`NEEDS_YOU.md` format (proposed, pending the Director's confirmation):**
   - The Director rewrites the whole file on every run, so it never goes stale mid-edit.
   - First line: `updated: <ISO 8601 timestamp with offset>`.
   - Then one item per line: `- <urgent|today|later> | <where> | <text>`, with `where` at most 60 characters and `text` at most 160.
   - Blank lines and lines starting with `#` are ignored.
   - The generator drops malformed lines with a warning rather than failing the whole feed.
   - If the file is missing or its `updated` is older than the feed's threshold, `needs_you` is empty and the Board shows STALE for that section.

## Firmware status

- `firmware/fleet_display/components/fleet_core/`: pure-C ISO 8601 parsing and staleness logic, host-tested with `make test` (no ESP-IDF needed).
- On-device build (ESP-IDF, Tab5 BSP, LVGL) comes after the board arrives. The Tab5 BSP on the component registry and the ESP-IDF version need checking against the official M5Stack docs first.

## Needs Rafael's OK (via the Mac custodian)

- the launchd agent;
- the HTTP server or MQTT broker, and any port opened on the Mac;
- any firmware flashing tool or download, from a trusted source.

The generator and firmware should get a code review before the launchd agent is installed.
