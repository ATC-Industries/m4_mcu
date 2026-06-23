# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Added

- Main-screen Silence Alarm button support: the hidden `silence alarm` control now appears automatically while an audible alarm is active and can mute current alarm horns on release
- Alarm horn runtime scheduling now runs non-blocking from the main loop, allowing audible alarm patterns without stalling UI, touch, or telemetry updates
- Top-of-file horn tuning knobs in `AlarmManager.cpp` now centralize per-style horn timings and semantics so alarm sound behavior can be adjusted without digging through runtime logic

### Changed

- Alarm horn runtime now supports per-alarm silence latching, so a silenced alarm stays quiet until that specific alarm resets and is tripped again
- If multiple alarms are sounding at once, the silence action now mutes all currently active audible alarms together instead of only affecting the current horn pulse
- Distance alarm visuals on the main screen now follow live enabled/tripped state the same way speed and RPM indicators do
- Alarm styles are now audibly differentiated in runtime behavior: `TRIP_ONCE` uses a crossover-only beep, `HOLD_AUTORESET` uses a continuous horn only while over threshold, `HOLD_PERSISTENT` can continue sounding while latched, and `AUTO_END_RUN_DQ` keeps a distinct triple-pulse alert
- `AUTO_END_RUN_DQ` alarms now latch and sound their DQ horn pattern but no longer force a `PULLEND` transition automatically, preventing max-value / end-of-run UI from taking over during a live pull
- Stage button now says "Press to Stage" below the large "STAGE" text

### Fixed

- `HOLD_AUTORESET` alarms now respect horn silencing correctly instead of resuming their continuous horn immediately while the alarm condition is still active
- Silencing an alarm now stops the horn immediately and re-hides the silence button until a new audible alarm condition is triggered
- Alarm runtime reset paths now clear per-alarm horn silence state consistently when alarms disable, reset, mirror-clear, or begin a new run
- `RPM #2` alarm control wiring from SquareLine now correctly re-enables its own color dropdown after the toggle is turned back on
- `TRIP_ONCE` horn behavior now matches the alarm help text again, firing only once at threshold crossover instead of acting like a sustained hold alert
- Mid-pull max-value / `PULLEND` UI takeover caused by alarm-triggered DQ handling has been removed

---

## [0.0.6-a] - 2026-06-21

### Added

- On-demand pull-history export over a temporary SoftAP, including CSV snapshot generation from in-memory pull history
- New ExportScreen flow with QR-guided Wi-Fi join and browser navigation for CSV download
- Browser landing page for downloading the current pull-history CSV during an export session

### Changed

