GCC = gcc
CFLAGS = -Wall -g -I./include
LDFLAGS = -lpthread

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

TEST_DIR = test
TEST_OBJ_DIR = $(OBJ_DIR)/test
TEST_BIN_DIR = $(BIN_DIR)

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))
TARGET = $(BIN_DIR)/driver

# 测试相关文件
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJECTS = $(patsubst $(TEST_DIR)/%.c, $(TEST_OBJ_DIR)/%.o, $(TEST_SOURCES))

# 模拟模式标志
SIM_FLAG = -DUSE_PC104_SIMULATION

# 单独的测试目标
PC104_SIM_TEST = $(TEST_BIN_DIR)/test_pc104_sim
CLOCK_TEST = $(TEST_BIN_DIR)/test_clock

all: directories $(TARGET)

# 测试目标依赖于所有的测试文件
test: directories test_directories $(PC104_SIM_TEST) $(CLOCK_TEST)

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

# PC104模拟器测试程序 - 使用模拟版本的PC104函数
$(PC104_SIM_TEST): $(TEST_OBJ_DIR)/test_pc104_sim.o $(TEST_OBJ_DIR)/pc104_simulator.o
	$(GCC) $(LDFLAGS) -o $@ $^

# 电子钟测试程序 - 使用模拟版本的PC104驱动程序
$(CLOCK_TEST): $(TEST_OBJ_DIR)/test_clock.o $(TEST_OBJ_DIR)/pc104_simulator.o $(TEST_OBJ_DIR)/pc104_bus_sim.o $(filter-out $(OBJ_DIR)/main.o $(OBJ_DIR)/pc104_bus.o, $(OBJECTS))
	$(GCC) $(LDFLAGS) -o $@ $^

# 测试对象文件编译规则
$(TEST_OBJ_DIR)/%.o: $(TEST_DIR)/%.c
	$(GCC) $(CFLAGS) -c -o $@ $<

# 源代码对象文件编译规则
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(GCC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean directories test test_directories sim test_sim