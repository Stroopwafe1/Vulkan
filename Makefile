CFLAGS = -std=c++17 -g -Og
LDFLAGS = -lglfw -lvulkan -ldl -lpthread

SOURCES = main.cpp Camera.cpp Mesh.cpp Vulkan.cpp Application.cpp importer/VRMImporter.cpp Scene.cpp

DEPENDENCIES = $(SOURCES) Camera.hpp Mesh.hpp Application.hpp importer/VRMImporter.hpp Scene.hpp structs.hpp

.PHONY: test clean

VulkanTest: $(DEPENDENCIES)
	g++ $(CFLAGS) -o VulkanTest $(SOURCES) $(LDFLAGS)

test: vert.spv frag.spv VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest vert.spv frag.spv

vert.spv: shader.vert
	glslc shader.vert -o vert.spv

frag.spv: shader.frag
	glslc shader.frag -o frag.spv
