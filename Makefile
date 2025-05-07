GCC = gcc
CFLAGS = -Wall -g -I./include
LDFLAGS = -lpthread

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = test
TEST_OBJ_DIR = $(OBJ_DIR)/test
TEST_BIN_DIR = $(BIN_DIR)/test

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))
TARGET = $(BIN_DIR)/driver

# 测试相关文件
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJECTS = $(patsubst $(TEST_DIR)/%.c, $(TEST_OBJ_DIR)/%.o, $(TEST_SOURCES))
TEST_TARGETS = $(patsubst $(TEST_DIR)/%.c, $(TEST_BIN_DIR)/%, $(TEST_SOURCES))

# 模拟模式标志
SIM_FLAG = -DUSE_PC104_SIMULATION

all: directories $(TARGET)

test: directories test_directories $(TEST_TARGETS)

# 模拟模式构建目标
sim: CFLAGS += $(SIM_FLAG)
sim: clean all

test_sim: CFLAGS += $(SIM_FLAG)
test_sim: test

test_directories:
	mkdir -p $(TEST_OBJ_DIR) $(TEST_BIN_DIR)

directories:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(TARGET): $(OBJECTS)
	$(GCC) $(LDFLAGS) -o $@ $^

# 构建测试可执行文件，链接除了main.o以外的所有对象文件
$(TEST_BIN_DIR)/%: $(TEST_OBJ_DIR)/%.o $(filter-out $(OBJ_DIR)/main.o, $(OBJECTS))
	$(GCC) $(LDFLAGS) -o $@ $^

$(TEST_OBJ_DIR)/%.o: $(TEST_DIR)/%.c
	$(GCC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(GCC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean directories test test_directories sim test_sim