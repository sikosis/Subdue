CXX ?= g++
CXXFLAGS ?= -O2
CXXFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -Isrc
LDLIBS += -lbe -lmedia

TARGET := Subdue
SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
RESOURCE := Subdue.rsrc
TEST_TARGET := tests/options_test
PREFIX ?= /boot/home/config/non-packaged
APP_DIR := $(PREFIX)/apps/Subdue

.PHONY: all clean install uninstall test

all: $(TARGET)

$(TARGET): $(OBJECTS) $(RESOURCE)
	$(CXX) $(OBJECTS) -o $@ $(LDLIBS)
	xres -o $@ $(RESOURCE)
	mimeset -f $@

$(RESOURCE): Subdue.rdef
	rc -o $@ $<

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_TARGET): tests/OptionsTest.cpp src/Options.cpp src/Options.h
	$(CXX) $(CXXFLAGS) tests/OptionsTest.cpp src/Options.cpp -o $@ -lbe

test: $(TEST_TARGET)
	./$(TEST_TARGET)

install: $(TARGET)
	install -d "$(APP_DIR)"
	install -m 755 $(TARGET) "$(APP_DIR)/$(TARGET)"
	mimeset -f "$(APP_DIR)/$(TARGET)"

uninstall:
	rm -f "$(APP_DIR)/$(TARGET)"
	rmdir "$(APP_DIR)" 2>/dev/null || true

clean:
	rm -f $(TARGET) $(OBJECTS) $(RESOURCE) $(TEST_TARGET)
