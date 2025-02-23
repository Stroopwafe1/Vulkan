#ifndef VULKAN_HPP
#define VULKAN_HPP

#include <vulkan/vulkan_core.h>
#include <vector>
#include <iostream>
#include "structs.hpp"

class Application;

extern const int MAX_FRAMES_IN_FLIGHT;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME,
	VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
	VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
	VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
	VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
	VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
	VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
	VK_KHR_MAINTENANCE2_EXTENSION_NAME,
	VK_KHR_MULTIVIEW_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
const bool validateExtensions = false;
#else
const bool enableValidationLayers = true;
const bool validateExtensions = true;
#endif

class Vulkan {
public:
	uint32_t currentFrame = 0;
	bool invalidated = false;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	VkSurfaceKHR surface;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	std::vector<VkImage> textureImages;
	std::vector<VkDeviceMemory> textureImageMemories;
	std::vector<VkImageView> textureImageViews;
	std::vector<VkSampler> textureSamplers;

	PFN_vkCmdSetDepthTestEnableEXT pvkCmdSetDepthTestEnableEXT;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	void init(Application* app, const std::vector<std::vector<uint8_t>>& textureData);
	void deviceWaitIdle();
	bool checkExtensions(const std::vector<const char*>& extensions);
	std::vector<const char*> getRequiredExtensions();
	bool checkValidationLayerSupport();
	void drawFrame(Application* app);
	void createInstance();
	void loadExtensionFunctions();
	void createSurface(Application* app);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, Application* app);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain(Application* app);
	void cleanupSwapChain();
	void recreateSwapChain(Application* app);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void createImageViews();
	void createRenderPass();
	void createDescriptorSetLayout(size_t numTextures);
	VkShaderModule createShaderModule(const std::vector<char>& code);
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPool();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, Application* app);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	bool hasStencilComponent(VkFormat format);
	void createDepthResources();
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void createTextureImages(size_t numTextures, const std::vector<std::vector<uint8_t>>& data);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void createTextureImageViews(size_t numTextures);
	void createTextureSamplers(size_t numTextures);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void createCommandBuffers();
	void createSyncObjects();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void setupDebugMessenger();
	void cleanup(size_t numTextures);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {
			std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

			return VK_FALSE;
		}

	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		} else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}
};

#endif
