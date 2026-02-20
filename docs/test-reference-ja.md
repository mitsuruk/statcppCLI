# statcpp CLI テストリファレンス

基本書式:

```text
statcpp <category> <command> [options] [file]
statcpp <shortcut> [options] [file]
```

ファイル引数が不要な場合は stdin から読み込みます。
`--row` を指定すると、横並びのデータ（カンマ/スペース区切り）を1列のデータとして処理します。

```bash
# カンマ区切り
echo "1,2,3,4,5" | statcpp desc mean --noheader --col 1 --row
#   Mean:         3

# スペース区切り
echo "1 2 3 4 5" | statcpp desc mean --noheader --col 1 --row
#   Mean:         3
```

本ドキュメントの実行例はすべて `test/e2e/data/` のファイルに基づいています。
`docs/run_reference.sh` で全例を実行し、出力を確認できます（実行結果: `docs/output.txt`）。

### テストデータ

| ファイル | 内容 |
| --- | --- |
| `basic.csv` | name, value (10,20,30,40,50), score |
| `two_groups.csv` | group1 (23,25,27,22,24,26,28,21), group2 (28,30,32,29,31,33,35,27) |
| `scores.csv` | math, science, english (10人分、3科目) |
| `forecast.csv` | actual, predicted (8件の予測精度データ) |
| `survival.csv` | time (1-8), event (0/1) |
| `survival_two.csv` | time1, event1, time2, event2 (2群生存データ、各6件) |
| `pvalues.csv` | pvalue (0.001, 0.013, 0.04, 0.06, 0.50) |
| `contingency.csv` | value (30,10,20,40) — 2x2分割表 (a,b,c,d) |
| `binary.csv` | x1, x2, y (0/1二値応答、12件) |
| `count.csv` | x1, x2, y (カウントデータ応答、8件) |
| `missing.csv` | id, value (NA/空セルを含む) |
| `quoted.csv` | name, city, value (RFC 4180 引用符付きフィールド) |
| `noheader.csv` | ヘッダなし 3列×3行 (10,20,30 / 40,50,60 / 70,80,90) |

---

## desc - 記述統計

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

データは内部で自動ソートされます（`--presorted` で省略可）。

```bash
statcpp desc median test/e2e/data/basic.csv --col value
```

```text
  Median:       30
```

### mode

複数ある場合はすべて表示。

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

デフォルトは標本分散 (ddof=1)。`--population` で母分散。

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

変動係数 (CV = SD / Mean)。

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

超過尖度。正規分布で 0。

```bash
statcpp desc kurtosis test/e2e/data/basic.csv --col value
```

```text
  Kurtosis:     -1.2
```

### percentile

`--p` で位置指定 (0.0-1.0)。

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

幾何平均。

```bash
statcpp desc gmean test/e2e/data/basic.csv --col value
```

```text
  Geometric Mean:26.0517
```

### hmean

調和平均。

```bash
statcpp desc hmean test/e2e/data/basic.csv --col value
```

```text
  Harmonic Mean:21.8978
```

### trimmed-mean

`--trim` でトリム比率指定（デフォルト 0.1 = 上下 10%）。

```bash
statcpp desc trimmed-mean test/e2e/data/basic.csv --col value
```

```text
  Trimmed Mean: 30
```

---

## test - 統計検定

`--alternative` (two-sided / less / greater) と `--alpha` (デフォルト 0.05) を指定可能。

### t

列数で自動判定。

```bash
# 1 標本 t 検定
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
# 2 標本 t 検定（独立）
statcpp test t test/e2e/data/two_groups.csv --col group1,group2
```

```text
  Statistic:    -4.78191
  df:           14
  p-value:      0.000292342
  alpha:        0.05
  Reject H0
```

対応あり: `--paired` を追加。

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

母標準偏差 `--sigma` が必要。

```bash
statcpp test z test/e2e/data/two_groups.csv --col group1 --mu0 25 --sigma 3
```

### f

F 検定（等分散の検定）。

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

Shapiro-Wilk 正規性検定。

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

Kolmogorov-Smirnov 正規性検定。

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

1 列で 1 標本、2 列で対応あり。

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

1 列で一様分布適合度検定、2 列で観測/期待度数比較。

```bash
statcpp test chisq test/e2e/data/basic.csv --col value
```

---

## corr - 相関・共分散

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

3 列以上で相関行列を表示。

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

## effect - 効果量

### cohens-d

```bash
statcpp effect cohens-d test/e2e/data/two_groups.csv --col group1,group2
```

```text
  Cohen's d:    -2.39096
  Interpretation: large
```

1 標本の場合は `--mu0` を指定。

### hedges-g

小標本補正付き。

```bash
statcpp effect hedges-g test/e2e/data/two_groups.csv --col group1,group2
```

```text
  Hedges' g:    -2.26054
  Interpretation: large
```

### glass-delta

対照群の SD で標準化。

