#include "VRMImporter.hpp"
#include <fstream>
#include <iostream>

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
			node.translation = glm::vec4(trans[0], trans[1], trans[2], 1.0);
		} else {
			node.translation = glm::vec4(0);
		}

		if (n.contains("rotation")) {
			auto& trans = n["rotation"];
			node.rotation = glm::vec4(trans[0], trans[1], trans[2], trans[3]);
		} else {
			node.rotation = glm::vec4(0, 0, 0, 1);
		}

		if (n.contains("scale")) {
			auto& trans = n["scale"];
			node.scale = glm::vec4(trans[0], trans[1], trans[2], 1.0);
		} else {
			node.scale = glm::vec4(1);
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

		node.nextSibling = -1;
		nodes.push_back(node);
	}

	// Next sibling loop
	for (size_t i = 0; i < size; i++) {
		auto& n = header["nodes"][i];

		if (!n.contains("children")) continue;
		size_t childrenCount = n["children"].size();
		if (childrenCount <= 1) continue;

		for (size_t j = 0; j < childrenCount - 1; j++) {
			size_t childIndex = n["children"][j];
			size_t siblingIndex = n["children"][j + 1];
			VRM::FCNSNode& child = nodes[childIndex];
			child.nextSibling = siblingIndex;
		}
	}

}


