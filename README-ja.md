# statcpp CLI

[![CI](https://github.com/mitsuruk/statcppCLI/actions/workflows/ci.yml/badge.svg)](https://github.com/mitsuruk/statcppCLI/actions/workflows/ci.yml)
[![Docs](https://img.shields.io/badge/docs-online-brightgreen.svg)](https://mitsuruk.github.io/statcppCLI/)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux%20%7C%20Windows-lightgrey.svg)](https://github.com/mitsuruk/statcppCLI/actions/workflows/ci.yml)
[![Tests](https://img.shields.io/badge/tests-49%20e2e-brightgreen.svg)](https://github.com/mitsuruk/statcppCLI/actions/workflows/ci.yml)

**UNIX パイプラインに統計学を持ち込む。**
*Windows (MSVC / MinGW) でも動作します*

[English](README.md)

## 開発の目的

C++17 ヘッダーオンリー統計ライブラリ `statcpp` を使ったコマンドライン統計ツール。
`awk` が文字列処理、`jq` が JSON 処理を担うように、`statcpp` が統計処理を担う。
シングルバイナリ・依存ゼロ・高速起動で、データ分析のワークフローに自然に溶け込むツールを目指す。

## 特徴

- 16 カテゴリ・94 コマンド（記述統計から生存分析まで）
- CSV / TSV 自動判定、stdin パイプ対応
- テキスト / JSON / quiet 出力モード
- C++17 シングルバイナリ、起動数 ms

## ビルド

```bash
git clone <repository-url>
cd statcppCLI
cmake -B build && cmake --build build
```

ビルド済みバイナリ: `build/statcpp`

**Windows (MSVC):**

```bat
git clone <repository-url>
cd statcppCLI
cmake -S . -B build
cmake --build build --config Release
```

ビルド済みバイナリ: `build\Release\statcpp.exe`

### インストール

```bash
# /usr/local/bin/statcpp にインストール
sudo cmake --install build

# 任意のディレクトリにインストール
cmake --install build --prefix ~/.local
```

**Windows:**

```bat
cmake --install build --config Release --prefix C:\Users\<user>\AppData\Local\statcpp
```

### 前提条件

- CMake 3.20+
- C++17 対応コンパイラ (GCC 9+, Clang 10+, Apple Clang 12+, MSVC 2019+)

statcpp, gflags, nlohmann/json は CMake が自動でダウンロードします。

## クイックスタート

```bash
# 基本統計量の一括表示
statcpp desc summary data.csv --col price

# 平均値（パイプ）
cat data.csv | statcpp desc mean --col value

# stdin から直接
printf '1\n2\n3\n4\n5\n' | statcpp desc mean --noheader --col 1

# インラインデータ（スペースまたはカンマ区切り）
echo '1 2 3 4 5' | statcpp desc mean --noheader --col 1 --row

# t 検定（2 群比較）
statcpp test t data.csv --col group1,group2

# 相関分析
statcpp corr pearson data.csv --col x,y

# 回帰分析
statcpp reg simple data.csv --col x,y

# JSON 出力
statcpp desc summary data.csv --col price --json

# 数値のみ（パイプ向け）
statcpp desc mean data.csv --col price --quiet

# 検出力分析（CSV 不要）
statcpp power t-one --effect 0.5 --n 30
```

## 出力例

```text
$ statcpp desc summary data.csv --col value
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

## カテゴリ一覧

| カテゴリ | 説明 | コマンド数 |
| --- | --- | --- |
| `desc` | 記述統計 | 17 |
| `test` | 統計検定 | 12 |
| `corr` | 相関・共分散 | 5 |
| `effect` | 効果量 | 6 |
| `ci` | 信頼区間 | 5 |
| `reg` | 回帰分析 | 5 |
| `anova` | 分散分析 | 5 |
| `resample` | リサンプリング | 6 |
| `ts` | 時系列分析 | 8 |
| `robust` | ロバスト統計 | 7 |
| `survival` | 生存分析 | 3 |
| `cluster` | クラスタリング | 3 |
| `multiple` | 多重検定補正 | 3 |
| `power` | 検出力分析 | 3 |
| `glm` | 一般化線形モデル | 2 |
| `model` | モデル選択 | 4 |

ショートカット: `mean`, `median`, `mode`, `sd`, `var`, `summary`, `range`, `iqr`, `cv`, `skewness`, `kurtosis`, `quartiles`, `gmean`, `hmean`, `ttest`, `pearson`, `spearman`, `kendall`

## ドキュメント

- [コマンドリファレンス](docs/commands-ja.md) - 全コマンドのオプションと使い方
- [テストリファレンス](docs/test-reference-ja.md) - 全コマンドの実行例と出力検証
- [設計・開発者ガイド](docs/design-ja.md) - アーキテクチャと開発情報
- [テスト検証結果](docs/output.txt) - 全コマンドの実行結果

## テスト

```bash
# 単体テスト
cmake -B build -DGTEST=true && cmake --build build && ctest --test-dir build --verbose

# E2E テスト (macOS / Linux のみ)
cd test/e2e && bash run_e2e.sh

# リファレンス全例の検証 (macOS / Linux のみ)
bash docs/run_reference.sh
```

## 動作確認環境

- macOS + Apple Clang 17.0.0
- macOS + GCC 15 (Homebrew)
- Ubuntu 24.04 ARM64 + GCC 13.3.0
- Windows 11 ARM64 + MSVC 2022 (Parallels 経由)

## ライセンス

このプロジェクトは MIT ライセンスの下で公開されています。詳細は [LICENSE](LICENSE) ファイルを参照してください。

### 依存ライブラリ

| ライブラリ | ライセンス |
| --- | --- |
| [statcpp](https://github.com/mitsuruk/statcpp) | MIT License |
| [gflags](https://github.com/gflags/gflags) | BSD 3-Clause License |
| [nlohmann/json](https://github.com/nlohmann/json) | MIT License |

## 謝辞

このプロジェクトの開発には以下のツールと AI を活用しています:

- **Claude Code for VS Code Opus 4.6** - ドキュメント類の用語の統一、説明不足の確認、テストコードの作成と結果の確認
- **OpenAI ChatGPT 5.2** - ドキュメント類の用語の統一、説明不足の確認

---

**注意**: 本ソフトウェアは数値安定性や極端なエッジケースへの対応において、商用の統計ソフトウェアと同等のレベルではありません。研究や本番環境で使用する場合は、結果を他のツールで検証することをお勧めします。
