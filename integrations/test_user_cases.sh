#!/usr/bin/env bash
# Integration tests: the 8 test cases specified by the user.
#
# Usage: test_user_cases.sh <binary> <sample-dir>

set -uo pipefail

CUT="${1:?missing binary argument}"
SAMPLE_DIR="${2:?missing sample-dir argument}"

PASS=0
FAIL=0

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# ---------- helpers ----------

check() {
    local name="$1" exp="$2" act="$3"
    if diff -q "$exp" "$act" >/dev/null 2>&1; then
        PASS=$((PASS + 1))
        echo "PASS  $name"
    else
        FAIL=$((FAIL + 1))
        echo "FAIL  $name"
        diff --unified "$exp" "$act" || true
    fi
}

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

# ---------- test 1: cut -f2 sample.tsv ----------

printf 'f1\n1\n6\n11\n16\n21\n' > "$TMP/exp1"
"$CUT" -f2 "$SAMPLE_DIR/sample.tsv" > "$TMP/act1"
check "cut -f2 sample.tsv" "$TMP/exp1" "$TMP/act1"

# ---------- test 2: cut -f1 -d, fourchords.csv | head -n5 ----------
# The CSV has a UTF-8 BOM (0xEF 0xBB 0xBF) at byte 0, so the first
# field of the first line carries those three bytes.

{
    printf '\xef\xbb\xbfSong title\n'
    printf '"10000 Reasons (Bless the Lord)"\n'
    printf '"20 Good Reasons"\n'
    printf '"Adore You"\n'
    printf '"Africa"\n'
} > "$TMP/exp2"
"$CUT" -f1 -d, "$SAMPLE_DIR/fourchords.csv" | head -n5 > "$TMP/act2"
check "cut -f1 -d, fourchords.csv | head -n5" "$TMP/exp2" "$TMP/act2"

# ---------- test 3: cut -f1 sample.tsv ----------

printf 'f0\n0\n5\n10\n15\n20\n' > "$TMP/exp3"
"$CUT" -f1 "$SAMPLE_DIR/sample.tsv" > "$TMP/act3"
check "cut -f1 sample.tsv" "$TMP/exp3" "$TMP/act3"

# ---------- test 4: cut -f1,2 sample.tsv ----------
# Output uses the input delimiter (tab) between selected fields.

printf 'f0\tf1\n0\t1\n5\t6\n10\t11\n15\t16\n20\t21\n' > "$TMP/exp4"
"$CUT" -f1,2 "$SAMPLE_DIR/sample.tsv" > "$TMP/act4"
check "cut -f1,2 sample.tsv" "$TMP/exp4" "$TMP/act4"

# ---------- test 5: cut -d, -f"1 2" fourchords.csv | head -n5 ----------
# Space-separated field list inside quotes is equivalent to comma-separated.

{
    printf '\xef\xbb\xbfSong title,Artist\n'
    printf '"10000 Reasons (Bless the Lord)",Matt Redman and Jonas Myrin\n'
    printf '"20 Good Reasons",Thirsty Merc\n'
    printf '"Adore You",Harry Styles\n'
    printf '"Africa",Toto\n'
} > "$TMP/exp5"
"$CUT" -d, "-f1 2" "$SAMPLE_DIR/fourchords.csv" | head -n5 > "$TMP/act5"
check 'cut -d, -f"1 2" fourchords.csv | head -n5' "$TMP/exp5" "$TMP/act5"

# ---------- test 6: tail -n5 fourchords.csv | cut -d, -f"1 2" ----------

{
    printf '"Young Volcanoes",Fall Out Boy\n'
    printf '"You Found Me",The Fray\n'
    printf '"You'"'"'ll Think Of Me",Keith Urban\n'
    printf '"You'"'"'re Not Sorry",Taylor Swift\n'
    printf '"Zombie",The Cranberries\n'
} > "$TMP/exp6"
tail -n5 "$SAMPLE_DIR/fourchords.csv" \
    | "$CUT" -d, "-f1 2" > "$TMP/act6"
check 'tail -n5 fourchords.csv | cut -d, -f"1 2"' "$TMP/exp6" "$TMP/act6"

# ---------- test 7: same but with explicit '-' for stdin ----------

tail -n5 "$SAMPLE_DIR/fourchords.csv" \
    | "$CUT" -d, "-f1 2" - > "$TMP/act7"
check 'tail -n5 fourchords.csv | cut -d, -f"1 2" -' "$TMP/exp6" "$TMP/act7"

# ---------- test 8: cut -f2 -d, fourchords.csv | uniq | wc -l → 155 ----------

actual8=$("$CUT" -f2 -d, "$SAMPLE_DIR/fourchords.csv" | uniq | wc -l | awk '{print $1}')
check_val 'cut -f2 -d, fourchords.csv | uniq | wc -l' "155" "$actual8"

# ---------- summary ----------

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ] || exit 1
