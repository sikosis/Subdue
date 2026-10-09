# Subdue for Windows

![Windows 10 and 11](https://img.shields.io/badge/Windows-10%20%7C%2011-0078D4?logo=windows11&logoColor=white)
![Version 0.12](https://img.shields.io/badge/version-0.12-4c8bf5)
![MIT License](https://img.shields.io/badge/license-MIT-green)

Subdue for Windows is the native C++/Win32 equivalent of the macOS utility. It
can schedule Subdue to close an application or Smother to mute the default
audio output. Once armed, its window shows the confirmation for five seconds,
then hides while the notification-area icon
tracks the action and live countdown in its tooltip and context menu.

## Requirements

- Windows 10 or Windows 11, x64
- Visual Studio 2022 with **Desktop development with C++**

## Build and package

From a regular Command Prompt, an x64 Native Tools Command Prompt, or by
double-clicking the file:

```bat
cd windows
build-windows.bat
```

Pass `Debug` if needed; `Release` is the default:

```bat
build-windows.bat Debug
```

The batch file locates Visual Studio automatically and calls `rc.exe` and
`cl.exe` directly. No additional build shell or generator is required. The
executable and SHA-256-labelled ZIP are written under `windows\dist`. The
package contains `Subdue.exe`, this guide, and the MIT license.

The current package is named `Subdue-0.12-windows-x64.zip`. Version 0.12
includes both the Subdue application scheduler and the Smother audio scheduler;
if the GUI has no **Action** selector, an older executable is being run.

For a regular Visual Studio solution:

```bat
cmake -S . -B build -A x64
cmake --build build --config Release
```

## Command line

```bat
Subdue.exe --app notepad --after 30
Subdue.exe --app "Google Chrome" --at 23:15
Subdue.exe --app vlc --top-of-hour
Subdue.exe --mute --after 45
Subdue.exe --mute --at 23:30
```

Options:

- `--app NAME`: executable name, display name, or full executable path
- `--mute`: use Smother to mute the current default audio output
- `--after MINUTES`: run the action after a positive number of minutes
- `--at HH:mm`: run at the next occurrence of a local 24-hour time
- `--top-of-hour` or `--top`: run at the start of the next hour
- `--force`: force-close after ten seconds; enabled by default
- `--no-force`: request a graceful close only
- `--foreground`: keep the window visible after command-line scheduling
- `--help`: show command-line help

Subdue first posts `WM_CLOSE` to the target's top-level windows. If the process
is still alive after ten seconds and force-close is enabled, it calls
`TerminateProcess`. Windows security boundaries still apply: an unelevated
Subdue process cannot terminate an elevated target.

Smother uses the Windows Core Audio endpoint-volume API. It mutes whichever
playback device is the default when the schedule fires. A failed mute now
reports its Windows `HRESULT` code in the completion notification to make audio
device or permission problems easier to diagnose.

## License

Subdue is released under the [MIT License](../LICENSE).

Copyright (c) 2026 Sikosis.
