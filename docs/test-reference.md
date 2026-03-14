# statcpp CLI Test Reference

Basic syntax:

```text
statcpp <category> <command> [options] [file]
statcpp <shortcut> [options] [file]
```

When no file argument is provided, input is read from stdin.
Use `--row` to treat horizontal data (comma- or space-delimited) as a single column.

```bash
# Comma-delimited
echo "1,2,3,4,5" | statcpp desc mean --noheader --col 1 --row
#   Mean:         3

# Space-delimited
echo "1 2 3 4 5" | statcpp desc mean --noheader --col 1 --row
#   Mean:         3
```

All examples in this document are based on files in `test/e2e/data/`.
You can run all examples with `docs/run_reference.sh` and verify the output (results: `docs/output.txt`).

### Test Data

| File | Contents |
| --- | --- |
| `basic.csv` | name, value (10,20,30,40,50), score |
| `two_groups.csv` | group1 (23,25,27,22,24,26,28,21), group2 (28,30,32,29,31,33,35,27) |
| `scores.csv` | math, science, english (10 subjects, 3 scores) |
| `forecast.csv` | actual, predicted (8 forecast accuracy records) |
| `survival.csv` | time (1-8), event (0/1) |
| `survival_two.csv` | time1, event1, time2, event2 (two-group survival data, 6 records each) |
| `pvalues.csv` | pvalue (0.001, 0.013, 0.04, 0.06, 0.50) |
| `contingency.csv` | value (30,10,20,40) — 2x2 contingency table (a,b,c,d) |
| `binary.csv` | x1, x2, y (binary response 0/1, 12 records) |
| `count.csv` | x1, x2, y (count data response, 8 records) |
| `missing.csv` | id, value (contains NA/empty cells) |
| `quoted.csv` | name, city, value (RFC 4180 quoted fields) |
| `noheader.csv` | No header, 3 columns x 3 rows (10,20,30 / 40,50,60 / 70,80,90) |

---

## desc - Descriptive Statistics

### summary

```bash
statcpp desc summary test/e2e/data/basic.csv --col value
```

```text
  Count:        5
  Mean:         30
  Std Dev:      15.8114
  Min:          10
  Q1:           20
  Median:       30
  Q3:           40
  Max:          50
  Skewness:     0
  Kurtosis:     -1.2
```

### mean

```bash
statcpp desc mean test/e2e/data/basic.csv --col value
```

```text
  Mean:         30
```

### median

Data is automatically sorted internally (use `--presorted` to skip sorting).

```bash
statcpp desc median test/e2e/data/basic.csv --col value
```

```text
  Median:       30
```

### mode

All modes are displayed if there are multiple.

```bash
statcpp desc mode test/e2e/data/basic.csv --col value
```

```text
  Mode[1]:      10
  Mode[2]:      20
  Mode[3]:      30
  Mode[4]:      40
  Mode[5]:      50
```

### var

Default is sample variance (ddof=1). Use `--population` for population variance.

```bash
statcpp desc var test/e2e/data/basic.csv --col value
```

```text
  Variance:     250
```

### sd

```bash
statcpp desc sd test/e2e/data/basic.csv --col value
```

```text
  Std Dev:      15.8114
```

### range

```bash
statcpp desc range test/e2e/data/basic.csv --col value
```

```text
  Range:        40
```

### iqr

```bash
statcpp desc iqr test/e2e/data/basic.csv --col value
```

```text
  IQR:          20
```

### cv

Coefficient of variation (CV = SD / Mean).

```bash
statcpp desc cv test/e2e/data/basic.csv --col value
```

```text
  CV:           0.527046
```

### skewness

```bash
statcpp desc skewness test/e2e/data/basic.csv --col value
```

```text
  Skewness:     0
```

### kurtosis

Excess kurtosis. Equals 0 for a normal distribution.

```bash
statcpp desc kurtosis test/e2e/data/basic.csv --col value
```

