# M4 — On-Demand Data Export over SoftAP (Codex Implementation Brief)

## Goal
Add an operator-initiated "Download Data" flow. WiFi runs **only** during an export session, then tears fully down. Operator scans an on-screen QR to join the device's access point, a captive portal opens an instructions page, they download a CSV of the device's data.

## Fixed constraints (do not deviate)
- WiFi is **OFF in all normal operation**. It is active only inside the export session. Reason: the 800×480 RGB parallel panel's framebuffer lives in PSRAM; WiFi contends for PSRAM bandwidth and causes visible flicker. Keep WiFi-active time minimal.
- Mode is **SoftAP, open** (no password). Do **not** use STA, and do **not** hardcode any site WiFi credentials.
- **Never block the LVGL loop.** AP bring-up, captive-portal DNS, and HTTP handling are polled non-blocking from the existing loop. A blocking `while (WiFi.status() != ...)` is not acceptable.
- Do not regress existing touch / calibration / display behavior.

## Stack
PlatformIO, Arduino-ESP32 core, LovyanGFX (display), LVGL (UI). Use core `WiFi.h` + `WebServer.h` + `DNSServer.h` — all ship with arduino-esp32, no extra deps. QR via LVGL's built-in qrcode widget (no extra dependency; bundled).

## State machine
Suggested enum `export_state_t`:
- `IDLE` — normal UI, WiFi off.
- `STARTING_AP` — SoftAP + server + DNS coming up (transient; SoftAP is near-instant).
- `READY` — AP up, server live, QR + instructions shown. DNS + HTTP polled each loop.
- `SHUTDOWN` — tearing down, then back to IDLE.

Transitions:
- `IDLE → STARTING_AP`: operator taps **Download Data**.
- `STARTING_AP → READY`: SoftAP started and `WiFi.softAPIP()` valid (immediate for AP — no DHCP wait for the AP itself).
- `READY → SHUTDOWN`: operator taps **Done**, OR inactivity timeout (no HTTP request for `EXPORT_TIMEOUT_MS`, default `180000`).
- `SHUTDOWN → IDLE`: teardown complete.

Guard: the **Download Data** button is live only in `IDLE`; ignore re-press in other states.

## Data snapshot
On entering `STARTING_AP`, build the CSV once into a buffer (a `String` or static buffer) and serve that snapshot for the whole session so re-downloads are consistent.

Source is the in-RAM pull history owned by `StateManager` — access via `StateManager::getPullHistory()` / `StateManager::getPullHistoryCount()` (the `preferences` member is **private**; do not reference `preferences.pullHistory[]` directly). `#include "StateManager.h"`. The builder reads RAM only; **do not** touch storage during the session. Export **all** stored pulls. History is append-with-shift-on-full: at `MAX_PULL_HISTORY` the oldest row is dropped, so the array is always oldest→newest (not circular).

Columns, one row per `PullResult` (`pull_index` prepended, `timestamp` exported as `uptime_s`):
`pull_index, driverName, driverNumber, className, classWeight, maxSpeedMPH, maxDistanceFeet, maxRPM, uptime_s`
- `pull_index` = array position (1-based) = chronological order of the currently-stored rows (oldest→newest). Order by this, never by the timestamp. It's a position, not a durable pull ID.
- `driverName` and `className` are free-text `String`s → CSV-quote them and escape internal quotes (e.g. `Smith, Jr.` must not split the row).
- `uptime_s` is `PullResult.timestamp` (`unsigned long`) = `millis()/1000` = seconds-since-boot, **not** wall-clock: resets each boot, non-monotonic across the array after a reboot. Emit raw; never format as a date.
- Match the real `PullResult` field names/types in the codebase.

## Web routes
- `GET /` → HTML landing page (see **Landing page**).
- `GET /data.csv` → the snapshot CSV, headers:
  - `Content-Type: text/csv`
  - `Content-Disposition: attachment; filename="m4_data.csv"` (use a session timestamp in the name if one is available).
- Catch-all (`server.onNotFound`) → **302 redirect to `/`**. This handles the OS captive-portal probe requests (Android `generate_204`, iOS `hotspot-detect.html`, Windows `ncsi.txt`, etc.) and triggers the "sign in to network" popup.

Every handler updates a `last_request_ms = millis()` timestamp (drives the inactivity timeout). Do **not** trigger shutdown from a `/data.csv` GET.

## Captive portal
- Start `DNSServer` on port 53, wildcard `"*"` → `WiFi.softAPIP()`. Call `dnsServer.processNextRequest()` every loop while in `READY`.
- This is what makes "scan → page opens" work. The DNS wildcard sends every hostname to the device; `onNotFound` redirects every path to `/`. If a given phone doesn't auto-pop, the on-screen text fallback still lets the operator browse to the IP manually.

## On-screen QR (WiFi-join)
- LVGL qrcode widget. Requires `LV_USE_QRCODE 1` in `lv_conf.h` — add if missing (no extra dependency, just the flag).
- Encode a **WiFi-join** string, **not** a URL:
  - `WIFI:T:nopass;S:<SSID>;;` (open network)
- iOS Camera and Android both recognize this and prompt to join the AP; the captive portal then opens `/`.
- Render below the QR, as text fallback: the network name `<SSID>` and `http://192.168.4.1`.
- Use the v8.3 API: `lv_qrcode_create(parent, size, dark_color, light_color)` then `lv_qrcode_update(qr, data, strlen(data))`; delete with `lv_obj_del()` on rebuild. This differs from v9 (`create()` + separate setters) — do not copy v9 examples.

