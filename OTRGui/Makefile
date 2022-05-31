USE_LLD ?= 0
CXXFLAGS ?=
LDFLAGS := -lGL -ldl -lpthread

BACKEND ?= SDL

ifeq ($(shell command -v clang++ >/dev/null 2>&1; echo $$?),0)
	CXX := clang++
else
	CXX := g++
endif

# Use LLD if available. Set LLD=0 to not use it
ifeq ($(shell command -v ld.lld >/dev/null 2>&1; echo $$?),0)
	LDFLAGS+= -fuse-ld=lld
endif



SRC_DIRS := src/ImGui #$(shell find ZAPD -type d)
SRC_DIRS += src/BaseromUtils

CPP_FILES := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))
CPP_FILES += src/main.cpp

ifeq ($(BACKEND),SDL)
	CPP_FILES += src/ImGui/backends/imgui_impl_opengl2.cpp
	CPP_FILES += src/ImGui/backends/imgui_impl_sdl.cpp
# There are a bunch of different places that SDL might be installed to. Just include all of the options.
	CXXFLAGS += -I/usr/include/SDL2 -I/usr/local/include/SDL2 -I/opt/local/include/SDL2
	LDFLAGS += -lSDL2
endif


O_FILES   := $(foreach f,$(CPP_FILES:.cpp=.o),build/$f)


CXXFLAGS += -O2 -DUSE_SDL -I ./src/ImGui/ -I ./src/ImGui/backends/ -std=c++20 -target aarch64-linux-gnu  -mtune=cortex-a57  -fPIE

$(shell mkdir -p $(foreach dir,$(SRC_DIRS),build/$(dir)))
$(shell mkdir -p build/src/ImGui/backends)

all: OTRGui.elf

build/%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $(OUTPUT_OPTION) $<

clean:
	rm -rf build OTRGui.elf

OTRGui.elf : $(O_FILES)
	$(CXX) $(CXXFLAGS) $(O_FILES) $(OUTPUT_OPTION) $(LDFLAGS)
.PHONY: all