```text
  Kurtosis:     -1.2
```

### percentile

Specify the position with `--p` (0.0-1.0).

```bash
statcpp desc percentile test/e2e/data/basic.csv --col value --p 0.95
```

```text
  P95:          48
```

### quartiles

```bash
statcpp desc quartiles test/e2e/data/basic.csv --col value
```

```text
  Q1:           20
  Q2:           30
  Q3:           40
```

### five-number

```bash
statcpp desc five-number test/e2e/data/basic.csv --col value
```

```text
  Min:          10
  Q1:           20
  Median:       30
  Q3:           40
  Max:          50
```

### gmean

Geometric mean.

```bash
statcpp desc gmean test/e2e/data/basic.csv --col value
```

```text
  Geometric Mean:26.0517
```

### hmean

Harmonic mean.

```bash
statcpp desc hmean test/e2e/data/basic.csv --col value
```

```text
  Harmonic Mean:21.8978
```

### trimmed-mean

Specify the trim ratio with `--trim` (default 0.1 = top and bottom 10%).

```bash
statcpp desc trimmed-mean test/e2e/data/basic.csv --col value
```

```text
  Trimmed Mean: 30
```

---

## test - Statistical Tests

`--alternative` (two-sided / less / greater) and `--alpha` (default 0.05) can be specified.

### t

Automatically determined by number of columns.

```bash
# One-sample t-test
statcpp test t test/e2e/data/two_groups.csv --col group1 --mu0 25
```

```text
  Statistic:    -0.57735
  df:           7
  p-value:      0.581788
  alpha:        0.05
  Fail to reject H0
```

```bash
# Two-sample t-test (independent)
statcpp test t test/e2e/data/two_groups.csv --col group1,group2
```

```text
  Statistic:    -4.78191
  df:           14
  p-value:      0.000292342
  alpha:        0.05
  Reject H0
```

Paired: add `--paired`.

### welch

```bash
statcpp test welch test/e2e/data/two_groups.csv --col group1,group2
```

```text
  Statistic:    -4.78191
  df:           13.8979
  p-value:      0.00029825
  alpha:        0.05
  Reject H0
```

### z

Requires population standard deviation `--sigma`.

```bash
statcpp test z test/e2e/data/two_groups.csv --col group1 --mu0 25 --sigma 3
```

### f

F-test (test for equality of variances).

```bash
statcpp test f test/e2e/data/two_groups.csv --col group1,group2
```

```text
  Statistic:    0.842105
  df:           7
  p-value:      0.82645
  alpha:        0.05
  Fail to reject H0
```

### shapiro

Shapiro-Wilk test for normality.

```bash
statcpp test shapiro test/e2e/data/two_groups.csv --col group1
```

```text
  Statistic:    0.974858
  df:           8
  p-value:      0.933165
  alpha:        0.05
  Fail to reject H0
```

### ks

Lilliefors test for normality. This is a Lilliefors test (not a standard KS test) because the mean and variance are estimated from the data.

```bash
statcpp test ks test/e2e/data/two_groups.csv --col group1
```

### mann-whitney

```bash
statcpp test mann-whitney test/e2e/data/two_groups.csv --col group1,group2
```

```text
  Statistic:    2
  df:           16
  p-value:      0.00160361
  alpha:        0.05
  Reject H0
```

### wilcoxon

One column for one-sample test, two columns for paired test.

```bash
statcpp test wilcoxon test/e2e/data/two_groups.csv --col group1 --mu0 25
```

```text
  Statistic:    10.5
  df:           7
  p-value:      0.61017
  alpha:        0.05
  Fail to reject H0
```

### kruskal

```bash
statcpp test kruskal test/e2e/data/two_groups.csv --col group1,group2
```

```text
  Statistic:    9.95575
  df:           1
  p-value:      0.00160348
  alpha:        0.05
  Reject H0
```

### levene

```bash
statcpp test levene test/e2e/data/two_groups.csv --col group1,group2
```

