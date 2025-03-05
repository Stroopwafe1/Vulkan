#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Camera.hpp"
#include "structs.hpp"
#include "Mesh.hpp"
#include "Vulkan.hpp"
#include "Scene.hpp"

#include "importer/VRMImporter.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

const uint32_t g_WIDTH = 800;
const uint32_t g_HEIGHT = 600;

class Application {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}
public:
	GLFWwindow* window;
	Vulkan vulkan;
	Scene scene;

	double _currentTime{};
	double _deltaTime{};
	bool _keystates[400]{};
public:
	void initWindow();

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		app->_keystates[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		app->vulkan.invalidate(width, height);
	}

	void initVulkan();
	void createSurface();
	void mainLoop();
	void loadScene();
	void cleanup();
};

#endif
