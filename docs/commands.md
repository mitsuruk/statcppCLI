# statcpp CLI Command Reference

Basic syntax:

```text
statcpp <category> <command> [options] [file]
statcpp <shortcut> [options] [file]
```

Input methods:

```bash
# 1. Specify a CSV file
statcpp desc mean test/e2e/data/basic.csv --col value

# 2. Pipe from stdin (vertical, one value per line)
cat test/e2e/data/basic.csv | statcpp desc mean --col value

# 3. --row: Pass inline values directly (comma/space-separated)
echo '1,2,3,4,5' | statcpp desc mean --noheader --col 1 --row
echo '1 2 3 4 5' | statcpp desc mean --noheader --col 1 --row
```

| Input Method | File Argument | Additional Flags | Data Format |
| --- | --- | --- | --- |
| CSV file | Required | Not required | Standard CSV/TSV |
| stdin pipe | Not required | Not required | Vertical (one value per line) |
| `--row` | Not required | `--noheader --col 1 --row` | Horizontal (comma/space-separated) |

---

## Common Options

Options available for all commands.

### Input Control

| Flag | Default | Description |
| --- | --- | --- |
| `--delimiter <char>` | Auto-detect | CSV delimiter character |
| `--header` / `--noheader` | `--header` | Whether a header row is present |
| `--na <strings>` | `NA,NaN,nan,N/A,n/a` | Strings treated as missing values |
| `--skip_na` / `--noskip_na` | `--skip_na` | Exclude missing values from calculations |
| `--fail_na` | false | Exit with error if missing values are present |
| `--presorted` | false | Data is already sorted (skip sorting) |
| `--row` | false | Treat horizontal data as a single column (comma/space-separated) |

### Output Control

| Flag | Default | Description |
| --- | --- | --- |
| `--json` | false | Output in JSON format |
| `--quiet` | false | Output numbers only (for piping) |

### Statistics Common

| Flag | Default | Description |
| --- | --- | --- |
| `--alpha` | 0.05 | Significance level |
| `--level` | 0.95 | Confidence level |
| `--population` | false | Population statistics (ddof=0) |

---

## desc - Descriptive Statistics

Computes descriptive statistics for a single column of numeric data.

| Subcommand | `--col` | Additional Options | Description |
| --- | --- | --- | --- |
| `summary` | 1 column | `--presorted` | Summary statistics (count, mean, SD, five-number summary, skewness, kurtosis) |
| `mean` | 1 column | | Arithmetic mean |
| `median` | 1 column | `--presorted` | Median |
| `mode` | 1 column | | Mode (displays all if multiple) |
| `var` | 1 column | `--population` | Variance (default: sample variance) |
| `sd` | 1 column | `--population` | Standard deviation |
| `range` | 1 column | | Range (max - min) |
| `iqr` | 1 column | `--presorted` | Interquartile range |
| `cv` | 1 column | | Coefficient of variation (SD / Mean) |
| `skewness` | 1 column | | Skewness |
| `kurtosis` | 1 column | | Excess kurtosis (0 for normal distribution) |
| `percentile` | 1 column | `--p` (required), `--presorted` | Percentile value |
| `quartiles` | 1 column | `--presorted` | Quartiles (Q1, Q2, Q3) |
| `five-number` | 1 column | `--presorted` | Five-number summary |
| `gmean` | 1 column | | Geometric mean |
| `hmean` | 1 column | | Harmonic mean |
| `trimmed-mean` | 1 column | `--trim` (default: 0.1) | Trimmed mean (removes upper and lower extremes) |

```bash
statcpp desc summary test/e2e/data/basic.csv --col value
statcpp desc mean test/e2e/data/basic.csv --col value
statcpp desc median test/e2e/data/basic.csv --col value
statcpp desc mode test/e2e/data/basic.csv --col value
statcpp desc var test/e2e/data/basic.csv --col score --population
statcpp desc sd test/e2e/data/basic.csv --col score
statcpp desc range test/e2e/data/basic.csv --col value
statcpp desc iqr test/e2e/data/basic.csv --col value
statcpp desc cv test/e2e/data/basic.csv --col value
statcpp desc skewness test/e2e/data/basic.csv --col value
statcpp desc kurtosis test/e2e/data/basic.csv --col value
statcpp desc percentile test/e2e/data/basic.csv --col value --p 0.95
statcpp desc quartiles test/e2e/data/basic.csv --col value
statcpp desc five-number test/e2e/data/basic.csv --col value
statcpp desc gmean test/e2e/data/basic.csv --col value
statcpp desc hmean test/e2e/data/basic.csv --col value
statcpp desc trimmed-mean test/e2e/data/basic.csv --col value --trim 0.1
```

