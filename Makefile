CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
LDFLAGS := -lheif -lwebp

# Use pkg-config if available
PKG_CONFIG := $(shell command -v pkg-config 2> /dev/null)
ifdef PKG_CONFIG
    CXXFLAGS += $(shell pkg-config --cflags libheif libwebp 2>/dev/null || true)
    LDFLAGS = $(shell pkg-config --libs libheif libwebp 2>/dev/null || echo "-lheif -lwebp")
endif

# macOS Homebrew paths
ifeq ($(shell uname), Darwin)
    HOMEBREW_PREFIX := $(shell brew --prefix 2>/dev/null || echo /opt/homebrew)
    CXXFLAGS += -I$(HOMEBREW_PREFIX)/include
    LDFLAGS += -L$(HOMEBREW_PREFIX)/lib
endif

SRC_DIR := src
BUILD_DIR := build
TARGET := heic2webp

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Install dependencies (macOS)
deps:
	brew install libheif libwebp
