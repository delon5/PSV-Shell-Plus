# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to "Ad Hoc" versioning (meaning, I assign versions based on how I like it).

## [Unreleased]
### Added
- FPS Limiter, that's active when only the FPS is visible.
- Page indicator dots to the top of the widget in full mode.
- Changelog
- DualShock 3 motion emulation (accelerometer and yaw gyro), mapping ported from ds34motion.

### Changed
- Updated code for new VitaSDK version (2022-03-10).
- The FPS counter is now red, when the FPS limiter is enabled.
- Bluetooth capture upgraded from the DSMotion design to the ds34motion 1.3.1 design: the controller is bound on
  its connection event, input reports are read when the transfer completes instead of when it is queued (one report
  of latency less, no stale data), the binding is dropped on the disconnect event, and the pending request is
  protected by a mutex.

### Fixed
- Motion emulation on a real PS Vita: the synthetic sensor calibration is now returned whenever motion emulation is
  enabled, instead of only when no motion device exists, so the injected samples are decoded correctly.
- Motion samples are only injected while a controller is bound and its data is fresh; a disconnected controller no
  longer keeps feeding its last sample.
- The SceMotionDev device info word is now actually zeroed instead of an uninitialised value being returned.
- Touch and motion hooks no longer interpret a failed SceTouch read as a buffer count.
- Walking a HID request chain is bounded, so a self-linked request (as ds34vita queues) cannot hang the hook.

## [1.0] - 2021-08-11
### Added
- Paging to the widget in full mode, with the L and R buttons.
- A second page for additional settings.
- Option to swap the X and O buttons.
- Option to disable the L3 and L3 buttons.
- Support for touch controls on DS4 controllers via Bluetooth.
- Support for motion controls on DS4 controllers via Bluetooth.

### Changed
- Updated the README.md with the additional features

[Unreleased]: https://onedev.server.local/projects/2/blob
[1.0]: https://onedev.server.local/projects/2/blob/1.0