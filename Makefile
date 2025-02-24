CFLAGS = -std=c++17 -g -Og
LDFLAGS = -lglfw -lvulkan -ldl -lpthread

.PHONY: test clean

VulkanTest: main.cpp Camera.hpp Camera.cpp Mesh.cpp Vulkan.cpp Application.hpp Application.cpp importer/VRMImporter.hpp importer/VRMImporter.cpp
	g++ $(CFLAGS) -o VulkanTest main.cpp Camera.cpp Mesh.cpp Vulkan.cpp Application.cpp importer/VRMImporter.cpp $(LDFLAGS)

test: vert.spv frag.spv VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest vert.spv frag.spv

vert.spv: shader.vert
	glslc shader.vert -o vert.spv

frag.spv: shader.frag
	glslc shader.frag -o frag.spv
