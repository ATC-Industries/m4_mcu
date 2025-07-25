# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.0.4-alpha] - v0.0.4-alpha

### Added

- Pull history table with clear routine
- Pull data now saved to EEPROM (up to 255 pulls; space limitations TBD)
- Screen rotation toggle in settings

### Changed

- Improved speed calculator to reduce spikes/noise
- Driver number now auto-increments on pull save

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
