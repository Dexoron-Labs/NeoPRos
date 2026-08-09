#!/bin/bash
# Сборка программ .BIN для образа FAT12.
# Порядок сборки: 1) rm -rf target/i386-none-elf/debug
#                 2) этот скрипт (собирает hello.bin, test.bin, ...)
#                 3) dcr build --force (ядро + [archive] собирает neopros.fs)
set -e

PROFILE="${1:-debug}"
TARGET="target/i386-none-elf/$PROFILE"
CLANG="${CLANG:-clang}"

mkdir -p "$TARGET"

CFLAGS=(--target=i386-none-elf -ffreestanding -fno-builtin
        -fno-stack-protector -fno-pic -fno-pie -mno-sse -mno-mmx
        -mno-80387 -O2 -Wall -Wextra -nostdlib -I src
        -Wl,-Ttext=0x400000 -Wl,-e,_start -Wl,--oformat=binary)

for src in programs/*.c; do
    name=$(basename "$src" .c)
    "$CLANG" "${CFLAGS[@]}" "$src" -o "$TARGET/$name.bin"
    echo "built $TARGET/$name.bin"
done
