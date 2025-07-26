include .env

CFLAGS = -std=c++17 -I. -I$(VULKAN_SDK_PATH)/include -I/opt/homebrew/include -I$(TINYOBJ_PATH)
LDFLAGS = \
	-L$(VULKAN_SDK_PATH)/lib \
	-Wl,-headerpad_max_install_names \
	-Wl,-rpath,$(VULKAN_SDK_PATH)/lib \
	`pkg-config --static --libs glfw3` \
	-lvulkan

# Create list of all spv files and set as dependency
vertSources = $(shell find ./shaders -type f -name "*.vert")
vertObjFiles = $(patsubst %.vert, %.vert.spv, $(vertSources))
fragSources = $(shell find ./shaders -type f -name "*.frag")
fragObjFiles = $(patsubst %.frag, %.frag.spv, $(fragSources))

TARGET = a.out
$(TARGET): $(vertObjFiles) $(fragObjFiles)
$(TARGET): *.cpp *.hpp
	g++ $(CFLAGS) -o $(TARGET) *.cpp $(LDFLAGS)

# Make shader targets
%.spv: %
	${GLSLC} $< -o $@

.PHONY: test clean

test: a.out
	./a.out

clean:
	rm -f a.out
	rm -f *.spv
