# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.0.2-alpha] - 2025-06-06

### Fixed

- Touchscreen edge glitch caused by float underflow during coordinate mapping
- Stable calibration now stores and uses proper matrix values
- Debug logs enhanced to track matrix computation and touch mapping behavior
- Raw touch axis swap issue that caused incorrect calibration results.
- Touch mapping instability during diagonal gestures and edge presses.

### Refactored

- Calibration input pipeline cleaned up to reflect correct axis orientation.

### Changed

- `TouchScreen::mapRawToScreen()` now clamps float values *before* casting to `uint16_t`

---

## [0.0.1-alpha] - 2025-06-01

### Added

- Initial implementation of UI, sensor modules, and state management
- Touch input with raw value reading
- Preliminary calibration logic and persistent preferences
- LVGL integration and screen rendering system
