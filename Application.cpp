#include "Application.hpp"


void Application::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Application::updateWindowSize(int* width, int* height) {
	glfwGetFramebufferSize(window, width, height);

	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, width, height);
		glfwWaitEvents();
	}
}

void Application::initVulkan() {
	vulkan.init(this, textureData);
	createVertexBuffers();
	createIndexBuffers();
	createUniformBuffers();
	createMaterialBuffers();
	createAnimBuffers();
	Mesh::createBoneBuffers(vrmImporter.getBoneCount(), vulkan);
	createDescriptorPool();
	createDescriptorSets();
}

void Application::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		double time_now = glfwGetTime();
		_deltaTime = time_now - _currentTime;
		_currentTime = time_now;

		glfwPollEvents();
		updateCamera();
		updateUniformBuffers(vulkan.currentFrame);
		updateBoneTransforms(vulkan.currentFrame);
		vulkan.drawFrame(this);
	}

	vulkan.deviceWaitIdle();
}

void Application::updateCamera() {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	camera.SetAspect(float(vulkan.swapChainExtent.width) / float(vulkan.swapChainExtent.height));
	glm::vec3 direction{};
	direction.x = cos(glm::radians(camera.m_Yaw)) * cos(glm::radians(camera.m_Pitch));
	direction.y = sin(glm::radians(camera.m_Pitch));
	direction.z = sin(glm::radians(camera.m_Yaw)) * cos(glm::radians(camera.m_Pitch));
	camera.m_Front = glm::normalize(direction);
	camera.m_View = glm::lookAt(camera.m_Position, camera.m_Position + camera.m_Front, camera.m_Up);
	MovementHandler();
}

void Application::updateUniformBuffers(uint32_t currentImage) {
	for (auto& mesh : meshes) {
		mesh.updateUniformBuffer(camera, currentImage);
	}
}

void Application::updateBoneTransforms(uint32_t currentImage) {
	Array<glm::mat4> boneTransforms = vrmImporter.getBoneTransforms();
	Mesh::updateBoneBuffers(boneTransforms.data, boneTransforms.count, currentImage);
}

void Application::MovementHandler() {
	float speedMult;
	if (_keystates[340]) { // This is Left Shift for some reason
		speedMult = 16;
	} else {
		speedMult = 4;
	}
	float cameraSpeed = _deltaTime * speedMult;
	float rotationSpeed = 10 * cameraSpeed / speedMult;
	if (_keystates[int('W')])
		camera.m_Position += cameraSpeed * camera.m_Front;
	if (_keystates[int('A')])
		camera.m_Position -= glm::normalize(glm::cross(camera.m_Front, camera.m_Up)) * cameraSpeed;
	if (_keystates[int('S')])
		camera.m_Position -= cameraSpeed * camera.m_Front;
	if (_keystates[int('D')])
		camera.m_Position += glm::normalize(glm::cross(camera.m_Front, camera.m_Up)) * cameraSpeed;
	if (_keystates[int('Q')]) // Directly down
		camera.m_Position -= camera.m_Up * cameraSpeed;
	if (_keystates[int('E')]) // Directly up
		camera.m_Position += camera.m_Up * cameraSpeed;

	if (_keystates[int('I')]) // Look up
		camera.m_Pitch += rotationSpeed * speedMult;
	if (_keystates[int('J')]) { // Look left
		camera.m_Yaw -= rotationSpeed * speedMult;
		if (camera.m_Yaw <= -360)
			camera.m_Yaw = 0;
	}
	if (_keystates[int('K')]) // Look down
		camera.m_Pitch -= rotationSpeed * speedMult;
	if (_keystates[int('L')]) { // Look right
		camera.m_Yaw += rotationSpeed * speedMult;
		if (camera.m_Yaw >= 360)
			camera.m_Yaw = 0;
	}

	// if (_keystates[int('S')] && _keystates[340]) {
	// 	std::cout << "Camera: " << std::endl;
	// 	std::cout << "\tPosition: " << camera.m_Position.x << ", " << camera.m_Position.y << ", " << camera.m_Position.z << std::endl;
	// 	std::cout << "\tFront: " << camera.m_Front.x << ", " << camera.m_Front.y << ", " << camera.m_Front.z << std::endl;
	// 	std::cout << "\tYaw: " << camera.m_Yaw << std::endl;
	// 	std::cout << "\tPitch: " << camera.m_Pitch << std::endl;
	// }
}

void Application::loadScene() {
	vrmImporter.loadModel("Evelynn.vrm");

	size_t meshCount = vrmImporter.getMeshCount();
	meshes.reserve(meshCount);

	for (size_t i = 0; i < meshCount; i++) {
		size_t primitiveCount = vrmImporter.getPrimitiveCount(i);
		for (size_t j = 0; j < primitiveCount; j++) {
			Mesh m;
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
}

void Application::createVertexBuffers() {
	for (auto& mesh : meshes) {
		mesh.createVertexBuffer(vulkan);
	}
}

void Application::createIndexBuffers() {
	for (auto& mesh : meshes) {
		mesh.createIndexBuffer(vulkan);
	}
}

void Application::createUniformBuffers() {
	for (auto& mesh : meshes) {
		mesh.createUniformBuffers(vulkan);
	}
}

void Application::createMaterialBuffers() {
	for (auto& mesh : meshes) {
		mesh.createMaterialBuffer(vulkan);
	}
}

void Application::createAnimBuffers() {
	for (auto& mesh : meshes) {
		mesh.createAnimBuffer(vulkan);
	}
}

void Application::createDescriptorPool() {
	for (auto& mesh : meshes) {
		mesh.createDescriptorPool(vulkan);
	}
}

void Application::createDescriptorSets() {
	for (auto& mesh : meshes) {
		mesh.createDescriptorSets(vulkan);
	}
}

void Application::cleanup() {
	for (auto& mesh : meshes) {
		mesh.cleanup(vulkan);
	}
	Mesh::cleanup_s(vulkan);
	vulkan.cleanup(vrmImporter.getTextureCount());
	glfwDestroyWindow(window);
	glfwTerminate();
}
