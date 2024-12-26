# Makefile

# srcファイルのパス
SRC_DIR = src
SRC_FILES_EXE = main.cpp $(SRC_DIR)/util_new.cpp $(SRC_DIR)/labeled_graph_new.cpp $(SRC_DIR)/query_new.cpp $(SRC_DIR)/planner_new.cpp $(SRC_DIR)/generic_executor.cpp # 問合せ処理用
SRC_FILES_SER = serialize.cpp $(SRC_DIR)/util_new.cpp # データシリアライズ用

# 出力ディレクトリと実行ファイル名
OBJ_DIR = obj
EXE = exec.out # 問合せ処理を実行
EXE_DEBUG = exec_debug.out
SER = serialize.out # データセットのシリアライズ
SER_DEBUG = serialize_debug.out

# オブジェクトファイル
OBJ_FILES_EXE = $(SRC_FILES_EXE:%.cpp=$(OBJ_DIR)/%.o)
OBJ_FILES_EXE_DEBUG = $(SRC_FILES_EXE:%.cpp=$(OBJ_DIR)/%.debug.o)
OBJ_FILES_SER = $(SRC_FILES_SER:%.cpp=$(OBJ_DIR)/%.o)
OBJ_FILES_SER_DEBUG = $(SRC_FILES_SER:%.cpp=$(OBJ_DIR)/%.debug.o)

# インクルードディレクトリ
INCLUDE = -I./include

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
release: $(OBJ_FILES_EXE)
	$(GCC) $(CFLAGS_RELEASE) -o $(EXE) $(OBJ_FILES_EXE)

release_s: $(OBJ_FILES_SER)
	$(GCC) $(CFLAGS_RELEASE) -o $(SER) $(OBJ_FILES_SER)

# デバッグモードのコンパイル
debug: $(OBJ_FILES_EXE_DEBUG)
	$(CLANG) $(CFLAGS_DEBUG) -o $(EXE_DEBUG) $(OBJ_FILES_EXE_DEBUG)

debug_s: $(OBJ_FILES_SER_DEBUG)
	$(CLANG) $(CFLAGS_DEBUG) -o $(SER_DEBUG) $(OBJ_FILES_SER_DEBUG)

# 本番モード用オブジェクトファイルの生成
$(OBJ_DIR)/%.o: %.cpp %.hpp
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/$(SRC_DIR)
	$(GCC) $(CFLAGS_RELEASE) -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/$(SRC_DIR)
	$(GCC) $(CFLAGS_RELEASE) -c $< -o $@
	
# デバッグモード用オブジェクトファイルの生成
$(OBJ_DIR)/%.debug.o: %.cpp %.hpp
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/$(SRC_DIR)
	$(CLANG) $(CFLAGS_DEBUG) -c $< -o $@

$(OBJ_DIR)/%.debug.o: %.cpp
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/$(SRC_DIR)
	$(CLANG) $(CFLAGS_DEBUG) -c $< -o $@

# クリーンアップ
clean_all: clean clean_debug clean_s clean_s_debug
clean:
	rm -f $(OBJ_FILES_EXE) $(EXE)
clean_debug:
	rm -f $(OBJ_FILES_EXE_DEBUG) $(EXE_DEBUG)
clean_s:
	rm -f $(OBJ_FILES_SER) $(SER)
clean_s_debug:
	rm -f $(OBJ_FILES_SER_DEBUG) $(SER_DEBUG)
