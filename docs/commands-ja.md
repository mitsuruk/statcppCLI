# statcpp CLI コマンドリファレンス

基本書式:

```text
statcpp <category> <command> [options] [file]
statcpp <shortcut> [options] [file]
```

入力方法:

```bash
# 1. CSV ファイルを指定
statcpp desc mean test/e2e/data/basic.csv --col value

# 2. stdin からパイプ（縦方向・1行1値）
cat test/e2e/data/basic.csv | statcpp desc mean --col value

# 3. --row: 横並びの値を直接渡す（カンマ/スペース区切り）
echo '1,2,3,4,5' | statcpp desc mean --noheader --col 1 --row
echo '1 2 3 4 5' | statcpp desc mean --noheader --col 1 --row
```

| 入力方法 | ファイル引数 | 追加フラグ | データ形式 |
| --- | --- | --- | --- |
| CSV ファイル | 必要 | 不要 | 通常の CSV/TSV |
| stdin パイプ | 不要 | 不要 | 縦方向（1行1値） |
| `--row` | 不要 | `--noheader --col 1 --row` | 横並び（カンマ/スペース区切り） |

---

## 共通オプション

すべてのコマンドで使用可能なオプションです。

### 入力制御

| フラグ | デフォルト | 説明 |
| --- | --- | --- |
| `--delimiter <char>` | 自動判定 | CSV 区切り文字 |
| `--header` / `--noheader` | `--header` | ヘッダ行の有無 |
| `--na <strings>` | `NA,NaN,nan,N/A,n/a` | 欠損値として扱う文字列 |
| `--skip_na` / `--noskip_na` | `--skip_na` | 欠損値を除外して計算 |
| `--fail_na` | false | 欠損値があればエラー終了 |
| `--presorted` | false | データがソート済み（ソートをスキップ） |
| `--row` | false | 横並びデータを1列として処理（カンマ/スペース区切り） |

### 出力制御

| フラグ | デフォルト | 説明 |
| --- | --- | --- |
| `--json` | false | JSON 形式で出力 |
| `--quiet` | false | 数値のみ出力（パイプ向け） |

### 統計共通

| フラグ | デフォルト | 説明 |
| --- | --- | --- |
| `--alpha` | 0.05 | 有意水準 |
| `--level` | 0.95 | 信頼水準 |
| `--population` | false | 母集団統計量 (ddof=0) |

---

## desc - 記述統計

1列の数値データに対する記述統計量を計算します。

| サブコマンド | `--col` | 追加オプション | 説明 |
| --- | --- | --- | --- |
| `summary` | 1列 | `--presorted` | 要約統計量（件数, 平均, SD, 5数要約, 歪度, 尖度） |
| `mean` | 1列 | | 算術平均 |
| `median` | 1列 | `--presorted` | 中央値 |
| `mode` | 1列 | | 最頻値（複数ある場合はすべて表示） |
| `var` | 1列 | `--population` | 分散（デフォルト: 標本分散） |
| `sd` | 1列 | `--population` | 標準偏差 |
| `range` | 1列 | | 範囲 (max - min) |
| `iqr` | 1列 | `--presorted` | 四分位範囲 |
| `cv` | 1列 | | 変動係数 (SD / Mean) |
| `skewness` | 1列 | | 歪度 |
| `kurtosis` | 1列 | | 超過尖度（正規分布で 0） |
| `percentile` | 1列 | `--p`（必須）, `--presorted` | パーセンタイル値 |
| `quartiles` | 1列 | `--presorted` | 四分位数 (Q1, Q2, Q3) |
| `five-number` | 1列 | `--presorted` | 五数要約 |
| `gmean` | 1列 | | 幾何平均 |
| `hmean` | 1列 | | 調和平均 |
| `trimmed-mean` | 1列 | `--trim`（デフォルト: 0.1） | トリム平均（上下を除去） |

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

## test - 統計検定

`--alternative`（`two-sided` / `less` / `greater`）と `--alpha`（デフォルト 0.05）を指定可能。

