# Contributing to Subdue

Thanks for helping improve Subdue.

## Before opening an issue

- Check whether the problem is specific to macOS, Windows, or Haiku.
- Reproduce it with the latest source when practical.
- Include the operating-system version and architecture.
- For scheduling problems, include the selected action and timing mode.
- Include compiler or Terminal output as text, not only as a screenshot.

## Building changes

- macOS: build the `Subdue` scheme from `Subdue.xcodeproj`.
- Windows: run `windows/build-windows.bat`, or use the CMake project.
- Haiku: run `make` and `make test` inside `haiku/`.

Keep platform implementations native. Shared behavior should remain consistent,
but each edition should use the conventions and APIs of its own operating
system.

Do not commit generated build trees, installers, ZIP files, packages, or local
IDE state. Attach release artifacts to a GitHub Release instead.

## Pull requests

- Keep each pull request focused on one change.
- Explain which platforms are affected.
- Describe how the change was tested.
- Update the appropriate README when behavior or build steps change.

By submitting a contribution, you agree that it may be distributed under the
project's [MIT License](LICENSE).
