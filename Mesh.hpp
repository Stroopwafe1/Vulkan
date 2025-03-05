#ifndef MESH_HPP
#define MESH_HPP

#include <vulkan/vulkan_core.h>
#include <vector>
#include "structs.hpp"
#include "Camera.hpp"
#include "Vulkan.hpp"
#include "importer/VRMImporter.hpp"

extern const int g_MAX_FRAMES_IN_FLIGHT;

class Mesh {
public:
	void updateUniformBuffer(const Camera& camera, uint32_t currentImage);
	void createVertexBuffer(Vulkan& vulkan);
	void createIndexBuffer(Vulkan& vulkan);
	void createUniformBuffers(Vulkan& vulkan);
	void createMaterialBuffers(Vulkan& vulkan);
	void createAnimBuffers(Vulkan& vulkan);
	void cleanup(Vulkan& vulkan);

	void createDescriptorSets(Vulkan& vulkan);
	void createDescriptorSetLayout(Vulkan& vulkan);
	void createDescriptorPool(Vulkan& vulkan);

public:
	int m_MeshIndex;
	int m_PrimitiveIndex;

	std::vector<Vertex> m_Vertices;
	std::vector<uint32_t> m_Indices;
	VRM::Material m_Material;
	std::vector<AnimMesh> m_Anims;
	std::vector<glm::mat4> m_Joints;

	VkBuffer m_VertexBuffer;
	VkDeviceMemory m_VertexBufferMemory;
	VkBuffer m_IndexBuffer;
	VkDeviceMemory m_IndexBufferMemory;

	std::vector<VkBuffer> m_UniformBuffers;
	std::vector<VkDeviceMemory> m_UniformBuffersMemory;
	std::vector<void*> m_UniformBuffersMapped;

	std::vector<VkBuffer> m_MaterialBuffers;
	std::vector<VkDeviceMemory> m_MaterialBuffersMemory;

	std::vector<VkBuffer> m_AnimBuffers;
	std::vector<VkDeviceMemory> m_AnimBuffersMemory;

	VkDescriptorPool m_DescriptorPool;
	std::vector<VkDescriptorSet> m_DescriptorSets;
	VkDescriptorSetLayout m_DescriptorSetLayout;
};

#endif
