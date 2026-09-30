# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to "Ad Hoc" versioning (meaning, I assign versions based on how I like it).

## [1.4] - 2026-09-30
### Added
- Adrenaline may ask for a 500 MHz CPU clock. A plugin running inside the PSP emulator (psp_bridge) can request
  more than 444 MHz through scePowerSetArmClockFrequency; the request is honoured the way the menu sets 500 MHz
  (444 MHz through ScePower, then the 500 MHz clock select), provided ScePower's stored clock reads back as 444.
  Requests from every other application are passed to ScePower unchanged, and Adrenaline stays out of the profiles
  and the menu.
### Changed
- The 500 MHz clock select is applied only when its low-level clock function was found at boot and the multiplier
  check inside it was disabled (or another module had already disabled it). Otherwise a 500 MHz request, whether
  from the menu, a profile or Adrenaline, stays at the 444 MHz ScePower sets and the clock getters keep reporting
  444 MHz, instead of a call through a missing function or a reported 500 MHz the hardware never took.

## [1.3] - 2026-09-26
### Fixed
- The X/O swap and the L3/R3 disable now also apply to input read through the "Ext" controller functions
  (sceCtrlPeek/ReadBufferPositiveExt and Ext2). The system dialogs shown inside games (save data, message and
  selection dialogs) and games with PS TV controller support read the pad that way, so the swap did not reach
  them before. The button icons those dialogs display still follow the system's enter-button setting.

## [1.2] - 2026-09-19
### Added
- DualShock 3 motion emulation (accelerometer and yaw gyro), mapping ported from ds34motion.

### Changed
- The plugin file is now named PSVshellPlus.skprx (was psvshell+.skprx); update the entry in ur0:tai/config.txt.
- Bluetooth capture upgraded from the DSMotion design to the ds34motion 1.3.1 design: the controller is bound on
  its connection event (or lazily on its first report if it was already connected), input reports are read when
  the transfer completes instead of when it is queued (one report of latency less, no stale data), a read that
  delivers an unrecognised report keeps the buffer pending, the binding is dropped on the disconnect event, and
  the pending request is protected by a mutex. Should read events never surface, the plugin falls back to the old
  transfer-time parsing after a few reports.
- A DS4 report is now only accepted when it carries the full report id (0x11), as ds34motion and ds34vita do.
- The controller binding is only kept alive by consumed reports, so a device the DS3 heuristic misidentifies frees
  the binding after five seconds; a DS4 misidentified as a DS3 corrects itself on its first full report.

### Fixed
- Motion emulation on a real PS Vita: the synthetic sensor calibration is now returned whenever motion emulation is
  enabled, instead of only when no motion device exists, so the injected samples are decoded correctly.
- On a real PS Vita motion samples are only injected while a controller is bound and its data is fresh; a
  disconnected controller no longer keeps feeding its last sample. On a PS TV the latest captured sample keeps
  being fed, as before.
- The SceMotionDev device info word is now actually zeroed instead of an uninitialised value being returned.
- The SceTouch hooks no longer interpret a failed read as a buffer count.
- Touch emulation now judges data freshness by the time of the last report instead of the last transfer.
- Walking a HID request chain is bounded, so a self-linked request (as ds34vita queues) cannot hang the hook.

## [1.1] - 2022-04-03
### Added
- FPS Limiter, that's active when only the FPS is visible.
- Page indicator dots to the top of the widget in full mode.
- Crosshair overlay (basic and cross), shown in the FPS only mode.
- Changelog

### Changed
- Updated code for new VitaSDK version (2022-03-10).
- The FPS counter is now red, when the FPS limiter is enabled.

## [1.0] - 2021-08-11
### Added
- Paging to the widget in full mode, with the L and R buttons.
- A second page for additional settings.
- Option to swap the X and O buttons.
- Option to disable the L3 and R3 buttons.
- Support for touch controls on DS4 controllers via Bluetooth.
- Support for motion controls on DS4 controllers via Bluetooth.

### Changed
- Updated the README.md with the additional features

[1.4]: https://github.com/delon5/PSV-Shell-Plus/compare/1.3...1.4
[1.3]: https://github.com/delon5/PSV-Shell-Plus/compare/1.2...1.3
[1.2]: https://github.com/delon5/PSV-Shell-Plus/compare/1.1...1.2
[1.1]: https://github.com/delon5/PSV-Shell-Plus/compare/1.0...1.1
[1.0]: https://github.com/delon5/PSV-Shell-Plus/releases/tag/1.0