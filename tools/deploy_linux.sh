#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PRESET="${1:-wsl-linux-release}"
MODE="${2:-appdir}"
BUILD_DIR="${XAMP_DEPLOY_BUILD_DIR:-${ROOT_DIR}/out/build/${PRESET}}"
SKIP_BUILD="${XAMP_DEPLOY_SKIP_BUILD:-0}"
APP_DIR="${BUILD_DIR}/src/xamp"
APP_BIN="${APP_DIR}/xamp"
DEPLOY_ROOT="${ROOT_DIR}/out/deploy/linux-x64"
APPDIR="${DEPLOY_ROOT}/XAMP.AppDir"
ZIP_FILE="${ROOT_DIR}/out/deploy/linux-x64.zip"
APPDIR_BIN_DIR="${APPDIR}/usr/bin"
APPDIR_LIB_DIR="${APPDIR}/usr/lib"
APPDIR_DESKTOP="${APPDIR}/usr/share/applications/xamp.desktop"
APPDIR_ICON="${APPDIR}/usr/share/icons/hicolor/256x256/apps/xamp.png"
APPDIR_PIXMAP_ICON="${APPDIR}/usr/share/pixmaps/xamp.png"
DEPLOY_LOG_FILE="${ROOT_DIR}/out/deploy/deploy_linux.log"
QT_PLUGIN_DIRS=(platforms imageformats sqldrivers tls iconengines)

log() {
    printf '[deploy] %s\n' "$*"
}

die() {
    printf '%s\n' "$*" >&2
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || die "Missing required command: $1"
}

step_start() {
    XAMP_STEP_NAME="$1"
    XAMP_STEP_TIME="$(date +%s)"
    log "Start ${XAMP_STEP_NAME}"
}

step_done() {
    local now elapsed
    now="$(date +%s)"
    elapsed=$((now - XAMP_STEP_TIME))
    log "Done ${XAMP_STEP_NAME} elapsed:${elapsed}s"
}

init_log_file() {
    mkdir -p "$(dirname "${DEPLOY_LOG_FILE}")"
    {
        printf '[deploy] ===== XAMP Linux deploy start =====\n'
        printf '[deploy] time: %s\n' "$(date -Is)"
        printf '[deploy] preset: %s\n' "${PRESET}"
        printf '[deploy] mode: %s\n' "${MODE}"
        printf '[deploy] build dir: %s\n' "${BUILD_DIR}"
        printf '[deploy] skip build: %s\n' "${SKIP_BUILD}"
    } > "${DEPLOY_LOG_FILE}"
    exec > >(tee -a "${DEPLOY_LOG_FILE}") 2>&1
}

copy_if_exists() {
    local src="$1"
    local dst="$2"
    if [[ -e "${src}" ]]; then
        mkdir -p "$(dirname "${dst}")"
        cp -a "${src}" "${dst}"
    fi
}

copy_dir_if_exists() {
    local src="$1"
    local dst="$2"
    if [[ -d "${src}" ]]; then
        rm -rf "${dst}"
        mkdir -p "$(dirname "${dst}")"
        cp -a "${src}" "${dst}"
    fi
}

copy_glob_to_dir() {
    local dst="$1"
    shift
    mkdir -p "${dst}"
    for pattern in "$@"; do
        for src in ${pattern}; do
            [[ -e "${src}" ]] || continue
            cp -a "${src}" "${dst}/"
        done
    done
}

copy_ldconfig_libraries_to_dir() {
    local dst="$1"
    shift
    mkdir -p "${dst}"

    for name in "$@"; do
        while IFS= read -r src; do
            [[ -e "${src}" ]] || continue
            cp -aL "${src}" "${dst}/$(basename "${src}")"
        done < <(
            ldconfig -p 2>/dev/null | awk -v name="lib${name}.so" '
                $1 == name || index($1, name ".") == 1 { print $NF }
            ' | sort -u
        )
    done
}

library_paths_for() {
    local binary="$1"
    ldd "${binary}" 2>/dev/null | awk '
        $2 == "=>" && $3 ~ /^\// { print $3 }
        $1 ~ /^\// { print $1 }
    '
}

should_skip_system_library() {
    local name
    name="$(basename "$1")"
    case "${name}" in
        ld-linux*.so*|linux-vdso.so*|libanl.so.*|libBrokenLocale.so.*|libc.so.*|libcidn.so.*|libdl.so.*|libm.so.*|libmvec.so.*|libnsl.so.*|libnss_*.so.*|libpthread.so.*|libresolv.so.*|librt.so.*|libthread_db.so.*|libutil.so.*)
            return 0
            ;;
    esac
    return 1
}

