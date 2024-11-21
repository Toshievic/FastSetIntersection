# Makefile

# 実行ファイル名
EXEC1 = exec # 問合せ処理を実行
EXEC1_DEBUG = exec_debug
EXEC2 = serialize # データセットのシリアライズ
EXEC2_DEBUG = serialize_debug

# オブジェクトファイル
# 問合せ処理の実行に必要なOBJファイル
OBJ = master.o util.o query.o labeled_graph.o generic_join.o alpha_generic_join.o beta_generic_join.o gamma_generic_join.o main.o
OBJ_DEBUG = master.debug.o util.debug.o query.debug.o labeled_graph.debug.o generic_join.debug.o alpha_generic_join.debug.o beta_generic_join.debug.o gamma_generic_join.debug.o main.debug.o
# データセットのシリアライズに必要なOBJファイル
OBJ_S = master.o util.o serialize_dataset.o
OBJ_S_DEBUG = master.debug.o util.debug.o serialize_dataset.debug.o

# インクルードディレクトリ
INCLUDE = -I../include

# コンパイラ (clanはmac上でデバッグする際に楽)
GCC = g++ -std=c++23
CLANG = clang++ -std=c++23

# 本番モード用オプション
CFLAGS_RELEASE = -O3 $(INCLUDE)

# デバッグモード用オプション
CFLAGS_DEBUG = -g $(INCLUDE)

# デフォルトターゲットは本番モードのコンパイル
all: release

# 本番モードのコンパイル
release: $(OBJ)
	$(GCC) $(CFLAGS_RELEASE) -o $(EXEC1) $(OBJ)

release_s: $(OBJ_S)
	$(GCC) $(CFLAGS_RELEASE) -o $(EXEC2) $(OBJ_S)

# デバッグモードのコンパイル
debug: $(OBJ_DEBUG)
	$(CLANG) $(CFLAGS_DEBUG) -o $(EXEC1_DEBUG) $(OBJ_DEBUG)

debug_s: $(OBJ_S_DEBUG)
	$(CLANG) $(CFLAGS_DEBUG) -o $(EXEC2_DEBUG) $(OBJ_S_DEBUG)

# 本番モード用オブジェクトファイルの生成
%.o: %.cpp %.hpp
	$(GCC) $(CFLAGS_RELEASE) -c $< -o $@

%.o: %.cpp
	$(GCC) $(CFLAGS_RELEASE) -c $< -o $@
	
# デバッグモード用オブジェクトファイルの生成
%.debug.o: %.cpp %.hpp
	$(CLANG) $(CFLAGS_DEBUG) -c $< -o $@

%.debug.o: %.cpp
	$(CLANG) $(CFLAGS_DEBUG) -c $< -o $@

# クリーンアップ
clean_all: clean clean_debug clean_s clean_s_debug
clean:
	rm -f $(OBJ) $(EXEC1)
clean_debug:
	rm -f $(OBJ_DEBUG) $(EXEC1_DEBUG)
clean_s:
	rm -f $(OBJ_S) $(EXEC2)
clean_s_debug:
	rm -f $(OBJ_S_DEBUG) $(EXEC2_DEBUG)
