#include "Application.hpp"

void Application::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Application::initVulkan() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	vulkan.init(extensions, WIDTH, HEIGHT);
	createSurface();
	loadScene();
	vulkan.init2();
	scene.setup();
	vulkan.setup();
}

void Application::createSurface() {
	if (glfwCreateWindowSurface(vulkan.instance, window, nullptr, &vulkan.surface) != VK_SUCCESS) {
		throw std::runtime_error("[Vulkan#createSurface]: Error: Failed to create window surface!");
	}
}

void Application::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		double time_now = glfwGetTime();
		_deltaTime = time_now - _currentTime;
		_currentTime = time_now;

		glfwPollEvents();
		scene.update(vulkan.currentFrame, _keystates, _deltaTime);
		uint32_t imageIndex;
		vulkan.beginDrawFrame(&imageIndex);
		scene.draw();
		vulkan.endDrawFrame(&imageIndex);
	}

	vulkan.deviceWaitIdle();
}

void Application::loadScene() {
	scene.load("Evelynn.vrm", &vulkan);
}

void Application::cleanup() {
	scene.cleanup();
	vulkan.cleanup();
	glfwDestroyWindow(window);
	glfwTerminate();
}