copy_library_closure() {
    local -a queue=("$@")
    declare -A seen=()

    while ((${#queue[@]} > 0)); do
        local binary="${queue[0]}"
        queue=("${queue[@]:1}")
        [[ -e "${binary}" ]] || continue
        [[ -n "${seen[${binary}]:-}" ]] && continue
        seen["${binary}"]=1

        while IFS= read -r lib; do
            [[ -e "${lib}" ]] || continue
            if should_skip_system_library "${lib}"; then
                continue
            fi

            local dst="${APPDIR_LIB_DIR}/$(basename "${lib}")"
            if [[ ! -e "${dst}" ]]; then
                cp -aL "${lib}" "${dst}"
            fi
            queue+=("${lib}")
        done < <(library_paths_for "${binary}")
    done
}

copy_qt_plugins() {
    local qt_plugin_root
    qt_plugin_root="$(qmake6 -query QT_INSTALL_PLUGINS)"
    [[ -d "${qt_plugin_root}" ]] || die "Cannot find Qt plugin directory: ${qt_plugin_root}"

    for dir in "${QT_PLUGIN_DIRS[@]}"; do
        copy_dir_if_exists "${qt_plugin_root}/${dir}" "${APPDIR}/usr/plugins/${dir}"
    done

    mapfile -t plugin_files < <(find "${APPDIR}/usr/plugins" -type f -name '*.so' 2>/dev/null | sort)
    if ((${#plugin_files[@]} > 0)); then
        copy_library_closure "${plugin_files[@]}"
    fi
}

find_linuxdeployqt() {
    if [[ -n "${LINUXDEPLOYQT:-}" && -x "${LINUXDEPLOYQT}" ]]; then
        printf '%s\n' "${LINUXDEPLOYQT}"
        return 0
    fi

    for name in linuxdeployqt linuxdeployqt-continuous-x86_64.AppImage linuxdeployqt-x86_64.AppImage; do
        if command -v "${name}" >/dev/null 2>&1; then
            command -v "${name}"
            return 0
        fi
    done

    for path in \
        "${ROOT_DIR}/tools/linuxdeployqt" \
        "${ROOT_DIR}/tools/linuxdeployqt-continuous-x86_64.AppImage" \
        "${ROOT_DIR}/tools/linuxdeployqt-x86_64.AppImage" \
        "${ROOT_DIR}/out/tools/linuxdeployqt-continuous-x86_64.AppImage" \
        "${ROOT_DIR}/out/tools/linuxdeployqt-x86_64.AppImage"; do
        if [[ -x "${path}" ]]; then
            printf '%s\n' "${path}"
            return 0
        fi
    done

    return 1
}

download_linuxdeployqt() {
    local dst="${ROOT_DIR}/out/tools/linuxdeployqt-continuous-x86_64.AppImage"
    local url="https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"

    mkdir -p "$(dirname "${dst}")"

    if command -v curl >/dev/null 2>&1; then
        curl -L --fail --retry 3 -o "${dst}" "${url}"
    elif command -v wget >/dev/null 2>&1; then
        wget -O "${dst}" "${url}"
    else
        die "Missing curl or wget for downloading linuxdeployqt."
    fi

    chmod +x "${dst}"
    printf '%s\n' "${dst}"
}

write_desktop_file() {
    mkdir -p "$(dirname "${APPDIR_DESKTOP}")"
    cat > "${APPDIR_DESKTOP}" <<'EOF'
[Desktop Entry]
Type=Application
Name=XAMP
Comment=Music player
Exec=xamp
Icon=xamp
Categories=Audio;AudioVideo;Player;Qt;
StartupWMClass=xamp
Terminal=false
EOF
}

write_apprun() {
    cat > "${APPDIR}/AppRun" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
APPDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${APPDIR}/usr/bin/xamp" "$@"
EOF
    chmod +x "${APPDIR}/AppRun"
}

write_launcher() {
    cat > "${APPDIR_BIN_DIR}/xamp" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
APP_BIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APPDIR="$(cd "${APP_BIN_DIR}/../.." && pwd)"
export LD_LIBRARY_PATH="${APPDIR}/usr/lib:${APP_BIN_DIR}/components:${LD_LIBRARY_PATH:-}"
export QT_PLUGIN_PATH="${APPDIR}/usr/plugins:${APPDIR}/usr/lib/qt6/plugins:${QT_PLUGIN_PATH:-}"
export QT_QPA_PLATFORM_PLUGIN_PATH="${APPDIR}/usr/plugins/platforms:${APPDIR}/usr/lib/qt6/plugins/platforms:${QT_QPA_PLATFORM_PLUGIN_PATH:-}"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
cd "${APP_BIN_DIR}"
exec "${APP_BIN_DIR}/xamp.bin" "$@"
EOF
    chmod +x "${APPDIR_BIN_DIR}/xamp" "${APPDIR_BIN_DIR}/xamp.bin"
}

copy_xamp_runtime_data() {
    for file in config.json fonticon.json en_US.qm ja_JP.qm zh_TW.qm xamp.db xamp.ini; do
        copy_if_exists "${APP_DIR}/${file}" "${APPDIR_BIN_DIR}/${file}"
        copy_if_exists "${ROOT_DIR}/src/xamp/${file}" "${APPDIR_BIN_DIR}/${file}"
    done

    for dir in components fonts migrations opencc eqpresets mecab; do
        copy_dir_if_exists "${APP_DIR}/${dir}" "${APPDIR_BIN_DIR}/${dir}"
        if [[ ! -d "${APPDIR_BIN_DIR}/${dir}" ]]; then
            copy_dir_if_exists "${ROOT_DIR}/src/xamp/${dir}" "${APPDIR_BIN_DIR}/${dir}"
        fi
    done

    copy_glob_to_dir "${APPDIR_BIN_DIR}/components" \
        "${BUILD_DIR}/vcpkg_installed/"*/lib/intel64/libiomp*.so* \
        "${BUILD_DIR}/vcpkg_installed/"*/lib/intel64/libmkl*.so* \
        "${BUILD_DIR}/vcpkg_installed/"*/lib/libavcodec.so* \
        "${BUILD_DIR}/vcpkg_installed/"*/lib/libavformat.so* \
        "${BUILD_DIR}/vcpkg_installed/"*/lib/libavutil.so* \
        "${BUILD_DIR}/vcpkg_installed/"*/lib/libcue.so* \
        "${BUILD_DIR}/vcpkg_installed/"*/lib/libmecab.so* \
        "${BUILD_DIR}/vcpkg_installed/"*/lib/libopencc.so* \
        "${BUILD_DIR}/vcpkg_installed/"*/lib/libsamplerate.so* \
        "${BUILD_DIR}/vcpkg_installed/"*/lib/libsoxr.so* \
        "${BUILD_DIR}/vcpkg_installed/"*/lib/libswresample.so* \
        "${BUILD_DIR}/vcpkg_installed/"*/lib/libswscale.so* \
        "${BUILD_DIR}/vcpkg_installed/"*/lib/libuchardet.so*

    copy_ldconfig_libraries_to_dir "${APPDIR_BIN_DIR}/components" \
        avcodec \
        avformat \
        avutil \
        cue \
        mecab \
        opencc \
        samplerate \
        soxr \
        swresample \
        swscale \
        uchardet
}

copy_project_libraries() {
    copy_glob_to_dir "${APPDIR_LIB_DIR}" \
        "${BUILD_DIR}/src/xamp_base/libxamp_base.so*" \
        "${BUILD_DIR}/src/xamp_metadata/libxamp_metadata.so*" \
        "${BUILD_DIR}/src/xamp_output_device/libxamp_output_device.so*" \
        "${BUILD_DIR}/src/xamp_player/libxamp_player.so*" \
        "${BUILD_DIR}/src/xamp_stream/libxamp_stream.so*" \
        "${BUILD_DIR}/src/widget_shared/libwidget_shared.so*" \
        "${BUILD_DIR}/out-x86_64-Release/lib/libQWKCore.so*" \
        "${BUILD_DIR}/out-x86_64-Release/lib/libQWKWidgets.so*" \
        "${BUILD_DIR}/out-x86_64-Debug/lib/libQWKCore.so*" \
        "${BUILD_DIR}/out-x86_64-Debug/lib/libQWKWidgets.so*"
}

copy_runtime_libraries() {
    local -a binaries=("${APPDIR_BIN_DIR}/xamp.bin")

    mapfile -t app_libraries < <(find "${APPDIR_LIB_DIR}" -maxdepth 1 -type f -name '*.so*' | sort)
    if ((${#app_libraries[@]} > 0)); then
        binaries+=("${app_libraries[@]}")
    fi

    mapfile -t component_libraries < <(find "${APPDIR_BIN_DIR}/components" -maxdepth 1 -type f -name '*.so*' 2>/dev/null | sort)
    if ((${#component_libraries[@]} > 0)); then
        binaries+=("${component_libraries[@]}")
    fi

    copy_library_closure "${binaries[@]}"
    copy_qt_plugins
}

validate_dependencies() {
    local missing
    missing="$(
        LD_LIBRARY_PATH="${APPDIR_LIB_DIR}:${APPDIR_BIN_DIR}/components:${LD_LIBRARY_PATH:-}" \
        ldd "${APPDIR_BIN_DIR}/xamp.bin" | grep 'not found' || true
    )"
    if [[ -n "${missing}" ]]; then
        printf '%s\n' "${missing}" >&2
        exit 1
    fi
}

require_command cmake
require_command ldd
require_command find
require_command qmake6
if [[ "${MODE}" == "zip" ]]; then
    require_command zip
fi

init_log_file

if [[ "${SKIP_BUILD}" != "1" ]]; then
    step_start "build preset ${PRESET}"
    cmake --build "${BUILD_DIR}" --target xamp -j "$(nproc)"
    step_done
else
    log "Skip build; use existing build directory: ${BUILD_DIR}"
fi

[[ -x "${APP_BIN}" ]] || die "Cannot find built xamp executable: ${APP_BIN}"

step_start "create AppDir"
rm -rf "${APPDIR}"
mkdir -p "${APPDIR_BIN_DIR}" "${APPDIR_LIB_DIR}" "$(dirname "${APPDIR_ICON}")"
cp -a "${APP_BIN}" "${APPDIR_BIN_DIR}/xamp.bin"
copy_if_exists "${ROOT_DIR}/src/xamp/xamp2.png" "${APPDIR_ICON}"
copy_if_exists "${APPDIR_ICON}" "${APPDIR_PIXMAP_ICON}"
write_desktop_file
copy_if_exists "${APPDIR_ICON}" "${APPDIR}/.DirIcon"
copy_if_exists "${APPDIR_ICON}" "${APPDIR}/xamp.png"
copy_if_exists "${APPDIR_DESKTOP}" "${APPDIR}/xamp.desktop"
write_apprun
write_launcher
step_done

step_start "copy project libraries"
copy_project_libraries
step_done

step_start "copy runtime data"
copy_xamp_runtime_data
step_done

step_start "copy runtime libraries and Qt plugins"
copy_runtime_libraries
step_done

LINUXDEPLOYQT_BIN="$(find_linuxdeployqt || true)"
if [[ -z "${LINUXDEPLOYQT_BIN}" ]]; then
    log "linuxdeployqt not found; downloading continuous AppImage."
    LINUXDEPLOYQT_BIN="$(download_linuxdeployqt)"
fi

if [[ -n "${LINUXDEPLOYQT_BIN}" ]]; then
    log "linuxdeployqt available: ${LINUXDEPLOYQT_BIN}"
    chmod +x "${LINUXDEPLOYQT_BIN}" || true
    if [[ "${MODE}" == "appimage" ]]; then
        log "Run linuxdeployqt AppImage packaging."
        "${LINUXDEPLOYQT_BIN}" \
            "${APPDIR_BIN_DIR}/xamp.bin" \
            "-qmake=$(command -v qmake6)" \
            "-unsupported-allow-new-glibc" \
            "-appimage" \
            "-no-translations" \
            "-verbose=1"
    fi
fi

validate_dependencies
log "Dependency validation completed."

if [[ "${MODE}" == "zip" ]]; then
    step_start "create zip package ${ZIP_FILE}"
    rm -f "${ZIP_FILE}"
    (
        cd "${DEPLOY_ROOT}/.."
        zip -qry "$(basename "${ZIP_FILE}")" "$(basename "${DEPLOY_ROOT}")/XAMP.AppDir"
    )
    step_done
fi

log "Deploy completed: ${APPDIR}"
log "Run AppDir: ${APPDIR}/AppRun"
log "Run direct: ${APPDIR_BIN_DIR}/xamp"
if [[ "${MODE}" == "zip" ]]; then
    log "Zip package: ${ZIP_FILE}"
fi