```text
  Statistic:    0.0366492
  df:           1
  p-value:      0.85093
  alpha:        0.05
  Fail to reject H0
```

### bartlett

```bash
statcpp test bartlett test/e2e/data/two_groups.csv --col group1,group2
```

```text
  Statistic:    0.0481772
  df:           1
  p-value:      0.826266
  alpha:        0.05
  Fail to reject H0
```

### chisq

One column for goodness-of-fit test (uniform distribution), two columns for observed/expected frequency comparison.

```bash
statcpp test chisq test/e2e/data/basic.csv --col value
```

---

## corr - Correlation & Covariance

### pearson

```bash
statcpp corr pearson test/e2e/data/two_groups.csv --col group1,group2
```

```text
  r:            0.928587
```

### spearman

```bash
statcpp corr spearman test/e2e/data/two_groups.csv --col group1,group2
```

```text
  r:            0.928571
```

### kendall

```bash
statcpp corr kendall test/e2e/data/two_groups.csv --col group1,group2
```

```text
  r:            0.785714
```

### cov

```bash
statcpp corr cov test/e2e/data/two_groups.csv --col group1,group2
```

```text
  Covariance:   6.07143
```

### matrix

Displays the correlation matrix for 3 or more columns.

```bash
statcpp corr matrix test/e2e/data/scores.csv --col math,science,english
```

```text
                      math     science     english
  math              1.0000      0.9391      0.9260
  science           0.9391      1.0000      0.8496
  english           0.9260      0.8496      1.0000
```

---

## effect - Effect Size

### cohens-d

```bash
statcpp effect cohens-d test/e2e/data/two_groups.csv --col group1,group2
```

```text
  Cohen's d:    -2.39096
  Interpretation: large
```

For one-sample, specify `--mu0`.

### hedges-g

Small-sample corrected.

```bash
statcpp effect hedges-g test/e2e/data/two_groups.csv --col group1,group2
```

```text
  Hedges' g:    -2.26054
  Interpretation: large
```

### glass-delta

Standardized by the control group's SD.

```bash
statcpp effect glass-delta test/e2e/data/two_groups.csv --col group1,group2
```

### cohens-h

Effect size for proportions. Specify proportions with `--p1` and `--p2`.

```bash
statcpp effect cohens-h --p1 0.6 --p2 0.4
```

### odds-ratio

Specify a 2x2 contingency table with 4 values (a, b, c, d) in one column.

```bash
statcpp effect odds-ratio test/e2e/data/contingency.csv --col value
```

```text
  Odds Ratio:   6
```

### risk-ratio

Specify a 2x2 contingency table with 4 values (a, b, c, d) in one column.

```bash
statcpp effect risk-ratio test/e2e/data/contingency.csv --col value
```

```text
  Risk Ratio:   2.25
```

---

## ci - Confidence Intervals

Specify the confidence level with `--level` (default 0.95).

### mean

```bash
statcpp ci mean test/e2e/data/two_groups.csv --col group1
```

```text
  Estimate:     24.5
  Lower:        22.4522
  Upper:        26.5478
  Level:        0.95
```

### diff

```bash
statcpp ci diff test/e2e/data/two_groups.csv --col group1,group2
```

```text
  Estimate:     -6.125
  Lower:        -8.87219
  Upper:        -3.37781
  Level:        0.95
```

### prop

```bash
statcpp ci prop --successes 45 --trials 100
```

### var

```bash
statcpp ci var test/e2e/data/two_groups.csv --col group1
```

### sample-size

No CSV required.

```bash
statcpp ci sample-size --moe 0.03
```

```text
  Sample Size:  1068
```

---

## reg - Regression Analysis

### simple

`--col x,y` (first column is the predictor, last is the response).

```bash
statcpp reg simple test/e2e/data/two_groups.csv --col group1,group2
```