| サブコマンド | `--col` | 追加の必須オプション | 追加の任意オプション | 説明 |
| --- | --- | --- | --- | --- |
| `t` | 1列 or 2列 | | `--mu0`, `--paired`, `--alternative` | t 検定（列数で自動判定: 1列=1標本, 2列=2標本） |
| `welch` | 2列 | | `--alternative` | Welch の t 検定 |
| `z` | 1列 | `--sigma` | `--mu0`, `--alternative` | z 検定（既知の母標準偏差が必要） |
| `f` | 2列 | | `--alternative` | F 検定（等分散の検定） |
| `shapiro` | 1列 | | | Shapiro-Wilk 正規性検定 |
| `ks` | 1列 | | | Lilliefors 正規性検定 |
| `mann-whitney` | 2列 | | `--alternative` | Mann-Whitney U 検定 |
| `wilcoxon` | 1列 or 2列 | | `--mu0`, `--alternative` | Wilcoxon 符号順位検定（1列=1標本, 2列=対応あり） |
| `kruskal` | 2列以上 | | | Kruskal-Wallis 検定 |
| `levene` | 2列以上 | | | Levene 検定（等分散性） |
| `bartlett` | 2列以上 | | | Bartlett 検定（等分散性） |
| `chisq` | 1列 or 2列 | | | カイ二乗検定（1列=適合度, 2列=観測/期待） |

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

## corr - 相関・共分散

| サブコマンド | `--col` | 追加オプション | 説明 |
| --- | --- | --- | --- |
| `pearson` | 2列 | | Pearson 相関係数 |
| `spearman` | 2列 | | Spearman 順位相関係数 |
| `kendall` | 2列 | | Kendall の順位相関係数 |
| `cov` | 2列 | `--population` | 共分散 |
| `matrix` | 2列以上 | | 相関行列 |

```bash
statcpp corr pearson test/e2e/data/two_groups.csv --col group1,group2
statcpp corr spearman test/e2e/data/two_groups.csv --col group1,group2
statcpp corr kendall test/e2e/data/two_groups.csv --col group1,group2
statcpp corr cov test/e2e/data/two_groups.csv --col group1,group2
statcpp corr matrix test/e2e/data/scores.csv --col math,science,english
```

---

## effect - 効果量

| サブコマンド | `--col` | 追加の必須オプション | 追加の任意オプション | 説明 |
| --- | --- | --- | --- | --- |
| `cohens-d` | 1列 or 2列 | | `--mu0` | Cohen's d（1列=1標本, 2列=2標本） |
| `hedges-g` | 1列 or 2列 | | `--mu0` | Hedges' g（小標本補正付き） |
| `glass-delta` | 2列 | | | Glass's delta（対照群の SD で標準化） |
| `cohens-h` | 不要（CSV 不要） | `--p1`, `--p2` | | Cohen's h（比率の効果量） |
| `odds-ratio` | 1列（4値） | | | オッズ比（a,b,c,d の4値） |
| `risk-ratio` | 1列（4値） | | | リスク比（a,b,c,d の4値） |

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

## ci - 信頼区間

`--level` で信頼水準を指定（デフォルト 0.95）。`ci prop` と `ci sample-size` は CSV 入力不要です。

| サブコマンド | `--col` | 追加の必須オプション | 追加の任意オプション | 説明 |
| --- | --- | --- | --- | --- |
| `mean` | 1列 | | `--sigma` | 平均の信頼区間（`--sigma` 指定で z、省略で t） |
| `diff` | 2列 | | | 平均の差の信頼区間 |
| `prop` | 不要 | `--successes`, `--trials` | | 比率の信頼区間 (Wilson) |
| `var` | 1列 | | | 分散の信頼区間 |
| `sample-size` | 不要 | `--moe` | `--sigma` | 必要サンプルサイズ（`--sigma` 指定で平均用、省略で比率用） |

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

## reg - 回帰分析

`--col` の最後の列が目的変数、それ以外が説明変数です。

| サブコマンド | `--col` | 説明 |
| --- | --- | --- |
| `simple` | 2列 (x, y) | 単回帰分析 |
| `multiple` | 3列以上 (x1,...,xp, y) | 重回帰分析 |
| `predict` | 2列 (x, y) | 予測値の計算 |
| `residuals` | 2列 (x, y) | 残差診断 |
| `vif` | 2列以上（説明変数のみ） | 分散膨張係数（多重共線性の診断） |

