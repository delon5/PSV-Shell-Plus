# PSV Shell Plus

This is a fork of PSVshell, called PSVShell+. It adds a number of extra features,  mostly for PS TV owners
(like myself), but others can find it useful too.

## Extra features

- Swap the cross and circle buttons for playing japanese games. The swap covers everything the game reads,
  including the system dialogs it opens (save data, message and selection dialogs); the icons those dialogs show
  still follow the system's enter-button setting
- Disable **L3** and **R3** (the stick buttons) to prevent you from accidentally pressing them
- DS4 touchpad emulation. This has three modes:
  - **front:** Use the touchpad to emulate the front touchscreen
  - **F/B:** Emulates both the front and the back touchpad, using the touchpad button to toggle between them
  - **F/B/X:** Same as "F/B", but it toggles between front, back and off, not just front and back
- DS3/DS4 motion emulation. This has two modes:
  - **normal:** The controller has the same orientation as the Vita
  - **vertical:** The controller held horizontally has the orientation of the Vita held vertically

  The Bluetooth capture follows the [ds34motion](https://github.com/MERLev/ds34motion) design: the controller is
  bound when it connects (or on its first report, if it was already connected when the plugin started), reports
  are read when the transfer completes and the binding is dropped on disconnect. The DualShock 3 only has an
  accelerometer and a yaw gyro, so the other two rotation axes stay still; the DS3 mapping is ported from
  ds34motion and has not been tested on hardware in this port. On a real PS Vita the controller replaces the
  internal sensors while motion emulation is enabled in the profile. Enable it before starting the game: the
  synthetic sensor calibration is handed to SceMotion when it initialises, so toggling the setting mid-game
  leaves motion disabled until the game is restarted (untested on a real PS Vita).
- Reboot or shutdown the Vita. This just seemed nice to have on hand.
- Adrenaline may ask for a 500 MHz CPU clock: a plugin running inside the PSP emulator (such as psp_bridge with
  `clock_cpu=500` in `ge_patch.ini`) can request more than 444 MHz, and PSVShell+ applies 500 MHz the same way its
  menu does. Other applications are limited to 444 MHz by the system as before.

In the "FULL" mode, there are now two pages, that you can switch between using **L** and **R**. The main page is the
same as it was in PSVshell. All the extra features added by PSVShell+ are on the second page.

Saving the profile on the main page (below the clock speeds) also save the extra features in the same manner.

## Installation

1. Copy `PSVshellPlus.skprx` to `ur0:tai/`.
2. Add it to `ur0:tai/config.txt` under the `*KERNEL` section (and remove any older `psvshell+.skprx` line):

   ```
   *KERNEL
   ur0:tai/PSVshellPlus.skprx
   ```

3. Reboot. Press **SELECT + UP** to open the menu; the extra features are on the second page (**L**/**R**).

If you used DSMotion, ds34motion or ds4Touch for your DualShock, remove them: PSVShell+ replaces both once the
"Bt Touch" and "Bt Motion" options are enabled in the profile. On a real PS Vita you still need ds34vita (or
ds4vita) to connect the controller in the first place.

## Acknowledgments

- To [Electry](https://github.com/Electry) for the original [PSVshell](https://github.com/Electry/PSVshell)
- To [MERLev](https://github.com/MERLev) for [ds4Touch](https://github.com/MERLev/ds4Touch)
- To [OperationNT414C](https://github.com/OperationNT414C) for [DSMotion](https://github.com/OperationNT414C/DSMotion)
- To [MERLev](https://github.com/MERLev) for [ds34motion](https://github.com/MERLev/ds34motion), the successor of DSMotion
- And a big thank you to [VitaSDK](https://github.com/vitasdk) for making this all possible.

# Original readme
Yet another overclocking plugin

## Features:
- Allows you to change CPU, GPU, BUS and XBAR clocks separately, in these steps:
  - **CPU:** 41, 83, 111, 166, 222, 333, 444, 500 MHz
  - **GPU (ES4):** 41, 55, 83, 111, 166, 222 MHz
  - **BUS:** 55, 83, 111, 166, 222 MHz
  - **XBAR:** 83, 111, 166 MHz
- Supports per-app profiles
- Shows per-core CPU usage in %, including peak single-thread load
- Runs in kernelland (=> visible in LiveArea)
- Pretty GUI with some useless eye-candy metrics such as ram/vram usage, battery temp, etc...
- Does not slow down games when menu is open
- Does not crash Adrenaline
- Clean code and patches
- **3.60** and **3.65** FWs are supported

## How to use:
- Press **SELECT + UP** or **SELECT + DOWN** to toggle between 3 GUI modes

#### When in 'FULL' mode:
- Use **UP/DOWN** to move in the menu
- Press **X** to toggle frequency mode for currently selected **> device <**:
  - **Default freq.** (WHITE) - the plugin will not interfere, but rather use the default freq. for current game
  - **Manual freq.** (BLUE) - the plugin will use your specified freq.
    - press **LEFT/RIGHT** to immediately change the frequency
- Press **X** when **> save profile <** is selected to save/delete profiles
  - All **Manual freq.** (BLUE) will be loaded and applied next time you start/resume the game
  - All **Default freq.** (WHITE) will be kept to default (set to whatever freq. the game asks for)
- Press and hold **LEFT TRIGGER** and **> save profile <** will change to **> save global <**
  - Press **X** when **> save global <** is selected and the options will be saved to *global* (default) profile
  - *Global* profile will be used as default profile when game-specific profile doesn't exist

## Screenshots:
![2019-12-21-181613](https://user-images.githubusercontent.com/12598379/71311342-c15df300-241e-11ea-8baf-c67ec2bcbbd7.png)

### 'FPS only' mode
![2019-11-01-194930](https://user-images.githubusercontent.com/12598379/68051962-e10f4f80-fce8-11e9-92d0-9662cc6f0d04.png)

### 'HUD' mode
![2019-12-21-181809](https://user-images.githubusercontent.com/12598379/71311344-c1f68980-241e-11ea-9ca1-4207d4887002.png)

### 'FULL' mode
![2019-12-21-181801](https://user-images.githubusercontent.com/12598379/71311343-c1f68980-241e-11ea-8249-5f2e0c44d642.png)

## Credits:
- [Yifan Lu](https://github.com/yifanlu) - for [ScePervasive](https://wiki.henkaku.xyz/vita/Pervasive) RE
- [dots-tb](https://github.com/dots-tb) - for ksceKernelInvokeProcEventHandler() hook
- [Rinnegatamante](https://github.com/Rinnegatamante) - for orig. framecounter impl.