## Landing page (HTML at `/`)
Fully self-contained — inline CSS, no external assets (the phone has no internet while on the AP). Contents:
- Title / what this is ("M4 Data Export").
- One-line instruction ("Tap to download all pull history stored on the device as a CSV.").
- A prominent **Download CSV** link/button → `/data.csv`.
- Optional: row count + snapshot timestamp so the operator can confirm freshness.
- Optional: a line noting the device closes this network when done.

## WiFi / server lifecycle
Bring-up (`STARTING_AP`):
```cpp
WiFi.mode(WIFI_AP);
WiFi.softAP(ssid);                       // open, no password, ssid = "M4-XXXX"
dnsServer.start(53, "*", WiFi.softAPIP());
server.on("/", handleRoot);
server.on("/data.csv", handleCsv);
server.onNotFound(handleCaptive);        // 302 -> "/"
server.begin();
```
Teardown (`SHUTDOWN`) — order matters:
```cpp
server.stop();
dnsServer.stop();
WiFi.softAPdisconnect(true);
WiFi.mode(WIFI_OFF);
```
Loop (only while `READY`):
```cpp
dnsServer.processNextRequest();
server.handleClient();
// if (millis() - last_request_ms > EXPORT_TIMEOUT_MS) -> SHUTDOWN
```
Call `data_export_loop()` **unconditionally** from the main loop; it early-returns unless `state == READY`. Gate on state, not on the active screen — a screen-identity check can desync from the AP's actual state.

## SSID
Per-unit, e.g. `M4-<last 3 bytes of efuse MAC>` so multiple devices on a bench don't collide. Derive at runtime; the SSID string also feeds the QR. (Keep it alphanumeric+hyphen so no QR escaping is needed.)

## UI wiring
SquareLine (LVGL 8.3.11) screens and buttons already exist. The three callbacks are empty stubs in the **generated** `ui/ui_events.cpp` — implement their bodies as **thin wrappers** that call into `data_export` (and the screen-build helper) so the real logic lives in non-generated files and survives a re-export.

- **Download** (PullHistoryScreen) → `downloadPullHistoryButtonClicked()`: call `data_export_start()` only. Do **not** touch ExportScreen widgets here — the screen change fires *after* this callback, so `ui_ExportScreenPanelinstructionPanel` is still NULL at this point.
- **ExportScreen loaded** → `exportScreenLoaded()`: the panel exists by now, so build the UI here. Populate the existing container `ui_ExportScreenPanelinstructionPanel` (no separate QR placeholder) with the QR + network/IP/instruction text.
  - First call `lv_obj_clean(ui_ExportScreenPanelinstructionPanel)` — SquareLine reuses screens, so this fires again on every re-entry; cleaning first prevents stacking a second QR.
  - The panel ships with no flex layout and non-scrollable — set a flex column on it at runtime so the QR + text stack/center, and give the QR a light background + small border (quiet zone) so it scans over the panel's transparent bg.
  - SoftAP comes up fast, so by load `WiFi.softAPIP()` is valid and you can build the QR directly; no "Starting…" intermediate needed in practice.
- **Done** (ExportScreen) → `exportDoneButtonClicked()`: call `data_export_stop()` (full teardown). The generated screen-change back to PullHistoryScreen runs after.

## Pitfalls / do-not
- Do not leave WiFi initialized after the session — full teardown to `WIFI_OFF`.
- Do not block the LVGL loop anywhere (no busy-wait on WiFi).
- Do not trigger shutdown from a single `/data.csv` GET (failed / partial / re-pull). Use **Done** + timeout only.
- Expect mild panel flicker while `READY`; keep the export screen simple/high-contrast. Verify flicker is gone after teardown.
- Open AP is intentional (data not sensitive) — do not add a password; it keeps the nopass QR join frictionless.

## Resolved inputs (verified against repo)
- Data: `StateManager::getPullHistory()` / `getPullHistoryCount()` (`preferences` is private); `#include "StateManager.h"`. Append-with-shift-on-full, not circular.
- `timestamp` = `millis()/1000` = seconds-since-boot → exported as `uptime_s`. Run order = `pull_index`, not the timestamp.
- Callbacks: empty stubs in generated `ui/ui_events.cpp`. Loop hook: `loop()` in `main.cpp` (not `lvgl_task`). QR widget confirmed compiling + linking.
- Heads-up: repo builds two LVGL trees (vendored `lib/lvgl` + PlatformIO dep). Builds fine; ensure QR uses the active `lv_conf.h`. Worth cleaning up later.

## Out of scope (forward-compat note)
A future driver-list upload (operator selects who's up) is **not** in scope — do **not** add upload routes, forms, or driver storage now. It will likely reuse this same SoftAP session + web server via an added `POST` route, so keep the session/lifecycle machinery (AP, DNS, server start/stop, timeout) decoupled from the route handlers (the CSV builder) so an upload handler drops in later without touching the lifecycle. No further abstraction needed now.

## Acceptance criteria
1. In normal operation WiFi is off (no AP broadcast).
2. Tapping **Download Data** brings up AP `M4-XXXX` within ~1s; export screen shows QR + IP.
3. Scanning the QR joins the AP; the instructions page auto-opens via the captive portal.
4. **Download CSV** returns a valid file containing all `pullHistoryCount` rows, with `driverName` / `className` correctly quoted.
5. Tapping **Done** tears WiFi fully down and returns to normal UI; AP no longer broadcasts; flicker gone.
6. Walking away (no requests) auto-shuts-down after `EXPORT_TIMEOUT_MS`.
7. Re-entering export works repeatedly with no leak or crash.
