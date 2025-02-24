#include "VRMImporter.hpp"
#include <fstream>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

void VRMImporter::loadModel(const std::string& path) {
	std::ifstream file(path, std::ios::in | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	uint32_t magic;
	file.read(reinterpret_cast<char *>(&magic), sizeof(uint32_t));
	assert(magic == 0x46546C67);

	uint32_t version;
	file.read(reinterpret_cast<char *>(&version), sizeof(uint32_t));

	uint32_t length;
	file.read(reinterpret_cast<char *>(&length), sizeof(uint32_t));

	uint32_t chunkLength;
	file.read(reinterpret_cast<char *>(&chunkLength), sizeof(uint32_t));

	uint32_t chunkType;
	file.read(reinterpret_cast<char *>(&chunkType), sizeof(uint32_t));

	buffer.resize(chunkLength);
	file.read(reinterpret_cast<char*>(buffer.data()), chunkLength);
	std::cout << "Bytes read: " << file.gcount() << std::endl;
	assert(file.gcount() >= chunkLength);
	//buffer.insert(buffer.begin(), std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	assert(file.good());
	assert(buffer.size() == chunkLength);

	header = nlohmann::json::parse(buffer.data());

	file.read(reinterpret_cast<char *>(&chunkLength), sizeof(uint32_t));
	file.read(reinterpret_cast<char *>(&chunkType), sizeof(uint32_t));
	buffer.clear();
	buffer.resize(chunkLength);
	file.read(reinterpret_cast<char*>(buffer.data()), chunkLength);

	std::cout << "VRM Header: " << header << std::endl;

	file.close();

	loadNodes();
}

void VRMImporter::loadNodes() {
	size_t size = header["nodes"].size();
	nodes.reserve(size);

	for (size_t i = 0; i < size; i++) {
		VRM::FCNSNode node;
		auto& n = header["nodes"][i];
		if (n.contains("translation")) {
			auto& trans = n["translation"];
			node.translation = glm::vec3(trans[0], trans[1], trans[2]);
		} else {
			node.translation = glm::vec3(0);
		}

		if (n.contains("rotation")) {
			auto& trans = n["rotation"];
			node.rotation = glm::vec4(trans[0], trans[1], trans[2], trans[3]);
		} else {
			node.rotation = glm::vec4(0, 0, 0, 1);
		}

		if (n.contains("scale")) {
			auto& trans = n["scale"];
			node.scale = glm::vec3(trans[0], trans[1], trans[2]);
		} else {
			node.scale = glm::vec3(1);
		}

		if (n.contains("mesh")) {
			node.mesh = n["mesh"];
		} else {
			node.mesh = -1;
		}

		if (n.contains("skin")) {
			node.skin = n["skin"];
		} else {
			node.skin = -1;
		}

		if (n.contains("children")) {
			auto& children = n["children"];
			node.firstChild = children[0];
		}

		node.parent = -1;
		node.nextSibling = -1;
		nodes.push_back(node);
	}

	// Next sibling loop
	for (size_t i = 0; i < size; i++) {
		auto& n = header["nodes"][i];

		if (!n.contains("children")) continue;
		size_t childrenCount = n["children"].size();
		if (childrenCount <= 1) {
			size_t childIndex = n["children"][0];
			VRM::FCNSNode& child = nodes[childIndex];
			child.parent = i;
			continue;
		}

		for (size_t j = 0; j < childrenCount - 1; j++) {
			size_t childIndex = n["children"][j];
			size_t siblingIndex = n["children"][j + 1];
			VRM::FCNSNode& child = nodes[childIndex];
			VRM::FCNSNode& sibling = nodes[siblingIndex];
			child.parent = i;
			sibling.parent = i;
			child.nextSibling = siblingIndex;
		}
	}
}

Array<glm::mat4> VRMImporter::getBoneTransforms() {
	auto& accessor = header["accessors"][0];
	size_t bufferViewIndex = accessor["bufferView"];
	size_t count = accessor["count"];
	_boneTransforms.reserve(count);
	auto& bufferView = header["bufferViews"][bufferViewIndex];
	size_t byteOffset = bufferView["byteOffset"];
	glm::mat4* inverseBinds = reinterpret_cast<glm::mat4*>(&buffer.at(byteOffset));

	_boneTransforms[0] = glm::mat4(1.0);
	for (size_t i = 0; i < count; i++) {
		VRM::FCNSNode& node = nodes[i];
		glm::mat4 trans = glm::mat4(1.0);
		//glm::mat4 scaled = glm::scale(trans, node.scale);
		// trans *= node.rotation;
		//glm::mat4 translated = glm::translate(trans, node.translation);

		if (node.parent != -1)
			_boneTransforms[i] = trans * _boneTransforms[node.parent];

		_boneTransforms[i] = _boneTransforms[i] * inverseBinds[i];
	}

	return {_boneTransforms.data(), count};
}