```text
  Intercept:    5.83333
  Slope:        1.0119
  R-squared:    0.862275
  Adj R-squared:0.839321
  Residual SE:  1.06997
  F-statistic:  37.565
  F p-value:    0.000862397

  Coefficients:
                Estimate    Std.Error   t-value     p-value
  Intercept     5.83333     4.06261     1.43586     0.201058
  Slope         1.0119      0.1651      6.12903     0.000862397
```

### multiple

`--col x1,x2,...,y` (last column is the response).

```bash
statcpp reg multiple test/e2e/data/scores.csv --col math,science,english
```

```text
  R-squared:    0.860845
  Adj R-squared:0.821086
  Residual SE:  2.99757
  F-statistic:  21.6518
  F p-value:    0.00100519

  Coefficients:
                  Estimate    Std.Error   t-value     p-value
  (Intercept)     31.3951     8.30518     3.78019     0.00689059
  math            0.764905    0.28918     2.64508     0.0331777
  science         -0.118209   0.285503    -0.414039   0.691238
```

### predict

Compute predicted values from the regression model.

```bash
statcpp reg predict test/e2e/data/two_groups.csv --col group1,group2
```

### residuals

Residual diagnostics.

```bash
statcpp reg residuals test/e2e/data/two_groups.csv --col group1,group2
```

### vif

Variance Inflation Factor (multicollinearity diagnostics).

```bash
statcpp reg vif test/e2e/data/two_groups.csv --col group1,group2
```

```text
  group1:       7.26083
  group2:       7.26083
```

---

## anova - Analysis of Variance

### oneway

```bash
statcpp anova oneway test/e2e/data/two_groups.csv --col group1,group2
```

```text
  ANOVA Table:
  Source        SS          df      MS          F           p-value
  Between       150.062     1       150.062     22.8667     0.000292342
  Within        91.875      14      6.5625      0           0
  Total         241.938     15

  F-statistic:  22.8667
  p-value:      0.000292342
  eta-squared:  0.620253
  omega-squared:0.577465
  Reject H0 (alpha=0.05)
```

### posthoc-tukey

```bash
statcpp anova posthoc-tukey test/e2e/data/scores.csv --col math,science,english
```

```text
  Tukey HSD Post-hoc Comparisons:
  Comparison      Diff        p-value     Lower       Upper       Sig?
  math vs science -1.4        0.938605    -11.626     8.82601     No
  math vs english -2.4        0.830879    -12.626     7.82601     No
  science vs english-1          0.96815     -11.226     9.22601     No
```

### posthoc-bonferroni

```bash
statcpp anova posthoc-bonferroni test/e2e/data/scores.csv --col math,science,english
```

### posthoc-scheffe

```bash
statcpp anova posthoc-scheffe test/e2e/data/scores.csv --col math,science,english
```

### eta-squared

```bash
statcpp anova eta-squared test/e2e/data/scores.csv --col math,science,english
```

```text
  eta-squared:  0.0124993
  omega-squared:-0.058509
  Cohen's f:    0.112506
```

---

## resample - Resampling

Output depends on random numbers, so results vary between runs.

### bootstrap-mean

```bash
statcpp resample bootstrap-mean test/e2e/data/two_groups.csv --col group1
```

### bootstrap-median

```bash
statcpp resample bootstrap-median test/e2e/data/two_groups.csv --col group1
```

### bootstrap-sd

```bash
statcpp resample bootstrap-sd test/e2e/data/two_groups.csv --col group1
```

### bca

BCa method (bias-corrected and accelerated bootstrap).

```bash
statcpp resample bca test/e2e/data/two_groups.csv --col group1
```

### permtest

Two columns for independent two-group test; add `--paired` for paired test.

```bash
statcpp resample permtest test/e2e/data/two_groups.csv --col group1,group2
```

### permtest-corr

```bash
statcpp resample permtest-corr test/e2e/data/two_groups.csv --col group1,group2
```

---

## ts - Time Series Analysis

### acf

Maximum lag of 20 (or data length - 1).