```bash
statcpp reg simple test/e2e/data/two_groups.csv --col group1,group2
statcpp reg multiple test/e2e/data/scores.csv --col math,science,english
statcpp reg predict test/e2e/data/two_groups.csv --col group1,group2
statcpp reg residuals test/e2e/data/two_groups.csv --col group1,group2
statcpp reg vif test/e2e/data/two_groups.csv --col group1,group2
```

---

## anova - 分散分析

各列を1群として扱います。

| サブコマンド | `--col` | 説明 |
| --- | --- | --- |
| `oneway` | 2列以上 | 一元配置分散分析 |
| `posthoc-tukey` | 2列以上 | Tukey HSD 事後検定 |
| `posthoc-bonferroni` | 2列以上 | Bonferroni 事後検定 |
| `posthoc-scheffe` | 2列以上 | Scheffe 事後検定 |
| `eta-squared` | 2列以上 | 効果量（eta 二乗, omega 二乗, Cohen's f） |

```bash
statcpp anova oneway test/e2e/data/scores.csv --col math,science,english
statcpp anova posthoc-tukey test/e2e/data/scores.csv --col math,science,english
statcpp anova posthoc-bonferroni test/e2e/data/scores.csv --col math,science,english
statcpp anova posthoc-scheffe test/e2e/data/scores.csv --col math,science,english
statcpp anova eta-squared test/e2e/data/scores.csv --col math,science,english
```

---

## resample - リサンプリング

出力は乱数に依存するため、実行ごとに結果が変わります。

| サブコマンド | `--col` | 追加オプション | 説明 |
| --- | --- | --- | --- |
| `bootstrap-mean` | 1列 | | 平均のブートストラップ信頼区間 |
| `bootstrap-median` | 1列 | | 中央値のブートストラップ信頼区間 |
| `bootstrap-sd` | 1列 | | 標準偏差のブートストラップ信頼区間 |
| `bca` | 1列 | | BCa 法（バイアス補正・加速ブートストラップ） |
| `permtest` | 2列 | `--paired` | 置換検定（独立2群 / 対応あり） |
| `permtest-corr` | 2列 | | 相関の置換検定 |

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

## ts - 時系列分析

| サブコマンド | `--col` | 説明 |
| --- | --- | --- |
| `acf` | 1列 | 自己相関関数（最大ラグ 20） |
| `pacf` | 1列 | 偏自己相関関数 |
| `ma` | 1列 | 移動平均（ウィンドウ = 3） |
| `ema` | 1列 | 指数移動平均（alpha = 0.3） |
| `diff` | 1列 | 階差（1次） |
| `mae` | 2列 (実測, 予測) | 平均絶対誤差 |
| `rmse` | 2列 (実測, 予測) | 二乗平均平方根誤差 |
| `mape` | 2列 (実測, 予測) | 平均絶対パーセント誤差 |

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

## robust - ロバスト統計

| サブコマンド | `--col` | 追加オプション | 説明 |
| --- | --- | --- | --- |
| `mad` | 1列 | | 中央絶対偏差 (MAD) |
| `outliers` | 1列 | | 外れ値検出（IQR 法） |
| `outliers-zscore` | 1列 | | 外れ値検出（Z-score 法） |
| `outliers-modified` | 1列 | | 外れ値検出（修正 Z-score / MAD ベース） |
| `winsorize` | 1列 | `--trim`（デフォルト: 0.1） | ウィンソライズ |
| `hodges-lehmann` | 1列 | | Hodges-Lehmann 推定量 |
| `biweight` | 1列 | | Biweight midvariance |

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

## survival - 生存分析

event 列: 1 = イベント発生, 0 = 打ち切り。

| サブコマンド | `--col` | 説明 |
| --- | --- | --- |
| `kaplan-meier` | 2列 (time, event) | Kaplan-Meier 生存曲線 |
| `logrank` | 4列 (time1, event1, time2, event2) | ログランク検定（2群比較） |
| `nelson-aalen` | 2列 (time, event) | Nelson-Aalen 累積ハザード |

