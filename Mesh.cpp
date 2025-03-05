#include "Mesh.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <stdexcept>

void Mesh::updateUniformBuffer(const Camera& camera, uint32_t currentImage) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), (float)glm::radians(90.0), glm::vec3(-1, 0, 0));
	ubo.view = camera.m_View;
	ubo.proj = camera.m_Projection;
	ubo.proj[1][1] *= -1; // Compensate for OpenGL being y upside-down
	ubo.time = time;
	memcpy(m_UniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void Mesh::createVertexBuffer(Vulkan& vulkan) {
	VkDeviceSize bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vulkan.m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, m_Vertices.data(), (size_t) bufferSize);
	vkUnmapMemory(vulkan.m_Device, stagingBufferMemory);

	vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory);
	vulkan.copyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);
	//vkBindBufferMemory(vulkan.device, vertexBuffer, vertexBufferMemory, 0);

	vkDestroyBuffer(vulkan.m_Device, stagingBuffer, nullptr);
	vkFreeMemory(vulkan.m_Device, stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer(Vulkan& vulkan) {

	VkDeviceSize bufferSize = sizeof(m_Indices[0]) * m_Indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vulkan.m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, m_Indices.data(), (size_t) bufferSize);
	vkUnmapMemory(vulkan.m_Device, stagingBufferMemory);

	vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory);

	vulkan.copyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

	vkDestroyBuffer(vulkan.m_Device, stagingBuffer, nullptr);
	vkFreeMemory(vulkan.m_Device, stagingBufferMemory, nullptr);

}

void Mesh::createUniformBuffers(Vulkan& vulkan) {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	m_UniformBuffers.resize(g_MAX_FRAMES_IN_FLIGHT);
	m_UniformBuffersMemory.resize(g_MAX_FRAMES_IN_FLIGHT);
	m_UniformBuffersMapped.resize(g_MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < g_MAX_FRAMES_IN_FLIGHT; i++) {
		vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniformBuffers[i], m_UniformBuffersMemory[i]);

		vkMapMemory(vulkan.m_Device, m_UniformBuffersMemory[i], 0, bufferSize, 0, &m_UniformBuffersMapped[i]);
	}
}

void Mesh::createMaterialBuffers(Vulkan& vulkan) {
	VkDeviceSize bufferSize = sizeof(VRM::Material);

	m_MaterialBuffers.resize(g_MAX_FRAMES_IN_FLIGHT);
	m_MaterialBuffersMemory.resize(g_MAX_FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < g_MAX_FRAMES_IN_FLIGHT; i++) {
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(vulkan.m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, &m_Material, (size_t) bufferSize);
		vkUnmapMemory(vulkan.m_Device, stagingBufferMemory);

		vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_MaterialBuffers[i], m_MaterialBuffersMemory[i]);

		vulkan.copyBuffer(stagingBuffer, m_MaterialBuffers[i], bufferSize);

		vkDestroyBuffer(vulkan.m_Device, stagingBuffer, nullptr);
		vkFreeMemory(vulkan.m_Device, stagingBufferMemory, nullptr);
	}
}

void Mesh::createAnimBuffers(Vulkan& vulkan) {
	if (m_Anims.empty()) {
		return;
	}
	VkDeviceSize bufferSize = m_Anims[0].verts.size() * sizeof(glm::vec4) * m_Anims.size();

	m_AnimBuffers.resize(g_MAX_FRAMES_IN_FLIGHT);
	m_AnimBuffersMemory.resize(g_MAX_FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < g_MAX_FRAMES_IN_FLIGHT; i++) {
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		size_t elementSize = m_Anims[0].verts.size() * sizeof(glm::vec4);
		void* data;
		vkMapMemory(vulkan.m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);

		for (int j = 0; j < m_Anims.size(); j++)
			memcpy((uint8_t*)data + elementSize * j, m_Anims[j].verts.data(), elementSize);
		vkUnmapMemory(vulkan.m_Device, stagingBufferMemory);

		vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_AnimBuffers[i], m_AnimBuffersMemory[i]);

		vulkan.copyBuffer(stagingBuffer, m_AnimBuffers[i], bufferSize);

		vkDestroyBuffer(vulkan.m_Device, stagingBuffer, nullptr);
		vkFreeMemory(vulkan.m_Device, stagingBufferMemory, nullptr);
	}
}

