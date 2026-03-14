#!/bin/bash
# =============================================================================
# run_reference.sh - Run and verify all command examples from test-reference.md
#
# Usage:
#   cd statcppCLI
#   bash docs/run_reference.sh
#
# Prerequisites:
#   - build/statcpp is built
#   - test/e2e/data/ contains test data
# =============================================================================

set -euo pipefail

# Path setup
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
STATCPP="$PROJECT_DIR/build/statcpp"
DATA_DIR="$PROJECT_DIR/test/e2e/data"

# Counters
PASS=0
FAIL=0
SKIP=0
TOTAL=0

# Color output
if [ -t 1 ]; then
    GREEN='\033[0;32m'
    RED='\033[0;31m'
    YELLOW='\033[0;33m'
    CYAN='\033[0;36m'
    BOLD='\033[1m'
    RESET='\033[0m'
else
    GREEN='' RED='' YELLOW='' CYAN='' BOLD='' RESET=''
fi

# --- Utilities ---

# Run a command and verify it exits successfully
# usage: run_test "test name" command [args...]
run_test() {
    local name="$1"
    shift
    TOTAL=$((TOTAL + 1))

    local output
    if output=$("$@" 2>&1); then
        PASS=$((PASS + 1))
        printf "  ${GREEN}PASS${RESET}  %s\n" "$name"
    else
        FAIL=$((FAIL + 1))
        printf "  ${RED}FAIL${RESET}  %s\n" "$name"
        printf "        command: %s\n" "$*"
        printf "        output:  %s\n" "$output"
    fi
}

# Run a command and verify output contains expected string
# usage: run_test_expect "test name" "expected string" command [args...]
run_test_expect() {
    local name="$1"
    local expected="$2"
    shift 2
    TOTAL=$((TOTAL + 1))

    local output
    if output=$("$@" 2>&1); then
        if echo "$output" | grep -qF "$expected"; then
            PASS=$((PASS + 1))
            printf "  ${GREEN}PASS${RESET}  %s\n" "$name"
        else
            FAIL=$((FAIL + 1))
            printf "  ${RED}FAIL${RESET}  %s  (expected: %s)\n" "$name" "$expected"
            printf "        output:  %s\n" "$(echo "$output" | head -3)"
        fi
    else
        FAIL=$((FAIL + 1))
        printf "  ${RED}FAIL${RESET}  %s  (command failed)\n" "$name"
        printf "        output:  %s\n" "$output"
    fi
}

# Skip a test (insufficient data, etc.)
skip_test() {
    local name="$1"
    local reason="$2"
    TOTAL=$((TOTAL + 1))
    SKIP=$((SKIP + 1))
    printf "  ${YELLOW}SKIP${RESET}  %s  (%s)\n" "$name" "$reason"
}

# --- Prerequisites check ---
if [ ! -x "$STATCPP" ]; then
    echo "Error: $STATCPP not found. Build first: cmake -B build && cmake --build build"
    exit 1
fi

echo ""
echo "${BOLD}statcpp CLI Reference Verification${RESET}"
echo "binary: $STATCPP"
echo ""

# =============================================================================
# desc - Descriptive Statistics
# =============================================================================
printf "${CYAN}== desc ==${RESET}\n"

run_test_expect "desc summary" \
    "Mean:" \
    "$STATCPP" desc summary "$DATA_DIR/basic.csv" --col value

run_test_expect "desc mean" \
    "Mean:" \
    "$STATCPP" desc mean "$DATA_DIR/basic.csv" --col value

run_test_expect "desc median" \
    "Median:" \
    "$STATCPP" desc median "$DATA_DIR/basic.csv" --col value

run_test_expect "desc mode" \
    "Mode[1]:" \
    "$STATCPP" desc mode "$DATA_DIR/basic.csv" --col value

run_test_expect "desc var" \
    "Variance:" \
    "$STATCPP" desc var "$DATA_DIR/basic.csv" --col value

run_test_expect "desc sd" \
    "Std Dev:" \
    "$STATCPP" desc sd "$DATA_DIR/basic.csv" --col value

run_test_expect "desc range" \
    "Range:" \
    "$STATCPP" desc range "$DATA_DIR/basic.csv" --col value

run_test_expect "desc iqr" \
    "IQR:" \
    "$STATCPP" desc iqr "$DATA_DIR/basic.csv" --col value

run_test_expect "desc cv" \
    "CV:" \
    "$STATCPP" desc cv "$DATA_DIR/basic.csv" --col value

