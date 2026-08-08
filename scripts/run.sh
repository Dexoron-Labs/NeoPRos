#!/usr/bin/env bash
# NeoPRos — запуск ядра в QEMU.
# Использование:
#   scripts/run.sh          — отладочная сборка (debug)
#   scripts/run.sh --release — релизная сборка
set -euo pipefail

cd "$(dirname "$0")/.."

PROFILE="debug"
case "${1:-}" in
    --release) PROFILE="release" ;;
    "") ;;
    *) echo "Неизвестный аргумент: $1" >&2; exit 1 ;;
esac

KERNEL="target/i386-none-elf/${PROFILE}/neopros.bin"

if [[ ! -f "$KERNEL" ]]; then
    echo "Образ не найден: $KERNEL" >&2
    echo "Соберите ядро: dcr build${1:+ $1}" >&2
    exit 1
fi

exec qemu-system-i386 -kernel "$KERNEL" -serial stdio
