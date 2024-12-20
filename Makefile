CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

.PHONY: test clean

VulkanTest: main.cpp
	g++ $(CFLAGS) -o VulkanTest main.cpp $(LDFLAGS)

test: vert.spv frag.spv VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest vert.spv frag.spv

vert.spv: shader.vert
	glslc shader.vert -o vert.spv

frag.spv: shader.frag
	glslc shader.frag -o frag.spv
