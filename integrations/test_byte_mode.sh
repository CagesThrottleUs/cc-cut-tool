#!/usr/bin/env bash
# Integration tests: TC-REQ007 byte-mode acceptance scenarios.
#
# Usage: test_byte_mode.sh <binary>

set -uo pipefail

CUT="${1:?missing binary argument}"

PASS=0
FAIL=0

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# ---------- helpers ----------

check_val() {
    local name="$1" exp="$2" act="$3"
    if [ "$exp" = "$act" ]; then
        PASS=$((PASS + 1))
        echo "PASS  $name"
    else
        FAIL=$((FAIL + 1))
        echo "FAIL  $name"
        echo "  expected: $exp"
        echo "  actual:   $act"
    fi
}

check_bytes() {
    local name="$1" exp_hex="$2"
    local act_hex
    act_hex=$(cat "$TMP/act_bytes" | xxd -p | tr -d '\n')
    if [ "$exp_hex" = "$act_hex" ]; then
        PASS=$((PASS + 1))
        echo "PASS  $name"
    else
        FAIL=$((FAIL + 1))
        echo "FAIL  $name"
        echo "  expected hex: $exp_hex"
        echo "  actual hex:   $act_hex"
    fi
}

check_exit_and_stderr() {
    local name="$1" exp_exit="$2" exp_msg="$3"
    local act_exit act_stderr
    act_stderr=$(eval "${@:4}" 2>&1 >/dev/null) || true
    act_exit=$("${@:4}" >/dev/null 2>&1; echo $?) || true
    # run again cleanly to capture exit code
    "${@:4}" >/dev/null 2>&1
    act_exit=$?
    if [ "$act_exit" -eq "$exp_exit" ] && echo "$act_stderr" | grep -qF "$exp_msg"; then
        PASS=$((PASS + 1))
        echo "PASS  $name"
    else
        FAIL=$((FAIL + 1))
        echo "FAIL  $name"
        echo "  expected exit=$exp_exit, stderr contains: $exp_msg"
        echo "  actual   exit=$act_exit, stderr: $act_stderr"
    fi
}

# ---------- TC-REQ007-01: single byte select ----------
# echo "hello" | cc-cut-tool -b1  → "h\n"

actual01=$(echo "hello" | "$CUT" -b1)
check_val "TC-REQ007-01: -b1 selects first byte" "h" "$actual01"

# ---------- TC-REQ007-02: byte range ----------
# echo "hello" | cc-cut-tool -b1-3  → "hel\n"

actual02=$(echo "hello" | "$CUT" -b1-3)
check_val "TC-REQ007-02: -b1-3 selects byte range" "hel" "$actual02"

# ---------- TC-REQ007-03: -n flag preserves multibyte sequence ----------
# printf '\xC3\xA9\n' | cc-cut-tool -b1 -n  → both bytes of é (C3 A9) + newline

printf '\xC3\xA9\n' | "$CUT" -b1 -n > "$TMP/act_bytes"
# expected: c3 a9 0a
check_bytes "TC-REQ007-03: -b1 -n keeps full multibyte char" "c3a90a"

# ---------- TC-REQ007-03b: raw byte slice without -n ----------
# printf '\xC3\xA9\n' | cc-cut-tool -b1  → only first raw byte (C3) + newline

printf '\xC3\xA9\n' | "$CUT" -b1 > "$TMP/act_bytes"
# expected: c3 0a
check_bytes "TC-REQ007-03b: -b1 without -n gives raw byte only" "c30a"

# ---------- TC-REQ007-04: -c1 selects first codepoint (SP-07 shipped) ----------
# echo "hello" | cc-cut-tool -c1  → "h", exit 0

actual04=$(echo "hello" | "$CUT" -c1)
check_val "TC-REQ007-04: -c1 selects first character" "h" "$actual04"

# ---------- TC-REQ007-05: field mode regression ----------
# echo "a,b,c" | cc-cut-tool -f2 -d,  → "b\n"

actual05=$(echo "a,b,c" | "$CUT" -f2 -d,)
check_val "TC-REQ007-05: field mode unaffected by byte mode work" "b" "$actual05"

# ---------- summary ----------

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ] || exit 1
