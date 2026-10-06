#!/bin/bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
brew_bin=/opt/homebrew/bin/brew
if [[ ! -x "$brew_bin" ]]; then
  echo "Install Homebrew from https://brew.sh first, then rerun this script."
  exit 1
fi
export PATH="/opt/homebrew/bin:/opt/homebrew/sbin:$PATH"
"$brew_bin" install cmake ninja pkgconf qtbase qtmultimedia qt5compat qttools qttranslations   ffmpeg@6 libarchive libdeflate simdutf simdjson libsamplerate libsoxr flac   libcue uchardet opencc mecab mecab-ipadic
cd "$project_dir"
fetch_repo() {
  local destination="$1" repository="$2" revision="$3"
  if [[ ! -d "$destination" ]]; then
    git clone --depth 1 --branch "$revision" --recurse-submodules --shallow-submodules "$repository" "$destination"
  fi
}
fetch_repo src/thirdparty/qcoro https://github.com/danvratil/qcoro.git v0.12.0
fetch_repo src/thirdparty/qwindowkit https://github.com/stdware/qwindowkit.git 1.4.0
fetch_repo src/thirdparty/glaze https://github.com/stephenberry/glaze.git v5.5.2
fetch_repo out/deps/taglib https://github.com/taglib/taglib.git v2.1.1
fetch_repo out/deps/spdlog https://github.com/gabime/spdlog.git v1.15.3
fetch_bass() {
  local name="$1" url="$2" library="$3"
  local destination="out/deps/bass-macos/$name"
  if [[ ! -f "$destination/$library" ]]; then
    mkdir -p "$destination"
    curl --fail --location "$url" -o "$destination/archive.zip"
    unzip -o -q "$destination/archive.zip" -d "$destination"
  fi
}
fetch_bass bass24-osx https://www.un4seen.com/files/bass24-osx.zip libbass.dylib
fetch_bass bassflac24-osx https://www.un4seen.com/files/bassflac24-osx.zip libbassflac.dylib
fetch_bass bassmix24-osx https://www.un4seen.com/files/bassmix24-osx.zip libbassmix.dylib
fetch_bass bassdsd24-osx https://www.un4seen.com/files/bassdsd24-osx.zip libbassdsd.dylib
fetch_bass bass_fx-arm64 https://www.un4seen.com/stuff/bass_fx-osx.zip libbass_fx.dylib
export DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer
export PKG_CONFIG_PATH="/opt/homebrew/opt/ffmpeg@6/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
cmake --preset macos-xcode
cmake --build --preset macos-xcode --parallel 4
