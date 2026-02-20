# statcpp CLI 設計・開発者ガイド

## コンセプト

**UNIX パイプラインに統計学を持ち込む。**

`awk` が文字列処理、`jq` が JSON 処理を担うように、`statcpp` が統計処理を担う。
シングルバイナリ・依存ゼロ・高速起動で、データ分析のワークフローに自然に溶け込むツール。

**一言で:** 「datamash の手軽さ」+「R の機能」+「C++ の速度」

---

## アーキテクチャ

### ファイル構成

```
statcppCLI/
├── CMakeLists.txt                     ビルド設定
├── cmake/
│   ├── gflags.cmake                   gflags 自動 DL+ビルド
│   └── nlohmann-json.cmake            nlohmann/json 自動 DL
├── src/
│   ├── main.cpp                       エントリーポイント (gflags 定義, ディスパッチ)
│   └── include/
│       ├── csv_reader.hpp             CSV/TSV リーダー (RFC 4180)
│       ├── cli_parser.hpp             サブコマンド解析・ショートカット
│       ├── output_formatter.hpp       テキスト/JSON/quiet 出力
│       └── commands/
│           ├── desc.hpp               記述統計 (17 コマンド)
│           ├── test_cmd.hpp           統計検定 (12 コマンド)
│           ├── corr.hpp               相関・共分散 (5 コマンド)
│           ├── effect.hpp             効果量 (6 コマンド)
│           ├── ci.hpp                 信頼区間 (5 コマンド)
│           ├── reg.hpp                回帰分析 (5 コマンド)
│           ├── anova.hpp              分散分析 (5 コマンド)
│           ├── resample.hpp           リサンプリング (6 コマンド)
│           ├── ts.hpp                 時系列分析 (8 コマンド)
│           ├── robust.hpp             ロバスト統計 (7 コマンド)
│           ├── survival.hpp           生存分析 (3 コマンド)
│           ├── cluster.hpp            クラスタリング (3 コマンド)
│           ├── multiple.hpp           多重検定補正 (3 コマンド)
│           ├── power.hpp              検出力分析 (3 コマンド)
│           ├── glm_cmd.hpp            一般化線形モデル (2 コマンド)
│           └── model.hpp              モデル選択 (4 コマンド)
├── test/
│   ├── test_csv_reader.cpp            CSV リーダー単体テスト
│   ├── test_output_formatter.cpp      出力フォーマッター単体テスト
│   ├── test_cli_parser.cpp            引数解析テスト
│   └── e2e/
│       ├── run_e2e.sh                 E2E テストランナー
│       ├── data/                      テスト用 CSV
│       └── golden/                    期待出力ファイル
└── download/                          自動生成 (.gitignore 対象)
    ├── statcpp/
    │   ├── statcpp-main.tar.gz        statcpp アーカイブキャッシュ
    │   └── statcpp-install/           statcpp ヘッダー (include/statcpp/)
    ├── gflags/
    │   ├── gflags/                    gflags ソース + ビルド (_build/)
    │   └── gflags-install/            gflags インストール先 (lib/, include/)
    └── nlohmann-json/
        └── nlohmann-json-install/     json.hpp (include/nlohmann/)
```

### ヘッダーオンリー設計

全コマンドファイルは `.hpp` (ヘッダーオンリー) で実装。各関数は `inline` 付き。

**理由:**
1. statcpp ライブラリ自体がヘッダーオンリー — スタイルの統一
2. CMakeLists.txt にソースファイルを追加する必要がない
3. `main.cpp` が各 `.hpp` を `#include` するだけで完結

**トレードオフ:**
- `main.cpp` 1 ファイルのコンパイルで済む（現状は問題ない）
- コマンド数が大幅に増えた場合、ビルド時間が長くなる可能性あり

### 処理フロー

```
main.cpp
  ├── gflags::ParseCommandLineFlags()    フラグ解析
  ├── parse_command()                     カテゴリ/コマンド/ファイルパス取得
  ├── CsvReader::read_file/read_stdin()   CSV 読み込み (power/ci sample-size は除く)
  ├── OutputFormatter(mode)               出力モード決定
  └── run_<category>()                    カテゴリ別ディスパッチ
        ├── csv.get_clean_data()          列取得 + 欠損値除去
        ├── statcpp::*()                  統計計算
        └── fmt.print() / fmt.flush()    結果出力
```

---

## 依存関係

