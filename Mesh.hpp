#ifndef MESH_HPP
#define MESH_HPP

#include <vulkan/vulkan_core.h>
#include <vector>
#include "structs.hpp"
#include "Camera.hpp"
#include "Vulkan.hpp"
#include "importer/VRMImporter.hpp"

extern const int MAX_FRAMES_IN_FLIGHT;

class Mesh {
public:
	void updateUniformBuffer(const Township::Camera& camera, uint32_t currentImage);
	void createDescriptorSets(Vulkan& vulkan);
	void createDescriptorPool(Vulkan& vulkan);
	void createVertexBuffer(Vulkan& vulkan);
	void createIndexBuffer(Vulkan& vulkan);
	void createUniformBuffers(Vulkan& vulkan);
	void createAnimBuffer(Vulkan& vulkan);
	void createMaterialBuffer(Vulkan& vulkan);
	void cleanup(Vulkan& vulkan);

public:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	VRM::Material material;
	std::vector<AnimMesh> anims;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	std::vector<VkBuffer> materialBuffers;
	std::vector<VkDeviceMemory> materialBuffersMemory;
	std::vector<VkBuffer> animBuffers;
	std::vector<VkDeviceMemory> animBuffersMemory;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
};

#endif