run_test_expect "desc skewness" \
    "Skewness:" \
    "$STATCPP" desc skewness "$DATA_DIR/basic.csv" --col value

run_test_expect "desc kurtosis" \
    "Kurtosis:" \
    "$STATCPP" desc kurtosis "$DATA_DIR/basic.csv" --col value

run_test_expect "desc percentile (p=0.95)" \
    "P95:" \
    "$STATCPP" desc percentile "$DATA_DIR/basic.csv" --col value --p 0.95

run_test_expect "desc quartiles" \
    "Q1:" \
    "$STATCPP" desc quartiles "$DATA_DIR/basic.csv" --col value

run_test_expect "desc five-number" \
    "Min:" \
    "$STATCPP" desc five-number "$DATA_DIR/basic.csv" --col value

run_test_expect "desc gmean" \
    "Geometric Mean:" \
    "$STATCPP" desc gmean "$DATA_DIR/basic.csv" --col value

run_test_expect "desc hmean" \
    "Harmonic Mean:" \
    "$STATCPP" desc hmean "$DATA_DIR/basic.csv" --col value

run_test_expect "desc trimmed-mean" \
    "Trimmed Mean:" \
    "$STATCPP" desc trimmed-mean "$DATA_DIR/basic.csv" --col value

echo ""

# =============================================================================
# test - Statistical Tests
# =============================================================================
printf "${CYAN}== test ==${RESET}\n"

run_test_expect "test t (1-sample)" \
    "p-value:" \
    "$STATCPP" test t "$DATA_DIR/two_groups.csv" --col group1 --mu0 25

run_test_expect "test t (2-sample)" \
    "p-value:" \
    "$STATCPP" test t "$DATA_DIR/two_groups.csv" --col group1,group2

run_test_expect "test welch" \
    "p-value:" \
    "$STATCPP" test welch "$DATA_DIR/two_groups.csv" --col group1,group2

run_test_expect "test z" \
    "p-value:" \
    "$STATCPP" test z "$DATA_DIR/two_groups.csv" --col group1 --mu0 25 --sigma 3

run_test_expect "test f" \
    "p-value:" \
    "$STATCPP" test f "$DATA_DIR/two_groups.csv" --col group1,group2

run_test_expect "test shapiro" \
    "p-value:" \
    "$STATCPP" test shapiro "$DATA_DIR/two_groups.csv" --col group1

run_test "test ks" \
    "$STATCPP" test ks "$DATA_DIR/two_groups.csv" --col group1

run_test_expect "test mann-whitney" \
    "p-value:" \
    "$STATCPP" test mann-whitney "$DATA_DIR/two_groups.csv" --col group1,group2

run_test_expect "test wilcoxon" \
    "p-value:" \
    "$STATCPP" test wilcoxon "$DATA_DIR/two_groups.csv" --col group1 --mu0 25

run_test_expect "test kruskal" \
    "p-value:" \
    "$STATCPP" test kruskal "$DATA_DIR/two_groups.csv" --col group1,group2

run_test_expect "test levene" \
    "p-value:" \
    "$STATCPP" test levene "$DATA_DIR/two_groups.csv" --col group1,group2

run_test_expect "test bartlett" \
    "p-value:" \
    "$STATCPP" test bartlett "$DATA_DIR/two_groups.csv" --col group1,group2

run_test "test chisq" \
    "$STATCPP" test chisq "$DATA_DIR/basic.csv" --col value

echo ""

# =============================================================================
# corr - Correlation & Covariance
# =============================================================================
printf "${CYAN}== corr ==${RESET}\n"

run_test_expect "corr pearson" \
    "r:" \
    "$STATCPP" corr pearson "$DATA_DIR/two_groups.csv" --col group1,group2

run_test_expect "corr spearman" \
    "r:" \
    "$STATCPP" corr spearman "$DATA_DIR/two_groups.csv" --col group1,group2

run_test_expect "corr kendall" \
    "r:" \
    "$STATCPP" corr kendall "$DATA_DIR/two_groups.csv" --col group1,group2

run_test_expect "corr cov" \
    "Covariance:" \
    "$STATCPP" corr cov "$DATA_DIR/two_groups.csv" --col group1,group2

run_test_expect "corr matrix" \
    "math" \
    "$STATCPP" corr matrix "$DATA_DIR/scores.csv" --col math,science,english

echo ""

# =============================================================================
# effect - Effect Sizes
# =============================================================================
printf "${CYAN}== effect ==${RESET}\n"