```bash
statcpp ts acf test/e2e/data/two_groups.csv --col group1
```

### pacf

```bash
statcpp ts pacf test/e2e/data/two_groups.csv --col group1
```

### ma

Moving average (window size = 3).

```bash
statcpp ts ma test/e2e/data/two_groups.csv --col group1
```

### ema

Exponential moving average (alpha = 0.3).

```bash
statcpp ts ema test/e2e/data/two_groups.csv --col group1
```

### diff

```bash
statcpp ts diff test/e2e/data/two_groups.csv --col group1
```

```text
  d[1]:         2
  d[2]:         2
  d[3]:         -5
  d[4]:         2
  d[5]:         2
  d[6]:         2
  d[7]:         -7
```

### mae

Two columns (actual, predicted).

```bash
statcpp ts mae test/e2e/data/forecast.csv --col actual,predicted
```

```text
  MAE:          3
```

### rmse

```bash
statcpp ts rmse test/e2e/data/forecast.csv --col actual,predicted
```

```text
  RMSE:         3.24037
```

### mape

```bash
statcpp ts mape test/e2e/data/forecast.csv --col actual,predicted
```

```text
  MAPE (%):     2.47699
```

---

## robust - Robust Statistics

### mad

```bash
statcpp robust mad test/e2e/data/two_groups.csv --col group1
```

```text
  MAD:          2
  MAD scaled:   2.9652
```

### outliers

IQR method.

```bash
statcpp robust outliers test/e2e/data/basic.csv --col value
```

```text
  Q1:           20
  Q3:           40
  IQR:          20
  Lower fence:  -10
  Upper fence:  70
  N outliers:   0
```

### outliers-zscore

Z-score method.

```bash
statcpp robust outliers-zscore test/e2e/data/basic.csv --col value
```

### outliers-modified

Modified Z-score method (MAD-based).

```bash
statcpp robust outliers-modified test/e2e/data/basic.csv --col value
```

### winsorize

Specify the trim ratio with `--trim`.

```bash
statcpp robust winsorize test/e2e/data/basic.csv --col value --trim 0.05
```

### hodges-lehmann

```bash
statcpp robust hodges-lehmann test/e2e/data/two_groups.csv --col group1
```

```text
  Hodges-Lehmann:24.5
```

### biweight

```bash
statcpp robust biweight test/e2e/data/two_groups.csv --col group1
```

```text
  Biweight midvariance:5.88694
```

---

## survival - Survival Analysis

### kaplan-meier

`--col time,event` (event: 1=event, 0=censored).

```bash
statcpp survival kaplan-meier test/e2e/data/survival.csv --col time,event
```

```text
  Kaplan-Meier Survival Estimates:
  Time      Survival    SE        At Risk   Events    Censored
  0         1           0         8         0         0
  1         0.875       0.116927  8         1         0
  3         0.729167    0.164976  6         1         0
  4         0.583333    0.185561  5         1         0
  6         0.388889    0.201269  3         1         0
  7         0.194444    0.170387  2         1         0

  Median survival: 6
```

### logrank

Specify two-group survival data with 4 columns (time1, event1, time2, event2).

```bash
statcpp survival logrank test/e2e/data/survival_two.csv --col time1,event1,time2,event2
```

```text
  Chi-square:   0.316564
  p-value:      0.57368
  df:           1
  Observed 1:   4
  Expected 1:   3.27778
  Observed 2:   4
  Expected 2:   4.72222
```

### nelson-aalen

```bash
statcpp survival nelson-aalen test/e2e/data/survival.csv --col time,event
```

```text
  Nelson-Aalen Cumulative Hazard:
  Time      Hazard      Cum.Hazard
  0         0           0
  1         0.125       0.125
  3         0.166667    0.291667
  4         0.2         0.491667
  6         0.333333    0.825
  7         0.5         1.325
```

---

## cluster - Clustering

Output may depend on random numbers (kmeans). Requires 2 or more columns.

