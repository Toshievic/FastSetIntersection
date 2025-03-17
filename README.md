# Fast Set Intersection

第17回 データ工学と情報マネジメントに関するフォーラムにて発表した「集約隣接リストを用いた Worst-Case Optimal Join の高速化」の評価実験に用いたコード。

## Environments
* gcc 14.2.0
* MacOS 14.2.1, Apple M1 Ultra chip

## Usage

### input
* 評価に用いるデータセット、クエリが必要です。
* このうち、データセットはvertices.csv, edges.csvの2つのファイルを用意する必要があります。それぞれのファイルはカンマ区切りで以下のカラムを持ちます。
    * vertices.csv: 頂点ID, 頂点ラベル
    * edges.csv: 始点ID, 終点ID, 辺ラベル
* クエリのフォーマットは以下の通りです。
```
<クエリ頂点1> <頂点ラベル>
<クエリ頂点2> <頂点ラベル>
...

<始点側クエリ頂点> <辺ラベル> <終点側クエリ頂点>
<始点側クエリ頂点> <辺ラベル> <終点側クエリ頂点>
...
```
* データ構造が使用するメモリ上限を指す環境変数`SIZE_LIMIT`をセットします。

### Serialization
* 提案手法はデータセットをCRSフォーマットで読み込むため、以下のコマンドによってデータセットを変換します。
    * filteredは任意
```
% serialize.out --input_dirpath <データセットへのパス> --output_dirpath <出力先のパス> --items agg_al (filtered)
```

### Query Processing
以下のコマンドを実行し、部分グラフ問合せを処理します。
```
% exec.out --data_dirpath <CRS形式のデータセットへのパス> --query_filepath <クエリファイルのパス> --method agg
```
処理が完了すると、以下のように処理結果が表示されます。
```
=== Agg Executor ===
best_order: 1 0 3 2 
min_cost: 2
via_map: 3->2 0->2 
--- Basic Info ---
Method: agg             Query: Q4_n
Options:
--- Runtimes ---
join_runtime: 100.783000 [ms]
--- Results ---
result_size: 87000186
intersection_count: 151758
```