run_test_expect "effect cohens-d" \
    "Cohen's d:" \
    "$STATCPP" effect cohens-d "$DATA_DIR/two_groups.csv" --col group1,group2

run_test_expect "effect hedges-g" \
    "Hedges' g:" \
    "$STATCPP" effect hedges-g "$DATA_DIR/two_groups.csv" --col group1,group2

run_test "effect glass-delta" \
    "$STATCPP" effect glass-delta "$DATA_DIR/two_groups.csv" --col group1,group2

run_test "effect cohens-h" \
    "$STATCPP" effect cohens-h --p1 0.6 --p2 0.4

run_test_expect "effect odds-ratio" \
    "Odds Ratio:" \
    "$STATCPP" effect odds-ratio "$DATA_DIR/contingency.csv" --col value

run_test_expect "effect risk-ratio" \
    "Risk Ratio:" \
    "$STATCPP" effect risk-ratio "$DATA_DIR/contingency.csv" --col value

echo ""

# =============================================================================
# ci - Confidence Intervals
# =============================================================================
printf "${CYAN}== ci ==${RESET}\n"

run_test_expect "ci mean" \
    "Lower:" \
    "$STATCPP" ci mean "$DATA_DIR/two_groups.csv" --col group1

run_test_expect "ci diff" \
    "Lower:" \
    "$STATCPP" ci diff "$DATA_DIR/two_groups.csv" --col group1,group2

run_test "ci prop" \
    "$STATCPP" ci prop --successes 45 --trials 100

run_test "ci var" \
    "$STATCPP" ci var "$DATA_DIR/two_groups.csv" --col group1

run_test_expect "ci sample-size" \
    "Sample Size:" \
    "$STATCPP" ci sample-size --moe 0.03

echo ""

# =============================================================================
# reg - Regression
# =============================================================================
printf "${CYAN}== reg ==${RESET}\n"

run_test_expect "reg simple" \
    "R-squared:" \
    "$STATCPP" reg simple "$DATA_DIR/two_groups.csv" --col group1,group2

run_test_expect "reg multiple" \
    "R-squared:" \
    "$STATCPP" reg multiple "$DATA_DIR/scores.csv" --col math,science,english

run_test "reg predict" \
    "$STATCPP" reg predict "$DATA_DIR/two_groups.csv" --col group1,group2

run_test "reg residuals" \
    "$STATCPP" reg residuals "$DATA_DIR/two_groups.csv" --col group1,group2

run_test "reg vif" \
    "$STATCPP" reg vif "$DATA_DIR/two_groups.csv" --col group1,group2

echo ""

# =============================================================================
# anova - ANOVA
# =============================================================================
printf "${CYAN}== anova ==${RESET}\n"

run_test_expect "anova oneway" \
    "F-statistic:" \
    "$STATCPP" anova oneway "$DATA_DIR/two_groups.csv" --col group1,group2

run_test_expect "anova posthoc-tukey" \
    "Tukey" \
    "$STATCPP" anova posthoc-tukey "$DATA_DIR/scores.csv" --col math,science,english

run_test "anova posthoc-bonferroni" \
    "$STATCPP" anova posthoc-bonferroni "$DATA_DIR/scores.csv" --col math,science,english

run_test "anova posthoc-scheffe" \
    "$STATCPP" anova posthoc-scheffe "$DATA_DIR/scores.csv" --col math,science,english

run_test_expect "anova eta-squared" \
    "eta-squared:" \
    "$STATCPP" anova eta-squared "$DATA_DIR/scores.csv" --col math,science,english

echo ""

# =============================================================================
# resample - Resampling (random-dependent, check exit status only)
# =============================================================================
printf "${CYAN}== resample ==${RESET}\n"

run_test "resample bootstrap-mean" \
    "$STATCPP" resample bootstrap-mean "$DATA_DIR/two_groups.csv" --col group1

run_test "resample bootstrap-median" \
    "$STATCPP" resample bootstrap-median "$DATA_DIR/two_groups.csv" --col group1

run_test "resample bootstrap-sd" \
    "$STATCPP" resample bootstrap-sd "$DATA_DIR/two_groups.csv" --col group1

run_test "resample bca" \
    "$STATCPP" resample bca "$DATA_DIR/two_groups.csv" --col group1

run_test "resample permtest" \
    "$STATCPP" resample permtest "$DATA_DIR/two_groups.csv" --col group1,group2

run_test "resample permtest-corr" \
    "$STATCPP" resample permtest-corr "$DATA_DIR/two_groups.csv" --col group1,group2

