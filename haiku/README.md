# Subdue for Haiku

![Platform: Haiku](https://img.shields.io/badge/platform-Haiku-ffcc00?logo=haiku&logoColor=000000)
![Language: C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![Version: 0.2.1](https://img.shields.io/badge/version-0.2.1-blue)
![Status: development](https://img.shields.io/badge/status-development-orange)

Subdue is a small native Haiku utility that performs an action later. It can
close a running application (**Subdue**) or mute the system audio mixer
(**Smother**) after a number of minutes, at a specific local time, or at the
top of the next hour.

After a schedule is created, the window stays visible for five seconds and
then hides. The Deskbar item continues to show the live countdown.

## Features

- Native C++ application using the Haiku Application, Interface, and Media Kits.
- Running-application picker populated through `BRoster`.
- Schedule after N minutes, at `HH:mm`, or at the top of the next hour.
- Graceful application quit using `B_QUIT_REQUESTED`.
- Force-close after ten seconds when enabled.
- System audio mute through Haiku's master mixer control.
- Persistent Deskbar item with a live countdown.
- Deskbar commands to show Subdue, cancel the schedule, or quit.
- Schedule recovery when Subdue is restarted.
- Haiku notifications confirming success or failure.
- Command-line operation for scripts and Terminal use.

## Requirements

- Haiku with the standard development tools installed.
- Working Media Services and a mixer exposing a master mute control for
  Smother.

No third-party libraries are required.

## Build

Open Terminal in this directory and run:

```sh
make
```

This creates the `Subdue` executable. Clean generated files with:

```sh
make clean
```

Run the option-parser tests with:

```sh
make test
```

To install a non-packaged copy under your user configuration:

```sh
make install
```

Remove that copy with `make uninstall`.

## Use the graphical interface

Start Subdue:

```sh
./Subdue
```

1. Choose **Subdue an application** or **Smother system audio**.
2. For Subdue, choose a running application.
3. Choose **After**, **At time**, or **Top of hour**.
4. Leave force-close enabled if Subdue should terminate an application that
   ignores the graceful quit request.
5. Click **Schedule Subdue** or **Schedule Smother**.

The confirmation remains visible for five seconds. The main window then hides
and the Deskbar item displays the countdown.

The active schedule is saved in your Haiku settings directory. If Subdue is
restarted, it restores a Smother schedule automatically. An application-close
schedule is restored when the target application is still running; otherwise
the stale schedule is discarded. Completed and failed actions produce a Haiku
notification.

### Deskbar controls

- Left-click the Deskbar item to show the main window.
- Right-click it to show Subdue, cancel the active schedule, or quit Subdue.
  Quitting removes the Subdue item from Deskbar.
- When Subdue is not running, left-clicking the item launches it.

The Deskbar item is installed automatically the first time Subdue runs. Use
Deskbar's replicant controls if you want to remove it. After rebuilding or
replacing Subdue, remove and re-add the item if Deskbar still has the older
replicant loaded.

## Command line

Close an application after 30 minutes:

```sh
./Subdue --app StyledEdit --after 30
```

Close WebPositive at 23:15:

```sh
./Subdue --app application/x-vnd.Haiku-WebPositive --at 23:15
```

Mute system audio at the next hour:

```sh
./Subdue --mute --top-of-hour
```

Available options:

```text
--app NAME       Running application name or signature
--mute           Smother the Haiku system audio mixer
--after MINUTES  Run after a positive number of minutes
--at HH:mm       Run at the next local occurrence of the time
--top-of-hour    Run at the start of the next hour
--force          Force-close after 10 seconds (default)
--no-force       Only request a graceful quit
--foreground     Keep the main window visible
--help           Print command-line help
```

## Project layout

```text
src/             Native C++ source
tests/           Command-line parser tests
Subdue.rdef      Haiku application metadata
Makefile         Build instructions
```

## Current status

The core Subdue and Smother actions have been tested successfully on Haiku.
The Deskbar component, restart recovery, and notifications should be tested in
the current 0.2.1 milestone. BeOS-style raster icon artwork is available under
`artwork/`; conversion to an embedded HVIF resource and HPKG/HaikuPorts
packaging are still to come.

## Contributing

Issues and pull requests are welcome. When reporting a problem, include your
Haiku version, architecture, the selected action, and any Terminal output.

## License

Subdue is released under the [MIT License](../LICENSE).

Copyright (c) 2026 Sikosis.
