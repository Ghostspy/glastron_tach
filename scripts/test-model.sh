#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build-host
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pedantic \
    -fsanitize=address,undefined -g -I main \
    tests/test_model.c main/tach_model.c -lm -o build-host/test-model
./build-host/test-model