echo ""

# =============================================================================
# ts - Time Series
# =============================================================================
printf "${CYAN}== ts ==${RESET}\n"

run_test "ts acf" \
    "$STATCPP" ts acf "$DATA_DIR/two_groups.csv" --col group1

run_test "ts pacf" \
    "$STATCPP" ts pacf "$DATA_DIR/two_groups.csv" --col group1

run_test "ts ma" \
    "$STATCPP" ts ma "$DATA_DIR/two_groups.csv" --col group1

run_test "ts ema" \
    "$STATCPP" ts ema "$DATA_DIR/two_groups.csv" --col group1

run_test_expect "ts diff" \
    "d[1]:" \
    "$STATCPP" ts diff "$DATA_DIR/two_groups.csv" --col group1

run_test_expect "ts mae" \
    "MAE:" \
    "$STATCPP" ts mae "$DATA_DIR/forecast.csv" --col actual,predicted

run_test_expect "ts rmse" \
    "RMSE:" \
    "$STATCPP" ts rmse "$DATA_DIR/forecast.csv" --col actual,predicted

run_test_expect "ts mape" \
    "MAPE" \
    "$STATCPP" ts mape "$DATA_DIR/forecast.csv" --col actual,predicted

echo ""

# =============================================================================
# robust - Robust Statistics
# =============================================================================
printf "${CYAN}== robust ==${RESET}\n"

run_test_expect "robust mad" \
    "MAD:" \
    "$STATCPP" robust mad "$DATA_DIR/two_groups.csv" --col group1

run_test_expect "robust outliers" \
    "N outliers:" \
    "$STATCPP" robust outliers "$DATA_DIR/basic.csv" --col value

run_test "robust outliers-zscore" \
    "$STATCPP" robust outliers-zscore "$DATA_DIR/basic.csv" --col value

run_test "robust outliers-modified" \
    "$STATCPP" robust outliers-modified "$DATA_DIR/basic.csv" --col value

run_test "robust winsorize" \
    "$STATCPP" robust winsorize "$DATA_DIR/basic.csv" --col value --trim 0.05

run_test_expect "robust hodges-lehmann" \
    "Hodges-Lehmann:" \
    "$STATCPP" robust hodges-lehmann "$DATA_DIR/two_groups.csv" --col group1

run_test_expect "robust biweight" \
    "Biweight" \
    "$STATCPP" robust biweight "$DATA_DIR/two_groups.csv" --col group1

echo ""

# =============================================================================
# survival - Survival Analysis
# =============================================================================
printf "${CYAN}== survival ==${RESET}\n"

run_test_expect "survival kaplan-meier" \
    "Kaplan-Meier" \
    "$STATCPP" survival kaplan-meier "$DATA_DIR/survival.csv" --col time,event

run_test_expect "survival logrank" \
    "Chi-square:" \
    "$STATCPP" survival logrank "$DATA_DIR/survival_two.csv" --col time1,event1,time2,event2

run_test_expect "survival nelson-aalen" \
    "Nelson-Aalen" \
    "$STATCPP" survival nelson-aalen "$DATA_DIR/survival.csv" --col time,event

echo ""

# =============================================================================
# cluster - Clustering (random-dependent, check exit status only)
# =============================================================================
printf "${CYAN}== cluster ==${RESET}\n"

run_test "cluster kmeans" \
    "$STATCPP" cluster kmeans "$DATA_DIR/scores.csv" --col math,science,english

run_test "cluster hierarchical" \
    "$STATCPP" cluster hierarchical "$DATA_DIR/scores.csv" --col math,science,english

run_test "cluster silhouette" \
    "$STATCPP" cluster silhouette "$DATA_DIR/scores.csv" --col math,science,english

echo ""

# =============================================================================
# multiple - Multiple Testing Correction
# =============================================================================
printf "${CYAN}== multiple ==${RESET}\n"

run_test_expect "multiple bonferroni" \
    "Bonferroni" \
    "$STATCPP" multiple bonferroni "$DATA_DIR/pvalues.csv" --col pvalue

run_test_expect "multiple holm" \
    "Holm" \
    "$STATCPP" multiple holm "$DATA_DIR/pvalues.csv" --col pvalue

run_test_expect "multiple bh" \
    "Benjamini-Hochberg" \
    "$STATCPP" multiple bh "$DATA_DIR/pvalues.csv" --col pvalue

echo ""

