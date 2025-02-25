#include "Scene.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void Scene::update(uint32_t currentImage, bool keystates[400], double dt) {
	handleKeystate(keystates, dt);
	updateCamera(dt);
	updateUniformBuffers(currentImage);
	vrmImporter.recalculateMatrices();
}

void Scene::cleanup() {
	for (auto& mesh : meshes) {
		mesh.cleanup(*vulkan);
	}
	for (int i = 0; i < textureData.size(); i++) {
		vkDestroySampler(vulkan->device, textureSamplers[i], nullptr);
		vkDestroyImageView(vulkan->device, textureImageViews[i], nullptr);
		vkDestroyImage(vulkan->device, textureImages[i], nullptr);
		vkFreeMemory(vulkan->device, textureImageMemories[i], nullptr);
	}

	vkDestroyDescriptorPool(vulkan->device, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(vulkan->device, descriptorSetLayout, nullptr);
}

void Scene::load(const std::string& file, Vulkan* vulkan) {
	this->vulkan = vulkan;

	vrmImporter.loadModel(file);
	size_t meshCount = vrmImporter.getMeshCount();
	meshes.reserve(meshCount);

	for (size_t i = 0; i < meshCount; i++) {
		size_t primitiveCount = vrmImporter.getPrimitiveCount(i);
		for (size_t j = 0; j < primitiveCount; j++) {
			Mesh m;
			m.meshIndex = i;
			m.primitiveIndex = j;
			Array<glm::vec3> positions = vrmImporter.getMeshAttribute<glm::vec3>(i, j, "POSITION");
			Array<glm::vec3> normals = vrmImporter.getMeshAttribute<glm::vec3>(i, j, "NORMAL");
			Array<glm::vec2> texCoords = vrmImporter.getMeshAttribute<glm::vec2>(i, j, "TEXCOORD_0");
			Array<glm::u16vec4> joints = vrmImporter.getMeshAttribute<glm::u16vec4>(i, j, "JOINTS_0");
			Array<glm::vec4> weights = vrmImporter.getMeshAttribute<glm::vec4>(i, j, "WEIGHTS_0");
			m.vertices.reserve(positions.count);

			for (size_t k = 0; k < positions.count; k++) {
				Vertex v;
				v.pos = glm::vec3(positions.data[k].x, positions.data[k].z, positions.data[k].y);
				v.normal = normals.data[k];
				v.texCoord = texCoords.data[k];
				v.index = k;

				if (joints.count > 0)
					v.joints = glm::uvec4(joints.data[k].x, joints.data[k].y, joints.data[k].z, joints.data[k].w);

				if (weights.count > 0)
					v.weights = weights.data[k];

				m.vertices.push_back(v);
			}

			size_t animCount = vrmImporter.getMeshBlendShapeCount(i, j);
			m.anims.reserve(animCount);
			for (size_t k = 0; k < animCount; k++) {
				AnimMesh anim;
				Array<glm::vec3> vecs = vrmImporter.getMeshMorph<glm::vec3>(i, j, k, "POSITION");
				anim.verts.reserve(vecs.count);
				for (size_t l = 0; l < vecs.count; l++) {
					glm::vec3 vec = vecs.data[l];
					anim.verts.push_back(glm::vec4(vec.x, vec.y, vec.z, 1));
				}
				m.anims.push_back(anim);
			}

			Array<uint32_t> indices = vrmImporter.getMeshProperty<uint32_t>(i, j, "indices");
			m.indices.reserve(indices.count);
			for (size_t k = 0; k < indices.count; k++) {
				m.indices.push_back(indices.data[k]);
			}

			size_t materialIndex = vrmImporter.getMeshMaterialIndex(i, j);
			m.material = vrmImporter.getMaterial(materialIndex);
			meshes.push_back(m);
		}
	}

	size_t textureCount = vrmImporter.getTextureCount();
	textureData.reserve(textureCount);
	for (size_t i = 0; i < textureCount; i++) {
		VRM::TextureData data = vrmImporter.getTextureData(i);
		std::vector<uint8_t> temp;
		temp.reserve(data.byteLength);
		for (size_t j = 0; j < data.byteLength; j++) {
			char* ptr = data.begin + j;
			temp.push_back(*ptr);
		}
		textureData.push_back(temp);
	}

	vrmImporter.calculateJoints();
}

void Scene::setup() {
	size_t textureCount = textureData.size();
	createDescriptorSetLayouts(textureCount);

	createTextureImages(textureCount);
	createTextureImageViews(textureCount);
	createTextureSamplers(textureCount);

	createVertexBuffers();
	createIndexBuffers();
	createUniformBuffers();
	createMaterialBuffers();
	createAnimBuffers();
	createDescriptorPools();
	createDescriptorSets();

	const std::vector<VkDescriptorSetLayout> layouts = {
		meshes[0].descriptorSetLayout,
		descriptorSetLayout
	};

	vulkan->createGraphicsPipeline(layouts);
}

void Scene::draw() {
	VkCommandBuffer commandBuffer = vulkan->commandBuffers[vulkan->currentFrame];
	size_t frame = vulkan->currentFrame;
	for (const auto& mesh : meshes) {
		VkBuffer vBuffers[] = {mesh.vertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		VkDescriptorSet meshs = mesh.descriptorSets[frame];
		VkDescriptorSet scenes = descriptorSets[frame];
		std::array<VkDescriptorSet, 2> sets = {
		 	meshs,
		 	scenes
		};
		//VkDescriptorSet sets[2] = {mesh.descriptorSets[frame], descriptorSets[frame]};

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan->pipelineLayout, 0, sets.size(), sets.data(), 0, nullptr);

		float anim = 0.0;
		if (!mesh.anims.empty())
		 	anim = 1.0;
		PushConstants constants;
		constants.materialIndex = 0;
		constants.value = anim;
		constants.numVertices = mesh.vertices.size();

		vkCmdPushConstants(commandBuffer, vulkan->pipelineLayout,  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &constants);
		//vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
	}
}

void Scene::updateCamera(double dt) {
	camera.SetAspect(float(vulkan->swapChainExtent.width) / float(vulkan->swapChainExtent.height));
	glm::vec3 direction{};
	direction.x = cos(glm::radians(camera.m_Yaw)) * cos(glm::radians(camera.m_Pitch));
	direction.y = sin(glm::radians(camera.m_Pitch));
	direction.z = sin(glm::radians(camera.m_Yaw)) * cos(glm::radians(camera.m_Pitch));
	camera.m_Front = glm::normalize(direction);
	camera.m_View = glm::lookAt(camera.m_Position, camera.m_Position + camera.m_Front, camera.m_Up);
}

void Scene::updateUniformBuffers(uint32_t currentImage) {
	for (auto& mesh : meshes) {
		mesh.updateUniformBuffer(camera, currentImage);
	}
}

void Scene::handleKeystate(bool keystates[400], double dt) {
	float speedMult;
	if (keystates[340]) { // This is Left Shift for some reason
		speedMult = 16;
	} else {
		speedMult = 4;
	}
	float cameraSpeed = dt * speedMult;
	float rotationSpeed = 10 * cameraSpeed / speedMult;
	if (keystates[int('W')])
		camera.m_Position += cameraSpeed * camera.m_Front;
	if (keystates[int('A')])
		camera.m_Position -= glm::normalize(glm::cross(camera.m_Front, camera.m_Up)) * cameraSpeed;
	if (keystates[int('S')])
		camera.m_Position -= cameraSpeed * camera.m_Front;
	if (keystates[int('D')])
		camera.m_Position += glm::normalize(glm::cross(camera.m_Front, camera.m_Up)) * cameraSpeed;
	if (keystates[int('Q')]) // Directly down
		camera.m_Position -= camera.m_Up * cameraSpeed;
	if (keystates[int('E')]) // Directly up
		camera.m_Position += camera.m_Up * cameraSpeed;

	if (keystates[int('I')]) // Look up
		camera.m_Pitch += rotationSpeed * speedMult;
	if (keystates[int('J')]) { // Look left
		camera.m_Yaw -= rotationSpeed * speedMult;
		if (camera.m_Yaw <= -360)
			camera.m_Yaw = 0;
	}
	if (keystates[int('K')]) // Look down
		camera.m_Pitch -= rotationSpeed * speedMult;
	if (keystates[int('L')]) { // Look right
		camera.m_Yaw += rotationSpeed * speedMult;
		if (camera.m_Yaw >= 360)
			camera.m_Yaw = 0;
	}
}

void Scene::createDescriptorSetLayouts(size_t numTextures) {
	for (auto& mesh : meshes) {
		mesh.createDescriptorSetLayout(*vulkan);
	}

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorCount = numTextures;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 1> bindings = {samplerLayoutBinding};

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(vulkan->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("[Scene#createDescriptorSetLayout]: Error: Failed to create descriptor set layout!");
	}
}

void Scene::createTextureImages(size_t numTextures) {
	textureImages.resize(numTextures);
	textureImageMemories.resize(numTextures);
	for (int i = 0; i < numTextures; i++) {
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load_from_memory(textureData[i].data(), textureData[i].size(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::runtime_error("[Scene#createTextureImage]: Error: Failed to load texture image!");
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		vulkan->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(vulkan->device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(vulkan->device, stagingBufferMemory);
		stbi_image_free(pixels);

		vulkan->createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImages[i], textureImageMemories[i]);

		vulkan->transitionImageLayout(textureImages[i], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		vulkan->copyBufferToImage(stagingBuffer, textureImages[i], static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		vulkan->transitionImageLayout(textureImages[i], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(vulkan->device, stagingBuffer, nullptr);
		vkFreeMemory(vulkan->device, stagingBufferMemory, nullptr);
	}
}

void Scene::createTextureImageViews(size_t numTextures) {
	textureImageViews.reserve(numTextures);
	for (int i = 0; i < numTextures; i++) {
		VkImageView view = vulkan->createImageView(textureImages[i], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
		assert(view != VK_NULL_HANDLE);
		textureImageViews.push_back(view);
	}
}

void Scene::createTextureSamplers(size_t numTextures) {
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(vulkan->physicalDevice, &properties);
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	textureSamplers.reserve(numTextures);
	for (int i = 0; i < numTextures; i++) {
		VkSampler sampler;
		if (vkCreateSampler(vulkan->device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
			throw std::runtime_error("[Scene#createTextureSampler]: Error: Failed to create texture sampler!");
		}
		textureSamplers.push_back(sampler);
	}
}

void Scene::createDescriptorSets() {
	for (auto& mesh : meshes) {
		mesh.createDescriptorSets(*vulkan);
	}

	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(vulkan->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("[HelloTriangleApplication#createDescriptorSet]: Error: Failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		std::vector<VkWriteDescriptorSet> descriptorWrites(1);
		std::vector<VkDescriptorImageInfo> imageInfos(textureImages.size());

		for (int j = 0; j < textureImages.size(); j++) {
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			assert(textureImageViews[j] != VK_NULL_HANDLE);
			imageInfo.imageView = textureImageViews[j];
			imageInfo.sampler = textureSamplers[j];
			imageInfos[j] = (imageInfo);
		}

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = imageInfos.size();
		descriptorWrites[0].pImageInfo = imageInfos.data();

		vkUpdateDescriptorSets(vulkan->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Scene::createDescriptorPools() {
	for (auto& mesh : meshes) {
		mesh.createDescriptorPool(*vulkan);
	}

	std::array<VkDescriptorPoolSize, 1> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * textureData.size());
	//poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	//poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	// poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	// poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(vulkan->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("[HelloTriangleApplication#createDescriptorPool]: Error: Failed to create descriptor pool!");
	}
}

void Scene::createVertexBuffers() {
	for (auto& mesh : meshes) {
		mesh.createVertexBuffer(*vulkan);
	}
}

void Scene::createIndexBuffers() {
	for (auto& mesh : meshes) {
		mesh.createIndexBuffer(*vulkan);
	}
}

void Scene::createUniformBuffers() {
	for (auto& mesh : meshes) {
		mesh.createUniformBuffers(*vulkan);
	}
}

void Scene::createMaterialBuffers() {
	for (auto& mesh : meshes) {
		mesh.createMaterialBuffers(*vulkan);
	}
}

void Scene::createAnimBuffers() {
	for (auto& mesh : meshes) {
		mesh.createAnimBuffers(*vulkan);
	}
}

