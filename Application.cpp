#include "Application.hpp"


void Application::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);

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
			m.vertices.reserve(positions.count);

			for (size_t k = 0; k < positions.count; k++) {
				Vertex v;
				v.pos = glm::vec3(positions.data[k].x, positions.data[k].z, positions.data[k].y);
				v.normal = normals.data[k];
				v.texCoord = texCoords.data[k];
				v.index = k;
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
	/*
	importer.SetPropertyInteger(AI_CONFIG_IMPORT_REMOVE_EMPTY_BONES, 0);
	scene = importer.ReadFile("Evelynn.vrm", aiProcess_JoinIdenticalVertices);
	if (scene == nullptr) {
		throw std::runtime_error("[Application#loadScene]: Error: Failed to load scene!");
	}

	meshes.reserve(scene->mNumMeshes);
	std::vector<Material> materials(scene->mNumMaterials);

	for (int i = 0; i < scene->mNumMaterials; i++) {
		aiMaterial* mat = scene->mMaterials[i];
		std::cout << "Material " << i << " has " << mat->mNumProperties << " properties:" << std::endl;

		mat->Get(AI_MATKEY_COLOR_DIFFUSE, materials[i].diffuse);
		mat->Get(AI_MATKEY_COLOR_SPECULAR, materials[i].specular);
		mat->Get(AI_MATKEY_COLOR_AMBIENT, materials[i].ambient);
		mat->Get(AI_MATKEY_COLOR_EMISSIVE, materials[i].emissive);
		mat->Get(AI_MATKEY_COLOR_TRANSPARENT, materials[i].transparent);
		mat->Get(AI_MATKEY_COLOR_REFLECTIVE, materials[i].reflective);

		mat->Get(AI_MATKEY_REFLECTIVITY, materials[i].reflectivity);
		mat->Get(AI_MATKEY_ENABLE_WIREFRAME, materials[i].wireframe);
		mat->Get(AI_MATKEY_TWOSIDED, materials[i].twosided);
		mat->Get(AI_MATKEY_SHADING_MODEL, materials[i].shading_model);
		mat->Get(AI_MATKEY_BLEND_FUNC, materials[i].blend_func);
		mat->Get(AI_MATKEY_OPACITY, materials[i].opacity);
		mat->Get(AI_MATKEY_SHININESS, materials[i].shininess);
		mat->Get(AI_MATKEY_SHININESS_STRENGTH, materials[i].shininess_strength);
		mat->Get(AI_MATKEY_REFRACTI, materials[i].refraction);

		int textureCount = 0;
		for (int type = aiTextureType_NONE; type < AI_TEXTURE_TYPE_MAX; type++) {
			for (int j = 0; j < mat->GetTextureCount((aiTextureType)type); j++) {
				aiString path;
				aiTextureMapping mapping;
				unsigned int uv_index;
				ai_real blend;
				aiTextureOp op;
				aiTextureMapMode mapmode[3];
				if (mat->GetTexture((aiTextureType)type, j, &path, &mapping, &uv_index, &blend, &op, mapmode) == AI_SUCCESS) {
					Texture& t = materials[i].textures[textureCount];
					t.type = (aiTextureType)type;
					t.tex_index = atoi(path.C_Str() + 1);
					t.tex_blend = blend;
					t.tex_op = op;
					t.mapping = mapping;
					mat->Get(AI_MATKEY_UVWSRC(type, j), t.uvwsrc);
					t.mappingmode_u = mapmode[0];
					t.mappingmode_v = mapmode[1];
					std::cout << "Loaded texture " << path.C_Str() << std::endl;
				}
				textureCount++;
			}
		}
		materials[i].textureCount = textureCount;

		// for (int p = 0; p < mat->mNumProperties; p++) {
		// 	aiMaterialProperty* prop = mat->mProperties[p];
		// 	if (prop->mIndex == 0 && prop->mSemantic == 0) continue;
		// 	std::cout << "Texture index " << prop->mIndex << std::endl;
		// 	std::cout << "\t" << prop->mKey.C_Str() << ": 0x" << std::hex ;
		// 	for (int x = 0; x < prop->mDataLength; x++)
		// 		std::cout << +uint8_t(prop->mData[x]);
		// 	std::cout << std::dec << std::endl;
		//  }
	}

	for (int i = 0; i < scene->mNumMeshes; i++) {
		Mesh m;
		aiMesh* mesh = scene->mMeshes[i];
		m.vertices.reserve(mesh->mNumVertices);
		m.indices.reserve(mesh->mNumFaces * 3);
		m.material = materials[mesh->mMaterialIndex];

		if (mesh->mNumAnimMeshes > 0) {
			AnimMesh anim;
			if (m.anims.capacity() == 0)
				m.anims.reserve(mesh->mNumAnimMeshes);

			for (int j = 0; j < mesh->mNumAnimMeshes; j++) {
				for (int k = 0; k < mesh->mNumVertices; k++) {
					aiVector3D v = mesh->mAnimMeshes[j]->mVertices[k];
					anim.verts.push_back(aiColor4D(v.x, v.y, v.z, 1));
					//anim.normals[k] = mesh->mAnimMeshes[j]->mNormals[k];
				}
				m.anims.push_back(anim);
			}
		}

		std::cout << "Mesh " << i << " has " << mesh->mNumVertices << " vertices" << std::endl;
		for (int j = 0; j < mesh->mNumVertices; j++) {
			aiVector3D vertexPos = mesh->mVertices[j];
			Vertex v;
			v.index = j;
			v.pos = glm::vec3(vertexPos.x, vertexPos.z, vertexPos.y);
			if (mesh->HasNormals()) {
				aiVector3D normal = mesh->mNormals[j];
				v.normal = glm::vec3(normal.x, normal.y, normal.z);
			} else {
				v.normal = glm::vec3(1);
			}

			if (mesh->HasTextureCoords(0)) {
				aiVector3D texCoord = mesh->mTextureCoords[0][j];
				v.texCoord = glm::vec2(texCoord.x, texCoord.y);
			} else {
				v.texCoord = glm::vec2(1);
			}

			if (mesh->HasVertexColors(0)) {
				aiColor4D colour = mesh->mColors[0][j];
				v.color = glm::vec4(colour.r, colour.g, colour.b, colour.a);
			} else {
				v.color = glm::vec4(1);
			}
			m.vertices.push_back(v);
		}

		for (int j = 0; j < mesh->mNumFaces; j++) {
			aiFace face = mesh->mFaces[j];
			for (int k = 0; k < face.mNumIndices; k++) {
				m.indices.push_back(face.mIndices[k]);
			}
		}
		meshes.push_back(m);

	}

	textureData.reserve(scene->mNumTextures);
	for (int i = 0; i < scene->mNumTextures; i++) {
		aiTexture* texture = scene->mTextures[i];
		std::vector<uint8_t> data;
		for (int j = 0; j * 4 < texture->mWidth; j++) {
			aiTexel texel = texture->pcData[j];
			data.push_back(texel.b);
			data.push_back(texel.g);
			data.push_back(texel.r);
			data.push_back(texel.a);
		}
		textureData.push_back(data);
	}

	std::cout << "Loaded scene with " << scene->mNumMeshes << " meshes" << std::endl;
	*/
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
	vulkan.cleanup(vrmImporter.getTextureCount());
	glfwDestroyWindow(window);
	glfwTerminate();
}