```bash
statcpp effect glass-delta test/e2e/data/two_groups.csv --col group1,group2
```

### cohens-h

比率の効果量。`--p1`, `--p2` で比率指定。

```bash
statcpp effect cohens-h --p1 0.6 --p2 0.4
```

### odds-ratio

1 列に 4 値 (a, b, c, d) で 2x2 分割表を指定。

```bash
statcpp effect odds-ratio test/e2e/data/contingency.csv --col value
```

```text
  Odds Ratio:   6
```

### risk-ratio

1 列に 4 値 (a, b, c, d) で 2x2 分割表を指定。

```bash
statcpp effect risk-ratio test/e2e/data/contingency.csv --col value
```

```text
  Risk Ratio:   2.25
```

---

## ci - 信頼区間

`--level` で信頼水準を指定（デフォルト 0.95）。

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

CSV 不要。

```bash
statcpp ci sample-size --moe 0.03
```

```text
  Sample Size:  1068
```

---

## reg - 回帰分析

### simple

`--col x,y`（最初が説明変数、最後が目的変数）。

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

`--col x1,x2,...,y`（最後の列が目的変数）。

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

回帰モデルで予測値を計算。

```bash
statcpp reg predict test/e2e/data/two_groups.csv --col group1,group2
```

### residuals

残差診断。

```bash
statcpp reg residuals test/e2e/data/two_groups.csv --col group1,group2
```

### vif

分散膨張係数（多重共線性の診断）。

```bash
statcpp reg vif test/e2e/data/two_groups.csv --col group1,group2
```

```text
  group1:       7.26083
  group2:       7.26083
```

---

## anova - 分散分析

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

## resample - リサンプリング

出力は乱数に依存するため、実行ごとに結果が変わります。

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

BCa 法（バイアス補正・加速ブートストラップ）。

```bash
statcpp resample bca test/e2e/data/two_groups.csv --col group1
```

### permtest

2 列で独立 2 群、`--paired` で対応あり。

```bash
statcpp resample permtest test/e2e/data/two_groups.csv --col group1,group2
```

### permtest-corr

```bash
statcpp resample permtest-corr test/e2e/data/two_groups.csv --col group1,group2
```

---

## ts - 時系列分析

### acf

最大ラグ 20（またはデータ長 - 1）。

```bash
statcpp ts acf test/e2e/data/two_groups.csv --col group1
```

### pacf

```bash
statcpp ts pacf test/e2e/data/two_groups.csv --col group1
```

### ma

移動平均（ウィンドウサイズ = 3）。

```bash
statcpp ts ma test/e2e/data/two_groups.csv --col group1
```

### ema

指数移動平均（alpha = 0.3）。

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

2 列 (実測, 予測)。

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

## robust - ロバスト統計

### mad

```bash
statcpp robust mad test/e2e/data/two_groups.csv --col group1
```

```text
  MAD:          2
  MAD scaled:   2.9652
```

### outliers

IQR 法。

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

Z-score 法。

```bash
statcpp robust outliers-zscore test/e2e/data/basic.csv --col value
```

### outliers-modified

修正 Z-score 法（MAD ベース）。

```bash
statcpp robust outliers-modified test/e2e/data/basic.csv --col value
```

### winsorize

`--trim` でトリム比率指定。

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

## survival - 生存分析

### kaplan-meier

`--col time,event`（event: 1=イベント, 0=打ち切り）。

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

4 列 (time1, event1, time2, event2) で 2 群の生存データを指定。

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

## cluster - クラスタリング

出力は乱数に依存する場合があります（kmeans）。2 列以上必須。

### kmeans

デフォルト k=3。

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

## multiple - 多重検定補正

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

Benjamini-Hochberg（FDR 制御）。

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

## power - 検出力分析

CSV 入力不要。`--n` 指定で検出力を計算、省略で必要サンプルサイズを計算。

### t-one

`--effect` (Cohen's d) 必須。

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

`--ratio` でサンプルサイズ比指定可。

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

`--p1`, `--p2` 必須。

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

## glm - 一般化線形モデル

### logistic

`--col x1,...,y`（y は 0/1 二値）。

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

`--col x1,...,y`（y はカウントデータ）。

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

## model - モデル選択

### aic

```bash
statcpp model aic test/e2e/data/scores.csv --col math,science,english
```

### cv

交差検証（5-fold）。

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

## 出力モード

### JSON 出力 (`--json`)

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

### Quiet 出力 (`--quiet`)

```bash
statcpp desc mean test/e2e/data/basic.csv --col value --quiet
```

```text
30
```

### stdin パイプ

```bash
printf '1\n2\n3\n4\n5\n' | statcpp desc mean --noheader --col 1
```

```text
  Mean:         3
```

---

## ショートカット

| ショートカット | 展開先 |
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

オプションの詳細は [コマンドリファレンス](commands-ja.md) を参照してください。
