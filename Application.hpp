#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const int MAX_FRAMES_IN_FLIGHT = 2;

#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>

#include "Camera.hpp"
#include "structs.hpp"
#include "Mesh.hpp"
#include "Vulkan.hpp"

#include "importer/VRMImporter.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <chrono>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class Application {
public:
	void run() {
		initWindow();
		loadScene();
		initVulkan();
		mainLoop();
		cleanup();
	}
public:
	GLFWwindow* window;
	VRMImporter vrmImporter;
	Vulkan vulkan;

	std::vector<Mesh> meshes;
	std::vector<std::vector<uint8_t>> textureData;

	Township::Camera camera {60.0f, 0};
	double _currentTime{};
	double _deltaTime{};
	float time;
	bool _keystates[400]{};
public:
	void initWindow();

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		app->_keystates[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		app->vulkan.invalidated = true;
	}

	void updateWindowSize(int* width, int* height);
	void initVulkan();
	void mainLoop();
	void updateCamera();
	void updateUniformBuffers(uint32_t currentImage);
	void MovementHandler();
	void loadScene();
	void createVertexBuffers();
	void createIndexBuffers();
	void createUniformBuffers();
	void createMaterialBuffers();
	void createAnimBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void cleanup();
};

#endif