### kmeans

Default k=3.

```bash
statcpp cluster kmeans test/e2e/data/scores.csv --col math,science,english
```

### hierarchical

```bash
statcpp cluster hierarchical test/e2e/data/scores.csv --col math,science,english
```

### silhouette

```bash
statcpp cluster silhouette test/e2e/data/scores.csv --col math,science,english
```

---

## multiple - Multiple Testing Correction

### bonferroni

```bash
statcpp multiple bonferroni test/e2e/data/pvalues.csv --col pvalue
```

```text
  Bonferroni Correction:
  Adjusted alpha:0.01

  Test    p-value     Adj. p-value  Sig?
  1       0.001       0.005         Yes
  2       0.013       0.065         No
  3       0.04        0.2           No
  4       0.06        0.3           No
  5       0.5         1             No
```

### holm

```bash
statcpp multiple holm test/e2e/data/pvalues.csv --col pvalue
```

```text
  Holm-Bonferroni Correction:
  Test    p-value     Adj. p-value  Sig?
  1       0.001       0.005         Yes
  2       0.013       0.052         No
  3       0.04        0.12          No
  4       0.06        0.12          No
  5       0.5         0.5           No
```

### bh

Benjamini-Hochberg (FDR control).

```bash
statcpp multiple bh test/e2e/data/pvalues.csv --col pvalue
```

```text
  Benjamini-Hochberg (FDR) Correction:
  Test    p-value     Adj. p-value  Sig?
  1       0.001       0.005         Yes
  2       0.013       0.0325        Yes
  3       0.04        0.0666667     No
  4       0.06        0.075         No
  5       0.5         0.5           No
```

---

## power - Power Analysis

No CSV input required. When `--n` is specified, power is calculated; when omitted, the required sample size is calculated.

### t-one

