\USE_LLD ?= 0
CXXFLAGS ?=
ifeq ($(shell command -v clang++ >/dev/null 2>&1; echo $$?),0)
	CXX := clang++
else
	CXX := g++
endif

# Use LLD if available. Set LLD=0 to not use it
ifeq ($(shell command -v ld.lld >/dev/null 2>&1; echo $$?),0)
	USE_LLD := 1
endif


SRC_DIRS := src/ImGui #$(shell find ZAPD -type d)
SRC_DIRS += src/BaseromUtils

CPP_FILES := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))
CPP_FILES += src/main.cpp
CPP_FILES += src/ImGui/backends/imgui_impl_opengl2.cpp
CPP_FILES += src/ImGui/backends/imgui_impl_sdl.cpp
O_FILES   := $(foreach f,$(CPP_FILES:.cpp=.o),build/$f)

CXXFLAGS += -O2 -DUSE_SDL  -I ./src/ImGui/ -I ./src/ImGui/backends/  -I/usr/include/SDL2 -I/usr/local/include/SDL2 -I/opt/local/include/SDL2 -std=c++20

$(shell mkdir -p $(foreach dir,$(SRC_DIRS),build/$(dir)))
$(shell mkdir -p src/ImGui/backends)

all: OTRGui.elf

build/%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $(OUTPUT_OPTION) $<

OTRGui.elf : $(O_FILES)
	$(CXX) $(CXXFLAGS) $(O_FILES)  $(OUTPUT_OPTION) -lSDL2 -lGL -ldl -fuse-ld=lld -lpthread

.PHONY: all