# =============================================================================
# power - Power Analysis (no CSV required)
# =============================================================================
printf "${CYAN}== power ==${RESET}\n"

run_test_expect "power t-one (power)" \
    "Power:" \
    "$STATCPP" power t-one --effect 0.5 --n 30

run_test_expect "power t-one (sample-size)" \
    "Sample size:" \
    "$STATCPP" power t-one --effect 0.5 --power 0.8

run_test_expect "power t-two (power)" \
    "Power:" \
    "$STATCPP" power t-two --effect 0.5 --n 30

run_test_expect "power t-two (sample-size)" \
    "n1:" \
    "$STATCPP" power t-two --effect 0.5 --power 0.8

run_test_expect "power prop (power)" \
    "Power:" \
    "$STATCPP" power prop --p1 0.3 --p2 0.5 --n 50

run_test_expect "power prop (sample-size)" \
    "Sample size:" \
    "$STATCPP" power prop --p1 0.3 --p2 0.5 --power 0.8

echo ""

# =============================================================================
# glm - Generalized Linear Models
# =============================================================================
printf "${CYAN}== glm ==${RESET}\n"

run_test_expect "glm logistic" \
    "Coefficients:" \
    "$STATCPP" glm logistic "$DATA_DIR/binary.csv" --col x1,x2,y

run_test_expect "glm poisson" \
    "Coefficients:" \
    "$STATCPP" glm poisson "$DATA_DIR/count.csv" --col x1,x2,y

echo ""

# =============================================================================
# model - Model Selection
# =============================================================================
printf "${CYAN}== model ==${RESET}\n"

run_test "model aic" \
    "$STATCPP" model aic "$DATA_DIR/scores.csv" --col math,science,english

run_test "model cv" \
    "$STATCPP" model cv "$DATA_DIR/scores.csv" --col math,science,english

run_test "model ridge" \
    "$STATCPP" model ridge "$DATA_DIR/scores.csv" --col math,science,english

run_test "model lasso" \
    "$STATCPP" model lasso "$DATA_DIR/scores.csv" --col math,science,english

echo ""

# =============================================================================
# Output Modes
# =============================================================================
printf "${CYAN}== output modes ==${RESET}\n"

run_test_expect "JSON desc summary" \
    '"command"' \
    "$STATCPP" desc summary "$DATA_DIR/basic.csv" --col value --json

run_test_expect "JSON test t" \
    '"test.t"' \
    "$STATCPP" test t "$DATA_DIR/two_groups.csv" --col group1,group2 --json

run_test_expect "JSON corr pearson" \
    '"corr.pearson"' \
    "$STATCPP" corr pearson "$DATA_DIR/two_groups.csv" --col group1,group2 --json

run_test_expect "JSON effect cohens-d" \
    '"effect.cohens-d"' \
    "$STATCPP" effect cohens-d "$DATA_DIR/two_groups.csv" --col group1,group2 --json

run_test_expect "JSON effect cohens-h" \
    '"effect.cohens-h"' \
    "$STATCPP" effect cohens-h --p1 0.6 --p2 0.4 --json

run_test_expect "JSON ci mean" \
    '"ci.mean"' \
    "$STATCPP" ci mean "$DATA_DIR/two_groups.csv" --col group1 --json

run_test_expect "JSON ci prop" \
    '"ci.prop"' \
    "$STATCPP" ci prop --successes 45 --trials 100 --json

run_test_expect "JSON ci sample-size" \
    '"ci.sample-size"' \
    "$STATCPP" ci sample-size --moe 0.03 --json

run_test_expect "JSON reg simple" \
    '"reg.simple"' \
    "$STATCPP" reg simple "$DATA_DIR/two_groups.csv" --col group1,group2 --json

run_test_expect "JSON anova oneway" \
    '"anova.oneway"' \
    "$STATCPP" anova oneway "$DATA_DIR/two_groups.csv" --col group1,group2 --json

run_test_expect "JSON ts diff" \
    '"ts.diff"' \
    "$STATCPP" ts diff "$DATA_DIR/two_groups.csv" --col group1 --json

run_test_expect "JSON robust mad" \
    '"robust.mad"' \
    "$STATCPP" robust mad "$DATA_DIR/two_groups.csv" --col group1 --json

run_test_expect "JSON survival kaplan-meier" \
    '"survival.kaplan-meier"' \
    "$STATCPP" survival kaplan-meier "$DATA_DIR/survival.csv" --col time,event --json

