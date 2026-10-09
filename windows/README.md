# Subdue for Windows

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
`cl.exe` directly. No additional build shell or generator is required. The executable and
SHA-256-labelled ZIP are written under `windows\dist`.

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
playback device is the default when the schedule fires.
