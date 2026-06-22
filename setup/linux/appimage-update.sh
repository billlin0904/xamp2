#!/usr/bin/env sh
set -eu

old_pid="$1"
new_appimage="$2"
target_appimage="$3"
log_file="$4"

{
    echo "[update] start $(date -Is)"
    echo "[update] old pid: ${old_pid}"
    echo "[update] new appimage: ${new_appimage}"
    echo "[update] target appimage: ${target_appimage}"

    while kill -0 "${old_pid}" 2>/dev/null; do
        sleep 1
    done

    target_dir="$(dirname "${target_appimage}")"
    target_name="$(basename "${target_appimage}")"
    staged="${target_dir}/.${target_name}.new"
    backup="${target_dir}/.${target_name}.old"

    cp "${new_appimage}" "${staged}"
    chmod +x "${staged}"

    if [ -e "${target_appimage}" ]; then
        mv "${target_appimage}" "${backup}"
    fi

    mv "${staged}" "${target_appimage}"
    rm -f "${backup}"

    echo "[update] relaunch ${target_appimage}"
    nohup "${target_appimage}" >/dev/null 2>&1 &
    echo "[update] done $(date -Is)"
} >> "${log_file}" 2>&1
