CXX = g++
CXXSTD = -std=c++17
CXXFLAGS = $(CXXSTD) -Wall -Wextra -Wpedantic -Iinclude -g -O0
LDFLAGS = -lpthread

SRCDIR = src
INCDIR = include
OBJDIR = obj

COMMON_SRCS = $(wildcard $(SRCDIR)/common/*.cpp)
AUTH_SRCS = $(wildcard $(SRCDIR)/auth/*.cpp)
ORDER_SRCS = $(wildcard $(SRCDIR)/order/*.cpp)
INVENTORY_SRCS = $(wildcard $(SRCDIR)/inventory/*.cpp)
PAYMENT_SRCS = $(wildcard $(SRCDIR)/payment/*.cpp)
DB_SRCS = $(wildcard $(SRCDIR)/db/*.cpp)
CACHE_SRCS = $(wildcard $(SRCDIR)/cache/*.cpp)
AUDIT_SRCS = $(wildcard $(SRCDIR)/audit/*.cpp)
NOTIFY_SRCS = $(wildcard $(SRCDIR)/notify/*.cpp)
PROCESS_SRCS = $(wildcard $(SRCDIR)/process/*.cpp)
BUSINESS_SRCS = $(wildcard $(SRCDIR)/business/*.cpp)
UTILS_SRCS = $(wildcard $(SRCDIR)/utils/*.cpp)
ADAPTER_SRCS = $(wildcard $(SRCDIR)/adapter/*.cpp)
FEATURE_SRCS = $(wildcard $(SRCDIR)/feature/*.cpp)
MAIN_SRC = $(SRCDIR)/main.cpp

ALL_SRCS = $(COMMON_SRCS) $(AUTH_SRCS) $(ORDER_SRCS) $(INVENTORY_SRCS) \
           $(PAYMENT_SRCS) $(DB_SRCS) $(CACHE_SRCS) $(AUDIT_SRCS) $(NOTIFY_SRCS) \
           $(MAIN_SRC) $(PROCESS_SRCS) $(BUSINESS_SRCS) $(UTILS_SRCS) $(ADAPTER_SRCS) $(FEATURE_SRCS)

OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(ALL_SRCS))

TARGET = oms

.PHONY: all clean run help modules

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET) oms.log

run: $(TARGET)
	./$(TARGET)

modules:
	@echo "Common module: $(words $(COMMON_SRCS)) files"
	@echo "Auth module: $(words $(AUTH_SRCS)) files"
	@echo "Order module: $(words $(ORDER_SRCS)) files"
	@echo "Inventory module: $(words $(INVENTORY_SRCS)) files"
	@echo "Payment module: $(words $(PAYMENT_SRCS)) files"
	@echo "DB module: $(words $(DB_SRCS)) files"
	@echo "Cache module: $(words $(CACHE_SRCS)) files"
	@echo "Audit module: $(words $(AUDIT_SRCS)) files"
	@echo "Notify module: $(words $(NOTIFY_SRCS)) files"

lines:
	find . -name "*.cpp" -o -name "*.h" | xargs wc -l | tail -1

help:
	@echo "OMS - Order Management System Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all      - Build the entire system (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  run      - Build and run the system"
	@echo "  modules  - Show module file count"
	@echo "  lines    - Count total lines of code"
	@echo "  help     - Show this help"
