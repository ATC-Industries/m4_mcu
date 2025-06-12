# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

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