---

## test - Statistical Tests

You can specify `--alternative` (`two-sided` / `less` / `greater`) and `--alpha` (default 0.05).

| Subcommand | `--col` | Additional Required Options | Additional Optional Options | Description |
| --- | --- | --- | --- | --- |
| `t` | 1 or 2 columns | | `--mu0`, `--paired`, `--alternative` | t-test (auto-detected by number of columns: 1 column = one-sample, 2 columns = two-sample) |
| `welch` | 2 columns | | `--alternative` | Welch's t-test |
| `z` | 1 column | `--sigma` | `--mu0`, `--alternative` | z-test (requires known population standard deviation) |
| `f` | 2 columns | | `--alternative` | F-test (test for equality of variances) |
| `shapiro` | 1 column | | | Shapiro-Wilk normality test |
| `ks` | 1 column | | | Kolmogorov-Smirnov normality test |
| `mann-whitney` | 2 columns | | `--alternative` | Mann-Whitney U test |
| `wilcoxon` | 1 or 2 columns | | `--mu0`, `--alternative` | Wilcoxon signed-rank test (1 column = one-sample, 2 columns = paired) |
| `kruskal` | 2+ columns | | | Kruskal-Wallis test |
| `levene` | 2+ columns | | | Levene's test (homogeneity of variances) |
| `bartlett` | 2+ columns | | | Bartlett's test (homogeneity of variances) |
| `chisq` | 1 or 2 columns | | | Chi-squared test (1 column = goodness-of-fit, 2 columns = observed/expected) |

```bash
statcpp test t test/e2e/data/two_groups.csv --col group1 --mu0 25
statcpp test t test/e2e/data/two_groups.csv --col group1,group2
statcpp test t test/e2e/data/two_groups.csv --col group1,group2 --paired
statcpp test welch test/e2e/data/two_groups.csv --col group1,group2
statcpp test z test/e2e/data/two_groups.csv --col group1 --mu0 25 --sigma 3
statcpp test f test/e2e/data/two_groups.csv --col group1,group2
statcpp test shapiro test/e2e/data/two_groups.csv --col group1
statcpp test ks test/e2e/data/two_groups.csv --col group1
statcpp test mann-whitney test/e2e/data/two_groups.csv --col group1,group2
statcpp test wilcoxon test/e2e/data/two_groups.csv --col group1 --mu0 25
statcpp test wilcoxon test/e2e/data/two_groups.csv --col group1,group2
statcpp test kruskal test/e2e/data/scores.csv --col math,science,english
statcpp test levene test/e2e/data/scores.csv --col math,science,english
statcpp test bartlett test/e2e/data/scores.csv --col math,science,english
statcpp test chisq test/e2e/data/basic.csv --col value
statcpp test chisq test/e2e/data/two_groups.csv --col group1,group2
```

---

## corr - Correlation & Covariance

| Subcommand | `--col` | Additional Options | Description |
| --- | --- | --- | --- |
| `pearson` | 2 columns | | Pearson correlation coefficient |
| `spearman` | 2 columns | | Spearman rank correlation coefficient |
| `kendall` | 2 columns | | Kendall rank correlation coefficient |
| `cov` | 2 columns | `--population` | Covariance |
| `matrix` | 2+ columns | | Correlation matrix |

```bash
statcpp corr pearson test/e2e/data/two_groups.csv --col group1,group2
statcpp corr spearman test/e2e/data/two_groups.csv --col group1,group2
statcpp corr kendall test/e2e/data/two_groups.csv --col group1,group2
statcpp corr cov test/e2e/data/two_groups.csv --col group1,group2
statcpp corr matrix test/e2e/data/scores.csv --col math,science,english
```

---

## effect - Effect Size