run_test_expect "JSON multiple bonferroni" \
    '"multiple.bonferroni"' \
    "$STATCPP" multiple bonferroni "$DATA_DIR/pvalues.csv" --col pvalue --json

run_test_expect "JSON power t-one" \
    '"power.t-one"' \
    "$STATCPP" power t-one --effect 0.5 --n 30 --json

run_test_expect "Quiet output (--quiet)" \
    "30" \
    "$STATCPP" desc mean "$DATA_DIR/basic.csv" --col value --quiet

# stdin pipe
TOTAL=$((TOTAL + 1))
PIPE_OUTPUT=$(printf '1\n2\n3\n4\n5\n' | "$STATCPP" desc mean --noheader --col 1 2>&1)
if echo "$PIPE_OUTPUT" | grep -qF "Mean:"; then
    PASS=$((PASS + 1))
    printf "  ${GREEN}PASS${RESET}  stdin pipe input\n"
else
    FAIL=$((FAIL + 1))
    printf "  ${RED}FAIL${RESET}  stdin pipe input\n"
    printf "        output: %s\n" "$PIPE_OUTPUT"
fi

# --row mode (comma separated)
TOTAL=$((TOTAL + 1))
ROW_OUTPUT=$(echo '1,2,3,4,5' | "$STATCPP" desc mean --noheader --col 1 --row 2>&1)
if echo "$ROW_OUTPUT" | grep -qF "Mean:"; then
    PASS=$((PASS + 1))
    printf "  ${GREEN}PASS${RESET}  --row comma separated\n"
else
    FAIL=$((FAIL + 1))
    printf "  ${RED}FAIL${RESET}  --row comma separated\n"
    printf "        output: %s\n" "$ROW_OUTPUT"
fi

# --row mode (space separated)
TOTAL=$((TOTAL + 1))
ROW_OUTPUT2=$(echo '1 2 3 4 5' | "$STATCPP" desc mean --noheader --col 1 --row 2>&1)
if echo "$ROW_OUTPUT2" | grep -qF "Mean:"; then
    PASS=$((PASS + 1))
    printf "  ${GREEN}PASS${RESET}  --row space separated\n"
else
    FAIL=$((FAIL + 1))
    printf "  ${RED}FAIL${RESET}  --row space separated\n"
    printf "        output: %s\n" "$ROW_OUTPUT2"
fi

echo ""

# =============================================================================
# Shortcuts
# =============================================================================
printf "${CYAN}== shortcuts ==${RESET}\n"

run_test_expect "shortcut: mean" \
    "Mean:" \
    "$STATCPP" mean "$DATA_DIR/basic.csv" --col value

run_test_expect "shortcut: median" \
    "Median:" \
    "$STATCPP" median "$DATA_DIR/basic.csv" --col value

run_test_expect "shortcut: sd" \
    "Std Dev:" \
    "$STATCPP" sd "$DATA_DIR/basic.csv" --col value

run_test_expect "shortcut: var" \
    "Variance:" \
    "$STATCPP" var "$DATA_DIR/basic.csv" --col value

run_test_expect "shortcut: summary" \
    "Mean:" \
    "$STATCPP" summary "$DATA_DIR/basic.csv" --col value

run_test_expect "shortcut: ttest" \
    "p-value:" \
    "$STATCPP" ttest "$DATA_DIR/two_groups.csv" --col group1 --mu0 25

run_test_expect "shortcut: pearson" \
    "r:" \
    "$STATCPP" pearson "$DATA_DIR/two_groups.csv" --col group1,group2

run_test_expect "shortcut: spearman" \
    "r:" \
    "$STATCPP" spearman "$DATA_DIR/two_groups.csv" --col group1,group2

run_test_expect "shortcut: kendall" \
    "r:" \
    "$STATCPP" kendall "$DATA_DIR/two_groups.csv" --col group1,group2

echo ""

# =============================================================================
# Results Summary
# =============================================================================
echo "==========================================="
printf "${BOLD}Results: ${RESET}"
printf "${GREEN}PASS: %d${RESET}  " "$PASS"
if [ "$FAIL" -gt 0 ]; then
    printf "${RED}FAIL: %d${RESET}  " "$FAIL"
else
    printf "FAIL: %d  " "$FAIL"
fi
if [ "$SKIP" -gt 0 ]; then
    printf "${YELLOW}SKIP: %d${RESET}  " "$SKIP"
else
    printf "SKIP: %d  " "$SKIP"
fi
printf "TOTAL: %d\n" "$TOTAL"
echo "==========================================="

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