void Mesh::createDescriptorSetLayout(Vulkan& vulkan) {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding materialLayoutBinding{};
	materialLayoutBinding.binding = 1;
	materialLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	materialLayoutBinding.descriptorCount = 1;
	materialLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	materialLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding animLayoutBinding{};
	animLayoutBinding.binding = 2;
	animLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	animLayoutBinding.descriptorCount = 1;
	animLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	animLayoutBinding.pImmutableSamplers = nullptr; // Optional

	std::array<VkDescriptorSetLayoutBinding, 3> bindings = {uboLayoutBinding, materialLayoutBinding, animLayoutBinding};

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(vulkan.m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("[Scene#createDescriptorSetLayout]: Error: Failed to create descriptor set layout!");
	}
}

void Mesh::createDescriptorSets(Vulkan& vulkan) {
	std::vector<VkDescriptorSetLayout> layouts(g_MAX_FRAMES_IN_FLIGHT, m_DescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(g_MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	m_DescriptorSets.resize(g_MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(vulkan.m_Device, &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("[HelloTriangleApplication#createDescriptorSet]: Error: Failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < g_MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_UniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		std::vector<VkWriteDescriptorSet> descriptorWrites(3);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_DescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		VkDescriptorBufferInfo materialBufferInfo{};
		materialBufferInfo.buffer = m_MaterialBuffers[i];
		materialBufferInfo.offset = 0;
		materialBufferInfo.range = sizeof(VRM::Material);

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_DescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &materialBufferInfo;

		VkDescriptorBufferInfo animBufferInfo{};
		animBufferInfo.offset = 0;
		if (!m_Anims.empty()) {
			animBufferInfo.buffer = m_AnimBuffers[i];
			animBufferInfo.range = m_Anims[0].verts.size() * sizeof(glm::vec4) * m_Anims.size();
		} else {
			animBufferInfo.buffer = m_MaterialBuffers[i];
			animBufferInfo.range = 1;
		}

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = m_DescriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &animBufferInfo;


		vkUpdateDescriptorSets(vulkan.m_Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Mesh::createDescriptorPool(Vulkan& vulkan) {
	std::array<VkDescriptorPoolSize, 3> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(g_MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(g_MAX_FRAMES_IN_FLIGHT);
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[2].descriptorCount = static_cast<uint32_t>(g_MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(g_MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(vulkan.m_Device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("[HelloTriangleApplication#createDescriptorPool]: Error: Failed to create descriptor pool!");
	}
}

void Mesh::cleanup(Vulkan& vulkan) {
	for (size_t i = 0; i < g_MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(vulkan.m_Device, m_UniformBuffers[i], nullptr);
		vkFreeMemory(vulkan.m_Device, m_UniformBuffersMemory[i], nullptr);

		vkDestroyBuffer(vulkan.m_Device, m_MaterialBuffers[i], nullptr);
		vkFreeMemory(vulkan.m_Device, m_MaterialBuffersMemory[i], nullptr);

		if (!m_Anims.empty()) {
			vkDestroyBuffer(vulkan.m_Device, m_AnimBuffers[i], nullptr);
			vkFreeMemory(vulkan.m_Device, m_AnimBuffersMemory[i], nullptr);
		}
	}

	vkDestroyBuffer(vulkan.m_Device, m_IndexBuffer, nullptr);
	vkFreeMemory(vulkan.m_Device, m_IndexBufferMemory, nullptr);

	vkDestroyBuffer(vulkan.m_Device, m_VertexBuffer, nullptr);
	vkFreeMemory(vulkan.m_Device, m_VertexBufferMemory, nullptr);

	vkDestroyDescriptorPool(vulkan.m_Device, m_DescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(vulkan.m_Device, m_DescriptorSetLayout, nullptr);
}
