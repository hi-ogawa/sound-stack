CXX = clang++
CXXFLAGS += -g -Wall -Wextra -std=c++14
DEPS += $(shell pkg-config --cflags --libs sdl2)
CPPFLAGS +=

TARGETS += sdl_audio_test

all: $(TARGETS)

sdl_audio_test: sdl_audio_test.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPS) $^ -o $@

.PHONY: clean
clean:
	rm -f $(TARGETS)
