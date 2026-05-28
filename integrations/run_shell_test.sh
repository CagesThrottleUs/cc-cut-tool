#!/usr/bin/env bash
# Wrapper that runs a coreutils-style shell test script.
#
# These scripts source tests/init.sh from the coreutils build tree.
# If that infrastructure is absent we exit 77 so CMake marks the
# test as SKIP (requires SKIP_RETURN_CODE 77 in set_tests_properties).
#
# Usage: run_shell_test.sh <script> <binary>

SCRIPT="${1:?missing script argument}"
BINARY="${2:?missing binary argument}"

SCRIPT_NAME=$(basename "$SCRIPT")

# The coreutils init.sh is looked up via $srcdir.
# Try a few conventional locations before giving up.
for candidate in \
        "${srcdir:-.}/tests/init.sh" \
        "$(dirname "$SCRIPT")/tests/init.sh" \
        "/usr/lib/coreutils/tests/init.sh"; do
    if [ -f "$candidate" ]; then
        INIT_SH="$candidate"
        break
    fi
done

if [ -z "${INIT_SH:-}" ]; then
    echo "SKIP  $SCRIPT_NAME  (coreutils test infrastructure not found — tests/init.sh missing)"
    exit 77
fi

# Prepend the directory containing our binary to PATH so that scripts
# which call 'cut' will pick up our implementation.
export PATH="$(dirname "$BINARY"):$PATH"
export srcdir

exec bash "$SCRIPT"