| ライブラリ | 用途 | 導入方法 |
|---|---|---|
| [statcpp](https://github.com/mitsuruk/statcpp) | 統計計算本体 (ヘッダーオンリー) | `cmake/statcpp.cmake` で GitHub から自動 DL |
| [gflags](https://github.com/gflags/gflags) | コマンドライン引数解析 | `cmake/gflags.cmake` で自動 DL+ビルド |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON 出力 | `cmake/nlohmann-json.cmake` で自動 DL |
| [Google Test](https://github.com/google/googletest) | 単体テスト (オプション) | `-DGTEST=true` で有効化 |

### gflags の設計

- グローバルフラグ (`--col`, `--json`, `--alpha` 等) は `main.cpp` で `DEFINE_*` マクロで定義
- 各コマンドファイルでは `DECLARE_*` で参照宣言
- サブコマンド (category, command) は gflags ではなく `cli_parser.hpp` で手動解析

### CSV 不要なコマンド

以下のコマンドは CSV 入力なしで動作:
- `ci sample-size` — 必要サンプルサイズ計算
- `ci prop` — 比率の信頼区間（`--successes`, `--trials` で指定）
- `effect cohens-h` — 比率の効果量（`--p1`, `--p2` で指定）
- `power *` — 検出力分析全般

`main.cpp` の `needs_csv` フラグで制御。

---

## データの並び・ソート戦略

### statcpp ライブラリのソート要件

statcpp には **ソート済みデータを前提とする関数** と **内部でソートする関数** が混在。
CLI 層でこの差異をユーザーに意識させない。

| 分類 | 関数例 | CLI の対処 |
|---|---|---|
| ソート済み入力が必須 | `median()`, `quartiles()`, `percentile()`, `iqr()`, `five_number_summary()` | CLI 側で自動ソート |
| 内部でソートする | `mad()`, `shapiro_wilk_test()`, `kaplan_meier()` | そのまま渡す |
| 順序が意味を持つ (ソート禁止) | `acf()`, `moving_average()`, `diff()`, `t_test_paired()`, 回帰全般 | そのまま渡す |

### 実装方針

1. **デフォルト:** コマンドごとの要件に従い、必要なら CLI 側で自動ソート
2. **最適化:** `--presorted` でソート済みデータのコピー・ソートをスキップ
3. **summary コマンド:** 1 回だけソートして median, quartiles, five_number_summary に再利用
4. **複数列:** 絶対にソートしない（対応関係が壊れる）

### 欠損値除去とソートの順序

```
1. CSV 読み込み
2. 欠損値除去 (--skip_na)
3. ソート（必要な場合のみ）
4. 統計計算
```

---

## 出力設計

### 3 つの出力モード

| モード | フラグ | 用途 | 形式 |
|---|---|---|---|
| Text | (デフォルト) | 人間が読む | `  Label:  value` |
| JSON | `--json` | プログラムから利用 | 構造化 JSON |
| Quiet | `--quiet` | パイプライン | 数値のみ |

### JSON 出力構造

```json
{
    "command": "desc.summary",
    "input": {
        "column": "value",
        "n": 5
    },
    "result": {
        "Count": 5.0,
        "Mean": 30.0,
        "Std Dev": 15.811388300841896,
        "Min": 10.0,
        "Median": 30.0,
        "Max": 50.0
    }
}
```

---

## テスト戦略

### 用語

| 用語 | 正式名称 | 意味 |
| --- | --- | --- |
| E2E テスト | End-to-End テスト | システム全体を入力から出力まで一気通貫で検証するテスト。単体テストが関数単位で正しさを確認するのに対し、E2E テストは実際のバイナリを実行して「ユーザーと同じ操作で期待通りの出力が得られるか」を確認する |
| ゴールデンファイル | Golden file | 「正解」として事前に保存しておく期待出力ファイル。テスト時に実際の出力と `diff` で比較し、不一致があれば FAIL とする。"gold standard"（金本位＝基準）に由来する |

### 3 層構造

```
Layer 1: statcpp ライブラリのテスト（既存・変更不要）
  ├── Google Test 758 テスト（関数単位の正確性）
  └── R 検証 167 チェック（数値精度の保証）

Layer 2: CLI 固有のユニットテスト（28 テスト）
  ├── test_csv_reader.cpp      CSV/TSV パーサーのテスト
  ├── test_output_formatter.cpp 出力フォーマットのテスト
  └── test_cli_parser.cpp      引数解析のテスト

Layer 3: E2E テスト（52 テスト）
  ├── ゴールデンファイルテスト  期待出力との diff 比較
  └── エラーケーステスト        異常系の動作確認

Layer 4: リファレンス検証（126 テスト）
  ├── docs/run_reference.sh    test-reference.md の全例を実行
  └── docs/output.txt          実行結果（PASS: 126, SKIP: 0）
```

### ビルド・インストール・テスト実行

```bash
# CLI バイナリのビルド
cmake -B build && cmake --build build

# インストール（デフォルト: /usr/local/bin/statcpp）
sudo cmake --install build

# 任意のディレクトリにインストール
cmake --install build --prefix ~/.local    # → ~/.local/bin/statcpp

# 単体テスト（GTest 有効化してビルド）
cmake -B build -DGTEST=true && cmake --build build && ctest --test-dir build --verbose

# CLI バイナリに戻す（GTest 無効化）
cmake -B build -DGTEST=false && cmake --build build

# E2E テスト
cd test/e2e && bash run_e2e.sh

# リファレンス全例の検証
bash docs/run_reference.sh
```

**注意:** `-DGTEST=true` でビルドすると、バイナリはテストランナーになります。
CLI として使う場合は `-DGTEST=false` で再ビルドしてください。

### ゴールデンファイルの更新

出力フォーマットを変更した場合:

1. CLI バイナリを再ビルド
2. 変更されたコマンドを実行し、出力をゴールデンファイルに保存
3. `diff` で変更内容を確認
4. E2E テスト全体を実行して確認

```bash
# 例: desc summary のゴールデンファイル更新
cd test/e2e
../../build/statcpp desc summary data/basic.csv --col value > golden/desc_summary.txt
bash run_e2e.sh
```

---

## 新しいコマンドの追加手順

### 1. コマンドファイルの作成/編集

`src/include/commands/<category>.hpp` に `cmd.command == "new-cmd"` のブランチを追加。

```cpp
} else if (cmd.command == "new-cmd") {
    auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
    double result = statcpp::new_function(data.begin(), data.end());
    fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}});
    fmt.print("Result", result);
}
```

### 2. 新カテゴリの場合

1. `commands/new_category.hpp` を作成
2. `main.cpp` に `#include` と `DECLARE_*` を追加
3. `main.cpp` のディスパッチに `else if` を追加
4. `cli_parser.hpp` の `categories` ベクタに追加
5. CSV 不要なコマンドなら `needs_csv` の条件を更新
6. 新しい gflags が必要なら `main.cpp` に `DEFINE_*` を追加

### 3. テストの追加

1. E2E テスト: `run_e2e.sh` にテストケース追加
2. ゴールデンファイル: `test/e2e/golden/` に期待出力を保存
3. テストデータ: 必要なら `test/e2e/data/` に CSV を追加

---

## パイプライン活用例

```bash
# CSV の特定列の要約統計量
statcpp desc summary data.csv --col price

# 正規性検定 → 適切な検定を選択
statcpp test shapiro data.csv --col score

# JSON 出力をパイプで処理
statcpp test t data.csv --col a,b --json | jq '.result["p-value"]'

# 数値のみ出力をパイプ
statcpp desc mean data.csv --col price --quiet | xargs echo "Mean:"

# stdin から読み込み
cat data.csv | statcpp desc mean --col value
awk '{print $3}' access.log | statcpp desc summary --noheader --col 1

# --row: 横並びデータを直接処理（カンマ/スペース区切り）
echo "1,2,3,4,5" | statcpp desc mean --noheader --col 1 --row
echo "1 2 3 4 5" | statcpp desc mean --noheader --col 1 --row

# 複数ファイルの一括分析
for f in experiment_*.csv; do
  echo "=== $f ==="
  statcpp desc summary "$f" --col result --quiet
done
```

---

## 未実装の設計アイデア

以下は `doc/CLI.md`（初期設計書）に記載されているが、現在未実装の機能:

- `--csv` 出力モード
- `--seed` 乱数シード（リサンプリング用）
- `--verbose` 詳細出力（ソート時間表示等）
- `test prop` / `test prop2` (比率の検定)
- `test chisq-indep` (独立性のカイ二乗検定)
- `test fisher` (Fisher の正確確率検定)
- `anova twoway` (二元配置分散分析)
- `anova ancova` (共分散分析)
- `cluster` の `--k` オプション (現在はデフォルト k=3 固定)
- `ts` の `--lag`, `--window`, `--alpha` オプション (現在はデフォルト値固定)
- シェル補完 (bash / zsh / fish)
- man page 生成
- Homebrew formula / apt パッケージ