| Subcommand | `--col` | Additional Required Options | Additional Optional Options | Description |
| --- | --- | --- | --- | --- |
| `cohens-d` | 1 or 2 columns | | `--mu0` | Cohen's d (1 column = one-sample, 2 columns = two-sample) |
| `hedges-g` | 1 or 2 columns | | `--mu0` | Hedges' g (with small-sample correction) |
| `glass-delta` | 2 columns | | | Glass's delta (standardized by control group SD) |
| `cohens-h` | Not required (no CSV input) | `--p1`, `--p2` | | Cohen's h (effect size for proportions) |
| `odds-ratio` | 1 column (4 values) | | | Odds ratio (4 values: a, b, c, d) |
| `risk-ratio` | 1 column (4 values) | | | Risk ratio (4 values: a, b, c, d) |

```bash
statcpp effect cohens-d test/e2e/data/two_groups.csv --col group1 --mu0 25
statcpp effect cohens-d test/e2e/data/two_groups.csv --col group1,group2
statcpp effect hedges-g test/e2e/data/two_groups.csv --col group1,group2
statcpp effect glass-delta test/e2e/data/two_groups.csv --col group1,group2
statcpp effect cohens-h --p1 0.6 --p2 0.4
statcpp effect odds-ratio test/e2e/data/contingency.csv --col value
statcpp effect risk-ratio test/e2e/data/contingency.csv --col value
```

---

## ci - Confidence Intervals

Specify the confidence level with `--level` (default 0.95). `ci prop` and `ci sample-size` do not require CSV input.

| Subcommand | `--col` | Additional Required Options | Additional Optional Options | Description |
| --- | --- | --- | --- | --- |
| `mean` | 1 column | | `--sigma` | Confidence interval for the mean (z-based if `--sigma` is specified, t-based otherwise) |
| `diff` | 2 columns | | | Confidence interval for the difference of means |
| `prop` | Not required | `--successes`, `--trials` | | Confidence interval for a proportion (Wilson) |
| `var` | 1 column | | | Confidence interval for the variance |
| `sample-size` | Not required | `--moe` | `--sigma` | Required sample size (for means if `--sigma` is specified, for proportions otherwise) |

```bash
statcpp ci mean test/e2e/data/two_groups.csv --col group1
statcpp ci mean test/e2e/data/two_groups.csv --col group1 --level 0.99
statcpp ci mean test/e2e/data/two_groups.csv --col group1 --sigma 3
statcpp ci diff test/e2e/data/two_groups.csv --col group1,group2
statcpp ci prop --successes 45 --trials 100
statcpp ci var test/e2e/data/two_groups.csv --col group1
statcpp ci sample-size --moe 0.03
statcpp ci sample-size --moe 5 --sigma 20
```

---

## reg - Regression Analysis

The last column in `--col` is the response variable; the remaining columns are predictor variables.

| Subcommand | `--col` | Description |
| --- | --- | --- |
| `simple` | 2 columns (x, y) | Simple linear regression |
| `multiple` | 3+ columns (x1,...,xp, y) | Multiple linear regression |
| `predict` | 2 columns (x, y) | Predicted values |
| `residuals` | 2 columns (x, y) | Residual diagnostics |
| `vif` | 2+ columns (predictors only) | Variance inflation factor (multicollinearity diagnostics) |

```bash
statcpp reg simple test/e2e/data/two_groups.csv --col group1,group2
statcpp reg multiple test/e2e/data/scores.csv --col math,science,english
statcpp reg predict test/e2e/data/two_groups.csv --col group1,group2
statcpp reg residuals test/e2e/data/two_groups.csv --col group1,group2
statcpp reg vif test/e2e/data/two_groups.csv --col group1,group2
```

---

## anova - Analysis of Variance

Each column is treated as one group.

