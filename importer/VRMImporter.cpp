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
}

