CXX = clang++
CXXFLAGS = -std=c++23 -stdlib=libc++ -O2 -Wall -Wextra
TARGET = cppcal
SRC = main.cpp

LLVM_LIBDIR := $(shell $(CXX) -print-resource-dir 2>/dev/null)/../../..
STD_CPPM := $(firstword $(wildcard $(LLVM_LIBDIR)/share/libc++/v1/std.cppm /usr/lib/llvm-*/share/libc++/v1/std.cppm /usr/share/libc++/v1/std.cppm))

all: $(TARGET)

$(TARGET): $(SRC) std.pcm
	$(CXX) $(CXXFLAGS) -fmodule-file=std=std.pcm $(SRC) std.pcm -o $@

std.pcm:
	@if [ -z "$(STD_CPPM)" ]; then \
		echo "error: std.cppm not found." >&2; \
		exit 1; \
	fi
	$(CXX) $(CXXFLAGS) --precompile $(STD_CPPM) -o $@

clean:
	rm -f $(TARGET) std.pcm

.PHONY: all clean