- Export UX no longer uses a captive portal; the device now shows two QR codes: one to join the network and one to open the browser page
- ExportScreen instructions now include direct manual fallback steps with the SSID and page URL shown on-device
- Export screen text now uses the LVGL default font for broader glyph coverage
- Judge Stand now mirrors the sled host as an authoritative read-only display, including host-selected units, alarm configuration/state, pull metadata, and pull history sync (#15, #25)
- Judge Stand host selection now requires an explicit Host MCU ID instead of passively accepting any broadcaster (#7)

### Fixed

- Screen jerking caused by synchronous preference writes has been reduced by deferring NVS saves out of the immediate UI path, including the pull Save flow (#4)
- Distance value and distance progress bar no longer pop back to the previous pull after Save; runtime distance state is now cleared consistently on return to READY (#5)
- Brightness slider now persists correctly when released, saved brightness is reapplied on reboot, and the Settings screen label now reflects the stored brightness value on first load
- Pull Save now updates pull history and driver number in RAM first, avoiding immediate blocking persistence in the visible SAVE transition path
- Pull history persistence is now narrower and lower-overhead, using packed per-row storage instead of rewriting row-by-row field keys during runtime saves
- Driver number persistence now uses its own narrow write path instead of forcing a broad general-preferences rewrite
- Judge Stand end-of-run current/max values now come directly from the sled host, fixing one-off mismatches at STOP / PULLEND (#1)
- Judge Stand speed alarm state now mirrors the sled host instead of drifting locally, and judge-side speed/RPM alarm indicator flicker caused by config refreshes clearing live trip state has been removed (#22)
- Judge Stand screen jerking has been reduced by avoiding row-by-row history persistence during mirror sync
- Judge Stand idle screen shimmy / bouncing has been reduced substantially by suppressing redundant LVGL state-container and main-screen redraw work when visible state has not changed
- Judge Host MCU ID entry now saves the tracked host ID correctly instead of overwriting the local unit ID, and judge units now ignore judge traffic until a Host MCU ID is configured (#7)
- Judge Stand database / pull history now mirrors from the sled host instead of staying local to the judge unit (#15, #25)
- Judge Stand now mirrors sled-side tach enable, limit enable, and relay enable states instead of falling back to the judge unit's local settings (#21)
- Judge Stand main-screen title now shows `M4 Remote Monitor - <version>` while in judge mode instead of reusing the sled class title (#13)
- Judge Stand horn behavior for staged alarms is now effectively disabled because judge alarm evaluation is suppressed in judge mode; no separate option has been added yet (#24)

### Technical

- Added `data_export.{h,cpp}` outside `ui/` to own export session state, web serving, QR rendering, and teardown
- `ui/ui_events.cpp` export callbacks are now thin wrappers into `data_export`
- `main.cpp` now polls `data_export_loop()` from the main loop
- Preference writes are now batched and flushed from the main loop instead of synchronously during UI interactions
- Pull-history persistence now stores runtime rows as packed blobs, while readback remains compatible with previously stored history data
- SAVE-triggered persistence now follows a RAM-first, batch-later flow with controlled flush points for safer moments
- Export sessions pause ESP-NOW comms during SoftAP use and restore them on teardown
- Main-screen and pull-state UI updates now short-circuit when the visible state has not changed, reducing unnecessary LVGL churn on the judge display

### Notes

- Export Wi-Fi remains session-scoped and is torn down when the operator taps Done or the session times out
- Browser auto-open behavior now depends on scanning the explicit page QR or manually entering the shown URL
- Some screen jerkiness still remains. The worst cases were reduced, but the root cause is still being investigated.
- Current recommendation: for a production-quality removal of runtime screen flicker/jitter, move high-frequency persistence such as pull history and driver number to I2C FRAM, and keep NVS for low-frequency settings only.

## [0.0.5-alpha] - v0.0.5-alpha

### Added

- Boot splash screen (#27) — drawn directly via TFT (not LVGL) so the display is alive immediately while LVGL and other subsystems initialize. Splash text is customizable; logos deferred for now (more complex under TFT draw than we want to take on yet)
- Save-confirmation popup when a new calibration number is saved (#3)

### Changed

- Overhauled logging system: all debug and serial prints now categorized as info, warning, error, or debug. Logging verbosity is globally configurable, and debug messages can be disabled on a per-file basis
- Substantially reduced boot time (#27)
- Judgemode state buttons now greyed out at 100/255 opacity when disabled (#2)
- Increased delay after final speed pulse before ending pull state to ~seconds (#6) — feels like an eternity on the bench; may need tuning to match real-world expectations

### Fixed

- Calibration accuracy — do NOT calibrate with the protective film installed; it causes issues
- Rotation state persistence (#20) — feature was always present; corrupted EEPROM data was the culprit. Resolved by a clean EEPROM erase
- Screen jerking when using the brightness slider
- Physical button press now invokes a real button press instead of injecting an XY touch (#23)
- Rough scrolling (#8)
- Unnecessary highlighting of clicked cells (#8)
- Auto cal drive-off (#2)

### Notes

- Work in progress – not ready for release
- Open question: the green button currently still allows pressing the state buttons — should this be disabled?
- TODO: ship units with a clean EEPROM erase before returning code (#20)

## [0.0.4-alpha] - v0.0.4-alpha

### Added

- Pull history table with clear routine
- Pull data now saved to EEPROM (up to 255 pulls; space limitations TBD)
- Screen rotation toggle in settings
- TachClient module for TSS pairing and connection management with automatic RPM polling

### Changed

- Improved speed calculator to reduce spikes/noise
- Driver number now auto-increments on pull save
- Updated RGB display pin mappings to match new PCB (Rev B) IO definitions
- TSS pairing now uses multi-broadcast approach for improved reliability

### Fixed

- Linker errors for library global variables

### Notes

- Work in progress – not ready for release

## [0.0.3-alpha] - 2025-06-13

### Added

- Implemented TFT_Touch library for touch screen calibration
- Added persistent touch calibration storage using ESP32 Preferences
- Created modular touch handling system with proper separation of concerns
- Added 15-second touch test after calibration (no serial input required)
- Implemented touch debouncing for LVGL input handling with timeout-based approach
- Added recalibration flag system for development and troubleshooting
- Enhanced touch state management to prevent false press/release cycles

### Changed

- Migrated from previous touch system to Bodmer's TFT_Touch library
- Restructured touch code into separate touch.h/touch.cpp modules
- Improved calibration workflow with automatic save/load functionality
- Replaced orientation testing with fixed LCD rotation 0, touch rotation 1
- Enhanced error handling and debug output for calibration process
- Optimized touch input driver to handle "fat finger" detection with hysteresis

### Fixed

- Resolved multiple definition linker errors with proper extern declarations
- Fixed touch pin configuration (MOSI/MISO pin assignment)
- Corrected touch coordinate mapping for LVGL integration
- Improved calibration stability and phantom touch detection
- Eliminated erratic press/release behavior during continuous touch input
- Reduced false touch releases caused by momentary touch controller polling gaps

### Technical

- Touch calibration data stored in ESP32 NVS under "touch_cal" namespace
- Calibration uses 5-point system for accurate screen mapping
- Touch coordinates properly transformed for 800x480 display resolution
- Touch debouncing uses 100ms timeout to maintain pressed state during brief interruptions
- LVGL input driver now properly tracks state transitions vs. polling artifacts

### Notes

- Touch responsiveness prioritizes accuracy over speed to prevent false inputs
- Touch debouncing parameters may need adjustment for different hardware configurations
- This version establishes stable foundation for reliable touch input handling
- Ready for MPH and distance calculation implementation in next version

## [0.0.2-alpha] - 2025-06-12

### Known Issues

- Calibration mapping is offset from screen edges (0,0 appears at ~50,50)
- Touch input noisy; needs filtering or better scaling logic

### Fixed

- Touchscreen edge glitch caused by float underflow during coordinate mapping
- Touch mapping instability during diagonal gestures and edge presses
- Raw touch axis swap issue that caused incorrect calibration results
- Inaccurate touch mapping caused by incorrect calibration point references
- Touch inputs now register at the correct visual locations, including near screen edges
- Touch spike detection now filters phantom touches that appear diagonally from corners
- Corrected XPT2046 command bytes (X=0xD0, Y=0x90) to match standard chip configuration

### Changed

- `TouchScreen::mapRawToScreen()` now clamps float values before casting to `uint16_t`
- Touch pressure threshold increased from 200 to 250 for better noise rejection
- Enhanced raw touch sampling with 32 samples, discarding 6 highest/lowest outliers
- Touch calibration now uses stable reading collection (10 consecutive readings within 30px)
- Stable calibration now stores and uses proper matrix values
- Debug logs enhanced to track matrix computation and touch mapping behavior

### Added

- Jump detection prevents impossible touch movements (>300px within 100ms)
- Touch test visualization shows spike detection with red indicators
- `waitForStableTouch()` method for more accurate calibration point collection

### Refactored

- Calibration input pipeline cleaned up to reflect correct axis orientation
- Calibration routine now uses actual on-screen target positions during setCalibration

---

## [0.0.1-alpha] - 2025-06-01

### Added

- Initial implementation of UI, sensor modules, and state management
- Touch input with raw value reading
- Preliminary calibration logic and persistent preferences
- LVGL integration and screen rendering system