| Subcommand | `--col` | Description |
| --- | --- | --- |
| `oneway` | 2+ columns | One-way ANOVA |
| `posthoc-tukey` | 2+ columns | Tukey HSD post hoc test |
| `posthoc-bonferroni` | 2+ columns | Bonferroni post hoc test |
| `posthoc-scheffe` | 2+ columns | Scheffe post hoc test |
| `eta-squared` | 2+ columns | Effect size (eta-squared, omega-squared, Cohen's f) |

```bash
statcpp anova oneway test/e2e/data/scores.csv --col math,science,english
statcpp anova posthoc-tukey test/e2e/data/scores.csv --col math,science,english
statcpp anova posthoc-bonferroni test/e2e/data/scores.csv --col math,science,english
statcpp anova posthoc-scheffe test/e2e/data/scores.csv --col math,science,english
statcpp anova eta-squared test/e2e/data/scores.csv --col math,science,english
```

---

## resample - Resampling

Output depends on random numbers, so results will vary between runs.

| Subcommand | `--col` | Additional Options | Description |
| --- | --- | --- | --- |
| `bootstrap-mean` | 1 column | | Bootstrap confidence interval for the mean |
| `bootstrap-median` | 1 column | | Bootstrap confidence interval for the median |
| `bootstrap-sd` | 1 column | | Bootstrap confidence interval for the standard deviation |
| `bca` | 1 column | | BCa method (bias-corrected and accelerated bootstrap) |
| `permtest` | 2 columns | `--paired` | Permutation test (independent two-sample / paired) |
| `permtest-corr` | 2 columns | | Permutation test for correlation |

```bash
statcpp resample bootstrap-mean test/e2e/data/basic.csv --col value
statcpp resample bootstrap-median test/e2e/data/basic.csv --col value
statcpp resample bootstrap-sd test/e2e/data/basic.csv --col value
statcpp resample bca test/e2e/data/basic.csv --col value
statcpp resample permtest test/e2e/data/two_groups.csv --col group1,group2
statcpp resample permtest test/e2e/data/two_groups.csv --col group1,group2 --paired
statcpp resample permtest-corr test/e2e/data/two_groups.csv --col group1,group2
```

---

## ts - Time Series Analysis

| Subcommand | `--col` | Description |
| --- | --- | --- |
| `acf` | 1 column | Autocorrelation function (max lag 20) |
| `pacf` | 1 column | Partial autocorrelation function |
| `ma` | 1 column | Moving average (window = 3) |
| `ema` | 1 column | Exponential moving average (alpha = 0.3) |
| `diff` | 1 column | Differencing (first-order) |
| `mae` | 2 columns (actual, predicted) | Mean absolute error |
| `rmse` | 2 columns (actual, predicted) | Root mean squared error |
| `mape` | 2 columns (actual, predicted) | Mean absolute percentage error |

```bash
statcpp ts acf test/e2e/data/two_groups.csv --col group1
statcpp ts pacf test/e2e/data/two_groups.csv --col group1
statcpp ts ma test/e2e/data/two_groups.csv --col group1
statcpp ts ema test/e2e/data/two_groups.csv --col group1
statcpp ts diff test/e2e/data/two_groups.csv --col group1
statcpp ts mae test/e2e/data/forecast.csv --col actual,predicted
statcpp ts rmse test/e2e/data/forecast.csv --col actual,predicted
statcpp ts mape test/e2e/data/forecast.csv --col actual,predicted
```

---

## robust - Robust Statistics

| Subcommand | `--col` | Additional Options | Description |
| --- | --- | --- | --- |
| `mad` | 1 column | | Median absolute deviation (MAD) |
| `outliers` | 1 column | | Outlier detection (IQR method) |
| `outliers-zscore` | 1 column | | Outlier detection (Z-score method) |
| `outliers-modified` | 1 column | | Outlier detection (modified Z-score / MAD-based) |
| `winsorize` | 1 column | `--trim` (default: 0.1) | Winsorization |
| `hodges-lehmann` | 1 column | | Hodges-Lehmann estimator |
| `biweight` | 1 column | | Biweight midvariance |

```bash
statcpp robust mad test/e2e/data/two_groups.csv --col group1
statcpp robust outliers test/e2e/data/basic.csv --col value
statcpp robust outliers-zscore test/e2e/data/basic.csv --col value
statcpp robust outliers-modified test/e2e/data/basic.csv --col value
statcpp robust winsorize test/e2e/data/basic.csv --col value --trim 0.05
statcpp robust hodges-lehmann test/e2e/data/two_groups.csv --col group1
statcpp robust biweight test/e2e/data/two_groups.csv --col group1
```

---

## survival - Survival Analysis

Event column: 1 = event occurred, 0 = censored.

| Subcommand | `--col` | Description |
| --- | --- | --- |
| `kaplan-meier` | 2 columns (time, event) | Kaplan-Meier survival curve |
| `logrank` | 4 columns (time1, event1, time2, event2) | Log-rank test (two-group comparison) |
| `nelson-aalen` | 2 columns (time, event) | Nelson-Aalen cumulative hazard |

```bash
statcpp survival kaplan-meier test/e2e/data/survival.csv --col time,event
statcpp survival logrank test/e2e/data/survival_two.csv --col time1,event1,time2,event2
statcpp survival nelson-aalen test/e2e/data/survival.csv --col time,event
```

---

## cluster - Clustering

Requires 2 or more columns of numeric data. Output may depend on random numbers.

| Subcommand | `--col` | Description |
| --- | --- | --- |
| `kmeans` | 2+ columns | k-means clustering (k = 3) |
| `hierarchical` | 2+ columns | Hierarchical clustering |
| `silhouette` | 2+ columns | Silhouette analysis (k = 3) |

```bash
statcpp cluster kmeans test/e2e/data/scores.csv --col math,science,english
statcpp cluster hierarchical test/e2e/data/scores.csv --col math,science,english
statcpp cluster silhouette test/e2e/data/scores.csv --col math,science,english
```

---

## multiple - Multiple Testing Correction

Specify a column of p-values.

| Subcommand | `--col` | Description |
| --- | --- | --- |
| `bonferroni` | 1 column | Bonferroni correction |
| `holm` | 1 column | Holm-Bonferroni correction |
| `bh` | 1 column | Benjamini-Hochberg (FDR) correction |

```bash
statcpp multiple bonferroni test/e2e/data/pvalues.csv --col pvalue
statcpp multiple holm test/e2e/data/pvalues.csv --col pvalue
statcpp multiple bh test/e2e/data/pvalues.csv --col pvalue
```

---

## power - Power Analysis

No CSV input required. If `--n` is specified, power is calculated; if omitted, the required sample size is calculated.

| Subcommand | `--col` | Required Options | Optional Options | Description |
| --- | --- | --- | --- | --- |
| `t-one` | Not required | `--effect` | `--n`, `--power` (default: 0.8), `--alternative` | Power for one-sample t-test |
| `t-two` | Not required | `--effect` | `--n`, `--power`, `--ratio` (default: 1.0), `--alternative` | Power for two-sample t-test |
| `prop` | Not required | `--p1`, `--p2` | `--n`, `--power`, `--alternative` | Power for proportion test |

```bash
statcpp power t-one --effect 0.5 --n 30
statcpp power t-one --effect 0.5 --power 0.8
statcpp power t-two --effect 0.5 --power 0.8 --ratio 2.0
statcpp power prop --p1 0.3 --p2 0.5 --n 50
```

---

## glm - Generalized Linear Models

The last column in `--col` is the response variable.

| Subcommand | `--col` | Description |
| --- | --- | --- |
| `logistic` | 2+ columns (x1,...,xp, y) | Logistic regression (y is binary 0/1) |
| `poisson` | 2+ columns (x1,...,xp, y) | Poisson regression (y is count data) |

```bash
statcpp glm logistic test/e2e/data/binary.csv --col x1,x2,y
statcpp glm poisson test/e2e/data/count.csv --col x1,x2,y
```

---

## model - Model Selection

The last column in `--col` is the response variable. Requires 3 or more columns (at least 2 predictor variables).

| Subcommand | `--col` | Description |
| --- | --- | --- |
| `aic` | 3+ columns | Model comparison by AIC |
| `cv` | 3+ columns | Cross-validation (5-fold) |
| `ridge` | 3+ columns | Ridge regression (lambda = 1.0) |
| `lasso` | 3+ columns | LASSO regression (lambda = 1.0) |

```bash
statcpp model aic test/e2e/data/scores.csv --col math,science,english
statcpp model cv test/e2e/data/scores.csv --col math,science,english
statcpp model ridge test/e2e/data/scores.csv --col math,science,english
statcpp model lasso test/e2e/data/scores.csv --col math,science,english
```

---

## Shortcuts

Frequently used commands can be entered without the category. If the first argument matches a shortcut name, it is automatically expanded to the corresponding category and command.

```bash
# The following two are equivalent
statcpp mean data.csv --col value
statcpp desc mean data.csv --col value
```

| Shortcut | Expands To |
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
