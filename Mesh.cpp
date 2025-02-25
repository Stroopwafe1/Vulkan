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
	memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void Mesh::createVertexBuffer(Vulkan& vulkan) {
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vulkan.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t) bufferSize);
	vkUnmapMemory(vulkan.device, stagingBufferMemory);

	vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
	vulkan.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
	//vkBindBufferMemory(vulkan.device, vertexBuffer, vertexBufferMemory, 0);

	vkDestroyBuffer(vulkan.device, stagingBuffer, nullptr);
	vkFreeMemory(vulkan.device, stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer(Vulkan& vulkan) {

	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vulkan.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t) bufferSize);
	vkUnmapMemory(vulkan.device, stagingBufferMemory);

	vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	vulkan.copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(vulkan.device, stagingBuffer, nullptr);
	vkFreeMemory(vulkan.device, stagingBufferMemory, nullptr);

}

void Mesh::createUniformBuffers(Vulkan& vulkan) {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

		vkMapMemory(vulkan.device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
	}
}

void Mesh::createMaterialBuffers(Vulkan& vulkan) {
	VkDeviceSize bufferSize = sizeof(VRM::Material);

	materialBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	materialBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(vulkan.device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, &material, (size_t) bufferSize);
		vkUnmapMemory(vulkan.device, stagingBufferMemory);

		vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, materialBuffers[i], materialBuffersMemory[i]);

		vulkan.copyBuffer(stagingBuffer, materialBuffers[i], bufferSize);

		vkDestroyBuffer(vulkan.device, stagingBuffer, nullptr);
		vkFreeMemory(vulkan.device, stagingBufferMemory, nullptr);
	}
}

void Mesh::createAnimBuffers(Vulkan& vulkan) {
	if (anims.empty()) {
		return;
	}
	VkDeviceSize bufferSize = anims[0].verts.size() * sizeof(glm::vec4) * anims.size();

	animBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	animBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		size_t elementSize = anims[0].verts.size() * sizeof(glm::vec4);
		void* data;
		vkMapMemory(vulkan.device, stagingBufferMemory, 0, bufferSize, 0, &data);

		for (int j = 0; j < anims.size(); j++)
			memcpy((uint8_t*)data + elementSize * j, anims[j].verts.data(), elementSize);
		vkUnmapMemory(vulkan.device, stagingBufferMemory);

		vulkan.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, animBuffers[i], animBuffersMemory[i]);

		vulkan.copyBuffer(stagingBuffer, animBuffers[i], bufferSize);

		vkDestroyBuffer(vulkan.device, stagingBuffer, nullptr);
		vkFreeMemory(vulkan.device, stagingBufferMemory, nullptr);
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

	if (vkCreateDescriptorSetLayout(vulkan.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("[Scene#createDescriptorSetLayout]: Error: Failed to create descriptor set layout!");
	}
}

void Mesh::createDescriptorSets(Vulkan& vulkan) {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(vulkan.device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("[HelloTriangleApplication#createDescriptorSet]: Error: Failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		std::vector<VkWriteDescriptorSet> descriptorWrites(3);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		VkDescriptorBufferInfo materialBufferInfo{};
		materialBufferInfo.buffer = materialBuffers[i];
		materialBufferInfo.offset = 0;
		materialBufferInfo.range = sizeof(VRM::Material);

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &materialBufferInfo;

		VkDescriptorBufferInfo animBufferInfo{};
		animBufferInfo.offset = 0;
		if (!anims.empty()) {
			animBufferInfo.buffer = animBuffers[i];
			animBufferInfo.range = anims[0].verts.size() * sizeof(glm::vec4) * anims.size();
		} else {
			animBufferInfo.buffer = materialBuffers[i];
			animBufferInfo.range = 1;
		}

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = descriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &animBufferInfo;


		vkUpdateDescriptorSets(vulkan.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Mesh::createDescriptorPool(Vulkan& vulkan) {
	std::array<VkDescriptorPoolSize, 3> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(vulkan.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("[HelloTriangleApplication#createDescriptorPool]: Error: Failed to create descriptor pool!");
	}
}

void Mesh::cleanup(Vulkan& vulkan) {
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(vulkan.device, uniformBuffers[i], nullptr);
		vkFreeMemory(vulkan.device, uniformBuffersMemory[i], nullptr);

		vkDestroyBuffer(vulkan.device, materialBuffers[i], nullptr);
		vkFreeMemory(vulkan.device, materialBuffersMemory[i], nullptr);

		if (!anims.empty()) {
			vkDestroyBuffer(vulkan.device, animBuffers[i], nullptr);
			vkFreeMemory(vulkan.device, animBuffersMemory[i], nullptr);
		}
	}

	vkDestroyBuffer(vulkan.device, indexBuffer, nullptr);
	vkFreeMemory(vulkan.device, indexBufferMemory, nullptr);

	vkDestroyBuffer(vulkan.device, vertexBuffer, nullptr);
	vkFreeMemory(vulkan.device, vertexBufferMemory, nullptr);

	vkDestroyDescriptorPool(vulkan.device, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(vulkan.device, descriptorSetLayout, nullptr);
}
