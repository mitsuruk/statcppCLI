#!/bin/bash
# statcpp CLI - E2E Test Runner
# Usage: cd test/e2e && bash run_e2e.sh

set -eu

STATCPP="../../build/statcpp"
PASS=0
FAIL=0
TOTAL=0

run_test() {
    local name="$1"
    local cmd="$2"
    local golden="$3"
    TOTAL=$((TOTAL + 1))

    local actual
    actual=$(eval "$cmd" 2>&1) || true
    local expected
    expected=$(cat "$golden")

    if [ "$actual" = "$expected" ]; then
        echo "  PASS: $name"
        PASS=$((PASS + 1))
    else
        echo "  FAIL: $name"
        echo "    Expected:"
        echo "$expected" | head -5 | sed 's/^/      /'
        echo "    Actual:"
        echo "$actual" | head -5 | sed 's/^/      /'
        FAIL=$((FAIL + 1))
    fi
}

echo "statcpp CLI E2E Tests"
echo "====================="

# Check binary exists
if [ ! -f "$STATCPP" ]; then
    echo "ERROR: Binary not found at $STATCPP"
    echo "Build first: cmake -B build && cmake --build build"
    exit 1
fi

echo ""
echo "desc category:"
run_test "desc summary"       "$STATCPP desc summary data/basic.csv --col value"      "golden/desc_summary.txt"
run_test "desc summary json"  "$STATCPP desc summary data/basic.csv --col value --json" "golden/desc_summary.json"
run_test "desc mean"          "$STATCPP desc mean data/basic.csv --col value"           "golden/desc_mean.txt"
run_test "desc mean quiet"    "$STATCPP desc mean data/basic.csv --col value --quiet"   "golden/desc_mean_quiet.txt"
run_test "desc mean (NA)"     "$STATCPP desc mean data/missing.csv --col value"         "golden/desc_mean_na.txt"
run_test "desc quartiles"     "$STATCPP desc quartiles data/basic.csv --col value"      "golden/desc_quartiles.txt"
run_test "desc five-number"   "$STATCPP desc five-number data/basic.csv --col value"    "golden/desc_five_number.txt"
run_test "quoted CSV"         "$STATCPP desc mean data/quoted.csv --col value"           "golden/quoted_mean.txt"
run_test "noheader CSV"       "$STATCPP desc mean data/noheader.csv --noheader --col 1"  "golden/noheader_mean.txt"

echo ""
echo "shortcuts:"
run_test "shortcut mean"      "$STATCPP mean data/basic.csv --col score"               "golden/shortcut_mean.txt"

echo ""
echo "test category:"
run_test "test t (two-sample)" "$STATCPP test t data/two_groups.csv --col group1,group2"   "golden/test_t_two_sample.txt"
run_test "test t (one-sample)" "$STATCPP test t data/two_groups.csv --col group1 --mu0 25" "golden/test_t_one_sample.txt"

echo ""
echo "corr category:"
run_test "corr pearson"        "$STATCPP corr pearson data/two_groups.csv --col group1,group2" "golden/corr_pearson.txt"

echo ""
echo "effect category:"
run_test "effect cohens-d"     "$STATCPP effect cohens-d data/two_groups.csv --col group1,group2" "golden/effect_cohens_d.txt"

echo ""
echo "ci category:"
run_test "ci mean"             "$STATCPP ci mean data/two_groups.csv --col group1"         "golden/ci_mean.txt"
run_test "ci sample-size"      "$STATCPP ci sample-size --moe 0.03"                        "golden/ci_sample_size.txt"

echo ""
echo "reg category:"
run_test "reg simple"            "$STATCPP reg simple data/two_groups.csv --col group1,group2"       "golden/reg_simple.txt"
run_test "reg vif"               "$STATCPP reg vif data/two_groups.csv --col group1,group2"          "golden/reg_vif.txt"

echo ""
echo "anova category:"
run_test "anova oneway"          "$STATCPP anova oneway data/two_groups.csv --col group1,group2"     "golden/anova_oneway.txt"

echo ""
echo "ts category:"
run_test "ts acf"                "$STATCPP ts acf data/two_groups.csv --col group1"                  "golden/ts_acf.txt"
run_test "ts diff"               "$STATCPP ts diff data/two_groups.csv --col group1"                 "golden/ts_diff.txt"

echo ""
echo "robust category:"
run_test "robust mad"            "$STATCPP robust mad data/two_groups.csv --col group1"              "golden/robust_mad.txt"
run_test "robust outliers"       "$STATCPP robust outliers data/basic.csv --col value"               "golden/robust_outliers.txt"
run_test "robust hodges-lehmann" "$STATCPP robust hodges-lehmann data/two_groups.csv --col group1"   "golden/robust_hodges_lehmann.txt"

