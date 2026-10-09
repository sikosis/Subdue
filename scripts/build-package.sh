#!/bin/bash
set -euo pipefail

project_root="$(cd "$(dirname "$0")/.." && pwd)"
build_root="$project_root/build/package"
dist_dir="$project_root/dist"
project_file="$project_root/Subdue.xcodeproj/project.pbxproj"

version="$(grep -o 'MARKETING_VERSION = [^;]*' "$project_file" | head -1 | cut -d= -f2 | xargs)"
if [[ -z "$version" ]]; then
  echo "Could not determine MARKETING_VERSION" >&2
  exit 1
fi

rm -rf "$build_root"
mkdir -p "$build_root/root/Applications" "$dist_dir"

xcodebuild \
  -project "$project_root/Subdue.xcodeproj" \
  -scheme Subdue \
  -configuration Release \
  -derivedDataPath "$build_root/DerivedData" \
  ARCHS="arm64 x86_64" \
  ONLY_ACTIVE_ARCH=NO \
  CODE_SIGNING_ALLOWED=NO \
  clean build

app="$build_root/DerivedData/Build/Products/Release/Subdue.app"
actual_version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$app/Contents/Info.plist")"
if [[ "$actual_version" != "$version" ]]; then
  echo "Built app version $actual_version does not match project version $version" >&2
  exit 1
fi

cp -R "$app" "$build_root/root/Applications/Subdue.app"
package="$dist_dir/Subdue-$version.pkg"
rm -f "$package" "$package.sha256"
pkgbuild \
  --root "$build_root/root" \
  --identifier "com.sikosis.Subdue.pkg" \
  --version "$version" \
  --install-location / \
  "$package"

shasum -a 256 "$package" > "$package.sha256"
echo "Created $package"
cat "$package.sha256"
