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
		node.localTransform = glm::mat4(1.0);
		auto& n = header["nodes"][i];

		glm::mat4 translation;
		glm::mat4 rotation;
		glm::mat4 scaling;

		if (n.contains("translation")) {
			auto& trans = n["translation"];
			translation = glm::translate(glm::mat4(1.0f), glm::vec3(trans[0], trans[1], trans[2]));
		} else {
			translation = glm::translate(glm::mat4(1.0), glm::vec3(0));
		}

		if (n.contains("rotation")) {
			auto& trans = n["rotation"];
			rotation = glm::mat4(glm::quat(trans[0], trans[1], trans[2], trans[3]));
		} else {
			rotation = glm::mat4(glm::quat(0, 0, 0, 1));
		}

		if (n.contains("scale")) {
			auto& trans = n["scale"];
			scaling = glm::scale(glm::mat4(1.0), glm::vec3(trans[0], trans[1], trans[2]));
		} else {
			scaling = glm::scale(glm::mat4(1.0), glm::vec3(1));
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

		node.localTransform = translation * rotation * scaling;
		node.globalTransform = node.localTransform;
		node.parent = -1;
		node.nextSibling = -1;
		nodes.push_back(node);
	}

	// Next sibling loop
	for (size_t i = 0; i < size; i++) {
		auto& n = header["nodes"][i];
		VRM::FCNSNode& parent = nodes[i];

		if (!n.contains("children")) continue;
		size_t childrenCount = n["children"].size();

		// Update global transform
		for (size_t j = 0; j < childrenCount; j++) {
			size_t childIndex = n["children"][j];
			VRM::FCNSNode& child = nodes[childIndex];
			child.globalTransform = child.localTransform * parent.localTransform;
		}

		if (childrenCount == 1) {
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

void VRMImporter::calculateJoints() {
	auto& accessor = header["accessors"][0];
	size_t bufferViewIndex = accessor["bufferView"];

	auto& bufferView = header["bufferViews"][bufferViewIndex];
	size_t byteOffset = bufferView["byteOffset"];
	glm::mat4* inverseBinds = reinterpret_cast<glm::mat4*>(&buffer.at(byteOffset));

	for (size_t i = 0; i < nodes.size(); i++) {
		VRM::FCNSNode& node = nodes[i];
		glm::mat4 inverseTransform = glm::inverse(node.globalTransform);

		if (node.skin != -1) {
			auto& joints = header["skins"][node.skin]["joints"];
			size_t jointsCount = joints.size();

			for (size_t j = 0; j < jointsCount; j++) {
				size_t jointIndex = joints[j];
				VRM::FCNSNode& joint = nodes[jointIndex];
				glm::mat4 jointMat = joint.globalTransform * inverseBinds[j];
				jointMat = inverseTransform * jointMat;
				node.jointMatrix = jointMat;
			}
		}
	}
}

void VRMImporter::recalculateMatrices() {
	for (size_t i = 0; i < nodes.size(); i++) {
		VRM::FCNSNode& parent = nodes[i];
		auto& n = header["nodes"][i];

		if (!n.contains("children")) continue;
		size_t childrenCount = n["children"].size();

		// Update global transform
		for (size_t j = 0; j < childrenCount; j++) {
			size_t childIndex = n["children"][j];
			VRM::FCNSNode& child = nodes[childIndex];
			child.globalTransform = child.localTransform * parent.localTransform;
		}
	}

	calculateJoints();
}