`--effect` (Cohen's d) is required.

```bash
statcpp power t-one --effect 0.5 --n 30
```

```text
  Power:        0.781908
```

```bash
statcpp power t-one --effect 0.5 --power 0.8
```

```text
  Sample size:  32
```

### t-two

Sample size ratio can be specified with `--ratio`.

```bash
statcpp power t-two --effect 0.5 --n 30
```

```text
  Power:        0.490686
```

```bash
statcpp power t-two --effect 0.5 --power 0.8
```

```text
  n1:           63
  n2:           63
```

### prop

`--p1` and `--p2` are required.

```bash
statcpp power prop --p1 0.3 --p2 0.5 --n 50
```

```text
  Power:        0.549836
```

```bash
statcpp power prop --p1 0.3 --p2 0.5 --power 0.8
```

```text
  Sample size:  93
```

---

## glm - Generalized Linear Models

### logistic

`--col x1,...,y` (y is binary 0/1).

```bash
statcpp glm logistic test/e2e/data/binary.csv --col x1,x2,y
```

```text
  Null deviance:16.3006
  Residual deviance:13.3226
  AIC:          19.3226
  Log-likelihood:-6.6613
  Iterations:   6

  Coefficients:
                  Estimate    Std.Error   z-value     p-value
  (Intercept)     0.173347    2.22534     0.0778966   0.93791
  x1              0.714642    0.580323    1.23145     0.218153
  x2              -0.796283   0.942142    -0.845183   0.398008

  Pseudo R-squared (McFadden): 0.182694
  Odds ratios: x1=0.451002 x2=2.22534
```

### poisson

`--col x1,...,y` (y is count data).

```bash
statcpp glm poisson test/e2e/data/count.csv --col x1,x2,y
```

```text
  Null deviance:9.16573
  Residual deviance:0.53496
  AIC:          34.4471
  Log-likelihood:-14.2236
  Iterations:   5

  Coefficients:
                  Estimate    Std.Error   z-value     p-value
  (Intercept)     0.422972    0.521971    0.810336    0.417747
  x1              0.0443169   0.0731394   0.605924    0.544565
  x2              0.138836    0.0561691   2.47175     0.0134455

  Incidence rate ratios: x1=1.14894 x2=8.79106e+252
```

---

## model - Model Selection

### aic

```bash
statcpp model aic test/e2e/data/scores.csv --col math,science,english
```

### cv

Cross-validation (5-fold).

```bash
statcpp model cv test/e2e/data/scores.csv --col math,science,english
```

### ridge

```bash
statcpp model ridge test/e2e/data/scores.csv --col math,science,english
```

### lasso

```bash
statcpp model lasso test/e2e/data/scores.csv --col math,science,english
```

---

## Output Modes

### JSON Output (`--json`)

#### desc

```bash
statcpp desc summary test/e2e/data/basic.csv --col value --json
```

```json
{
    "command": "desc.summary",
    "input": {
        "column": "value",
        "n": 5
    },
    "result": {
        "Count": 5.0,
        "Kurtosis": -1.2000000000000002,
        "Max": 50.0,
        "Mean": 30.0,
        "Median": 30.0,
        "Min": 10.0,
        "Q1": 20.0,
        "Q3": 40.0,
        "Skewness": 0.0,
        "Std Dev": 15.811388300841896
    }
}
```

#### test

```bash
statcpp test t test/e2e/data/two_groups.csv --col group1,group2 --json
```

```json
{
    "command": "test.t",
    "input": {
        "columns": [
            "group1",
            "group2"
        ],
        "n1": 8,
        "n2": 8
    },
    "result": {
        "Statistic": -4.781910357447813,
        "alpha": 0.05,
        "df": 14.0,
        "p-value": 0.00029234175805803453
    }
}
```

#### corr

```bash
statcpp corr pearson test/e2e/data/two_groups.csv --col group1,group2 --json
```

```json
{
    "command": "corr.pearson",
    "input": {
        "columns": [
            "group1",
            "group2"
        ],
        "n": 8
    },
    "result": {
        "r": 0.9285874942379881
    }
}
```

#### effect

```bash
statcpp effect cohens-d test/e2e/data/two_groups.csv --col group1,group2 --json
```

```json
{
    "command": "effect.cohens-d",
    "input": {
        "columns": [
            "group1",
            "group2"
        ],
        "n1": 8,
        "n2": 8
    },
    "result": {
        "Cohen's d": -2.3909551787239063
    }
}
```

```bash
statcpp effect cohens-h --p1 0.6 --p2 0.4 --json
```

```json
{
    "command": "effect.cohens-h",
    "input": {
        "p1": 0.6,
        "p2": 0.4
    },
    "result": {
        "Cohen's h": 0.40271584158066154
    }
}
```

#### ci

```bash
statcpp ci mean test/e2e/data/two_groups.csv --col group1 --json
```

```json
{
    "command": "ci.mean",
    "input": {
        "column": "group1",
        "n": 8
    },
    "result": {
        "Estimate": 24.5,
        "Level": 0.95,
        "Lower": 22.452175328624975,
        "Upper": 26.547824671375025
    }
}
```

```bash
statcpp ci prop --successes 45 --trials 100 --json
```

```json
{
    "command": "ci.prop",
    "input": {
        "successes": 45,
        "trials": 100
    },
    "result": {
        "Estimate": 0.45,
        "Level": 0.95,
        "Lower": 0.3561453797236467,
        "Upper": 0.5475539701027973
    }
}
```

```bash
statcpp ci sample-size --moe 0.03 --json
```

```json
{
    "command": "ci.sample-size",
    "input": {
        "level": 0.95,
        "moe": 0.03
    },
    "result": {
        "Sample Size": 1068.0
    }
}
```

#### reg

```bash
statcpp reg simple test/e2e/data/two_groups.csv --col group1,group2 --json
```

```json
{
    "command": "reg.simple",
    "input": {
        "n": 8,
        "x": "group1",
        "y": "group2"
    },
    "result": {
        "Adj R-squared": 0.8393205235310498,
        "F p-value": 0.0008623966761827084,
        "F-statistic": 37.56499133448873,
        "Intercept": 5.833333333333334,
        "R-squared": 0.8622747344551855,
        "Residual SE": 1.0699725556486344,
        "Slope": 1.0119047619047619
    }
}
```

#### anova

```bash
statcpp anova oneway test/e2e/data/two_groups.csv --col group1,group2 --json
```

```json
{
    "command": "anova.oneway",
    "input": {
        "columns": [
            "group1",
            "group2"
        ],
        "k": 2,
        "n": 16
    },
    "result": {
        "F-statistic": 22.866666666666667,
        "eta-squared": 0.620253164556962,
        "omega-squared": 0.5774647887323944,
        "p-value": 0.00029234175805814555
    }
}
```

#### ts

```bash
statcpp ts diff test/e2e/data/two_groups.csv --col group1 --json
```

```json
{
    "command": "ts.diff",
    "input": {
        "column": "group1",
        "n": 8,
        "order": 1
    },
    "result": {
        "d[1]": 2.0,
        "d[2]": 2.0,
        "d[3]": -5.0,
        "d[4]": 2.0,
        "d[5]": 2.0,
        "d[6]": 2.0,
        "d[7]": -7.0
    }
}
```

#### robust

```bash
statcpp robust mad test/e2e/data/two_groups.csv --col group1 --json
```

```json
{
    "command": "robust.mad",
    "input": {
        "column": "group1",
        "n": 8
    },
    "result": {
        "MAD": 2.0,
        "MAD scaled": 2.9652
    }
}
```

#### survival

```bash
statcpp survival kaplan-meier test/e2e/data/survival.csv --col time,event --json
```

```json
{
    "command": "survival.kaplan-meier",
    "input": {
        "event": "event",
        "n": 8,
        "time": "time"
    },
    "result": {
        "Median survival": 6.0,
        "N events": 5.0
    }
}
```

#### multiple

```bash
statcpp multiple bonferroni test/e2e/data/pvalues.csv --col pvalue --json
```

```json
{
    "command": "multiple.bonferroni",
    "input": {
        "alpha": 0.05,
        "column": "pvalue",
        "n_tests": 5
    },
    "result": {
        "Adjusted alpha": 0.01,
        "p1 adjusted": 0.005,
        "p2 adjusted": 0.065,
        "p3 adjusted": 0.2,
        "p4 adjusted": 0.3,
        "p5 adjusted": 1.0
    }
}
```

#### power

```bash
statcpp power t-one --effect 0.5 --n 30 --json
```

```json
{
    "command": "power.t-one",
    "input": {
        "alpha": 0.05,
        "alternative": "two-sided",
        "effect": 0.5,
        "n": 30
    },
    "result": {
        "Power": 0.781908063920346
    }
}
```

### Quiet Output (`--quiet`)

```bash
statcpp desc mean test/e2e/data/basic.csv --col value --quiet
```

```text
30
```

### stdin Pipe

```bash
printf '1\n2\n3\n4\n5\n' | statcpp desc mean --noheader --col 1
```

```text
  Mean:         3
```

---

## Shortcuts

| Shortcut | Expands to |
| --- | --- |
| `mean` | `desc mean` |
| `median` | `desc median` |
| `mode` | `desc mode` |
| `sd` | `desc sd` |
| `var` | `desc var` |
| `summary` | `desc summary` |
| `range` | `desc range` |
| `iqr` | `desc iqr` |
| `cv` | `desc cv` |
| `skewness` | `desc skewness` |
| `kurtosis` | `desc kurtosis` |
| `quartiles` | `desc quartiles` |
| `gmean` | `desc gmean` |
| `hmean` | `desc hmean` |
| `ttest` | `test t` |
| `pearson` | `corr pearson` |
| `spearman` | `corr spearman` |
| `kendall` | `corr kendall` |

For detailed options, see the [Command Reference](commands.md).