```bash
statcpp survival kaplan-meier test/e2e/data/survival.csv --col time,event
statcpp survival logrank test/e2e/data/survival_two.csv --col time1,event1,time2,event2
statcpp survival nelson-aalen test/e2e/data/survival.csv --col time,event
```

---

## cluster - クラスタリング

2列以上の数値データが必要です。出力は乱数に依存する場合があります。

| サブコマンド | `--col` | 説明 |
| --- | --- | --- |
| `kmeans` | 2列以上 | k-means クラスタリング（k = 3） |
| `hierarchical` | 2列以上 | 階層的クラスタリング |
| `silhouette` | 2列以上 | シルエット分析（k = 3） |

```bash
statcpp cluster kmeans test/e2e/data/scores.csv --col math,science,english
statcpp cluster hierarchical test/e2e/data/scores.csv --col math,science,english
statcpp cluster silhouette test/e2e/data/scores.csv --col math,science,english
```

---

## multiple - 多重検定補正

p 値の列を指定します。

| サブコマンド | `--col` | 説明 |
| --- | --- | --- |
| `bonferroni` | 1列 | Bonferroni 補正 |
| `holm` | 1列 | Holm-Bonferroni 補正 |
| `bh` | 1列 | Benjamini-Hochberg (FDR) 補正 |

```bash
statcpp multiple bonferroni test/e2e/data/pvalues.csv --col pvalue
statcpp multiple holm test/e2e/data/pvalues.csv --col pvalue
statcpp multiple bh test/e2e/data/pvalues.csv --col pvalue
```

---

## power - 検出力分析

CSV 入力不要。`--n` を指定すると検出力を計算、省略すると必要サンプルサイズを計算します。

| サブコマンド | `--col` | 必須オプション | 任意オプション | 説明 |
| --- | --- | --- | --- | --- |
| `t-one` | 不要 | `--effect` | `--n`, `--power`（デフォルト: 0.8）, `--alternative` | 1標本 t 検定の検出力 |
| `t-two` | 不要 | `--effect` | `--n`, `--power`, `--ratio`（デフォルト: 1.0）, `--alternative` | 2標本 t 検定の検出力 |
| `prop` | 不要 | `--p1`, `--p2` | `--n`, `--power`, `--alternative` | 比率検定の検出力 |

```bash
statcpp power t-one --effect 0.5 --n 30
statcpp power t-one --effect 0.5 --power 0.8
statcpp power t-two --effect 0.5 --power 0.8 --ratio 2.0
statcpp power prop --p1 0.3 --p2 0.5 --n 50
```

---

## glm - 一般化線形モデル

`--col` の最後の列が応答変数です。

| サブコマンド | `--col` | 説明 |
| --- | --- | --- |
| `logistic` | 2列以上 (x1,...,xp, y) | ロジスティック回帰（y は 0/1 二値） |
| `poisson` | 2列以上 (x1,...,xp, y) | ポアソン回帰（y はカウントデータ） |

```bash
statcpp glm logistic test/e2e/data/binary.csv --col x1,x2,y
statcpp glm poisson test/e2e/data/count.csv --col x1,x2,y
```

---

## model - モデル選択

`--col` の最後の列が応答変数です。3列以上（説明変数2つ以上）が必要です。

| サブコマンド | `--col` | 説明 |
| --- | --- | --- |
| `aic` | 3列以上 | AIC によるモデル比較 |
| `cv` | 3列以上 | 交差検証（5-fold） |
| `ridge` | 3列以上 | リッジ回帰（lambda = 1.0） |
| `lasso` | 3列以上 | LASSO 回帰（lambda = 1.0） |

```bash
statcpp model aic test/e2e/data/scores.csv --col math,science,english
statcpp model cv test/e2e/data/scores.csv --col math,science,english
statcpp model ridge test/e2e/data/scores.csv --col math,science,english
statcpp model lasso test/e2e/data/scores.csv --col math,science,english
```

---

## ショートカット

よく使うコマンドをカテゴリなしで入力できます。第一引数がショートカット名に一致すると、対応するカテゴリとコマンドに自動展開されます。

```bash
# 以下の2つは同じ結果
statcpp mean data.csv --col value
statcpp desc mean data.csv --col value
```

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
