#ifndef SCENE_HPP
#define SCENE_HPP

#include <vector>
#include "Mesh.hpp"
#include "Camera.hpp"
#include "Vulkan.hpp"

extern const int MAX_FRAMES_IN_FLIGHT;

class Scene {
public:
	std::vector<Mesh> meshes;
	Camera camera {60.0f, 0};
	Vulkan* vulkan;
	VRMImporter vrmImporter;
	std::vector<std::vector<uint8_t>> textureData;

	std::vector<VkImage> textureImages;
	std::vector<VkDeviceMemory> textureImageMemories;
	std::vector<VkImageView> textureImageViews;
	std::vector<VkSampler> textureSamplers;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	void load(const std::string& file, Vulkan* vulkan);
	void setup();
	void update(uint32_t currentImage, bool _keystates[400], double dt);
	void cleanup();
	void draw();
private:
	void updateCamera(double dt);
	void updateUniformBuffers(uint32_t currentImage);
	void handleKeystate(bool _keystates[400], double dt);

	void createTextureImages(size_t numTextures);
	void createTextureImageViews(size_t numTextures);
	void createTextureSamplers(size_t numTextures);

	void createDescriptorSets();
	void createDescriptorPools();
	void createDescriptorSetLayouts(size_t numTextures);

	void createAnimBuffers();
	void createMaterialBuffers();

	void createVertexBuffers();
	void createIndexBuffers();
	void createUniformBuffers();
};

#endif