echo ""
echo "survival category:"
run_test "survival kaplan-meier" "$STATCPP survival kaplan-meier data/survival.csv --col time,event" "golden/survival_km.txt"
run_test "survival nelson-aalen" "$STATCPP survival nelson-aalen data/survival.csv --col time,event" "golden/survival_na.txt"

echo ""
echo "multiple category:"
run_test "multiple bonferroni"   "$STATCPP multiple bonferroni data/pvalues.csv --col pvalue"        "golden/multiple_bonferroni.txt"
run_test "multiple bh"           "$STATCPP multiple bh data/pvalues.csv --col pvalue"                "golden/multiple_bh.txt"

echo ""
echo "power category:"
run_test "power t-one"              "$STATCPP power t-one --effect 0.5 --n 30"                             "golden/power_t_one.txt"
run_test "power t-one sample-size"  "$STATCPP power t-one --effect 0.5 --power 0.8"                       "golden/power_t_one_samplesize.txt"
run_test "power t-two"              "$STATCPP power t-two --effect 0.5 --n 30"                             "golden/power_t_two.txt"
run_test "power prop"               "$STATCPP power prop --p1 0.3 --p2 0.5 --n 50"                        "golden/power_prop.txt"

echo ""
echo "--row mode:"
run_test "row comma mean"      "echo '1,2,3,4,5' | $STATCPP desc mean --noheader --col 1 --row"        "golden/row_comma_mean.txt"
run_test "row space mean"      "echo '1 2 3 4 5' | $STATCPP desc mean --noheader --col 1 --row"        "golden/row_space_mean.txt"
run_test "row comma mean json" "echo '1,2,3,4,5' | $STATCPP desc mean --noheader --col 1 --row --json" "golden/row_comma_mean.json"

echo ""
echo "JSON output (per-category):"
run_test "test t json"             "$STATCPP test t data/two_groups.csv --col group1,group2 --json"            "golden/test_t_two_sample.json"
run_test "corr pearson json"       "$STATCPP corr pearson data/two_groups.csv --col group1,group2 --json"      "golden/corr_pearson.json"
run_test "effect cohens-d json"    "$STATCPP effect cohens-d data/two_groups.csv --col group1,group2 --json"    "golden/effect_cohens_d.json"
run_test "effect cohens-h json"    "$STATCPP effect cohens-h --p1 0.6 --p2 0.4 --json"                         "golden/effect_cohens_h.json"
run_test "ci mean json"            "$STATCPP ci mean data/two_groups.csv --col group1 --json"                   "golden/ci_mean.json"
run_test "ci prop json"            "$STATCPP ci prop --successes 45 --trials 100 --json"                        "golden/ci_prop.json"
run_test "ci sample-size json"     "$STATCPP ci sample-size --moe 0.03 --json"                                  "golden/ci_sample_size.json"
run_test "reg simple json"         "$STATCPP reg simple data/two_groups.csv --col group1,group2 --json"         "golden/reg_simple.json"
run_test "anova oneway json"       "$STATCPP anova oneway data/two_groups.csv --col group1,group2 --json"       "golden/anova_oneway.json"
run_test "ts diff json"            "$STATCPP ts diff data/two_groups.csv --col group1 --json"                   "golden/ts_diff.json"
run_test "robust mad json"         "$STATCPP robust mad data/two_groups.csv --col group1 --json"                "golden/robust_mad.json"
run_test "survival km json"        "$STATCPP survival kaplan-meier data/survival.csv --col time,event --json"    "golden/survival_km.json"
run_test "multiple bonferroni json" "$STATCPP multiple bonferroni data/pvalues.csv --col pvalue --json"          "golden/multiple_bonferroni.json"
run_test "power t-one json"        "$STATCPP power t-one --effect 0.5 --n 30 --json"                            "golden/power_t_one.json"

echo ""
echo "error cases:"
# Missing file
TOTAL=$((TOTAL + 1))
if $STATCPP desc mean nonexistent.csv --col value 2>&1 | grep -q "Error"; then
    echo "  PASS: missing file error"
    PASS=$((PASS + 1))
else
    echo "  FAIL: missing file error"
    FAIL=$((FAIL + 1))
fi

# Missing --col
TOTAL=$((TOTAL + 1))
if $STATCPP desc mean data/basic.csv 2>&1 | grep -q "Error"; then
    echo "  PASS: missing --col error"
    PASS=$((PASS + 1))
else
    echo "  FAIL: missing --col error"
    FAIL=$((FAIL + 1))
fi

# Unknown command
TOTAL=$((TOTAL + 1))
if $STATCPP foobar 2>&1 | grep -q "Error"; then
    echo "  PASS: unknown command error"
    PASS=$((PASS + 1))
else
    echo "  FAIL: unknown command error"
    FAIL=$((FAIL + 1))
fi

echo ""
echo "====================="
echo "Results: $PASS/$TOTAL passed, $FAIL failed"

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
