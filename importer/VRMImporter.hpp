#ifndef VRMIMPORTER_HPP
#define VRMIMPORTER_HPP

#include "../json.hpp"
#include <iostream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

template<class T>
struct Array {
	T* data;
	size_t count;
};

namespace VRM {
	struct Material {
		bool doubleSided;
		int alphaMode;
		int normalTextureIndex;
		int emissiveTextureIndex;
		int baseColourTextureIndex;
	};

	struct TextureData {
		char* begin;
		size_t byteLength;
	};

	struct FCNSNode {
		glm::vec4 translation;
		glm::vec4 scale;
		glm::vec4 rotation;
		int mesh;
		int skin;
		int firstChild;
		int nextSibling;
	};
}

class VRMImporter {
public:
	nlohmann::json header;
	std::vector<char> buffer;
	std::vector<VRM::FCNSNode> nodes;
	void loadModel(const std::string& path);

	void loadNodes();

	template<class T>
	Array<T> getMeshAttribute(int meshIndex, int primitiveIndex, const std::string& attribute) {
		auto& mesh = header["meshes"][meshIndex];
		auto& primitive = mesh["primitives"][primitiveIndex];
		if (!primitive["attributes"].contains(attribute)) {
			return {nullptr, 0};
		}
		int accessorIndex = primitive["attributes"][attribute];
		auto& accessor = header["accessors"][accessorIndex];
		size_t bufferViewIndex = accessor["bufferView"];
		auto& bufferView = header["bufferViews"][bufferViewIndex];
		size_t byteOffset = bufferView["byteOffset"];
		size_t count = accessor["count"];
		return {reinterpret_cast<T*>(&buffer.at(byteOffset)), count};
	}

	template<class T>
	Array<T> getMeshProperty(int meshIndex, int primitiveIndex, const std::string& property) {
		auto& mesh = header["meshes"][meshIndex];
		auto& primitive = mesh["primitives"][primitiveIndex];
		int accessorIndex = primitive[property];
		auto& accessor = header["accessors"][accessorIndex];
		size_t bufferViewIndex = accessor["bufferView"];
		auto& bufferView = header["bufferViews"][bufferViewIndex];
		size_t byteOffset = bufferView["byteOffset"];
		size_t count = accessor["count"];
		return {reinterpret_cast<T*>(&buffer.at(byteOffset)), count};
	}

	template<class T>
	Array<T> getMeshMorph(int meshIndex, int primitiveIndex, size_t morphIndex, const std::string& attribute) {
		auto& mesh = header["meshes"][meshIndex];
		auto& primitive = mesh["primitives"][primitiveIndex];
		int accessorIndex = primitive["targets"][morphIndex][attribute];
		auto& accessor = header["accessors"][accessorIndex];
		size_t bufferViewIndex = accessor["bufferView"];
		auto& bufferView = header["bufferViews"][bufferViewIndex];
		size_t byteOffset = bufferView["byteOffset"];
		size_t count = accessor["count"];
		return {reinterpret_cast<T*>(&buffer.at(byteOffset)), count};
	}

	size_t getMeshMaterialIndex(size_t meshIndex, size_t primitiveIndex) {
		return header["meshes"][meshIndex]["primitives"][primitiveIndex]["material"];
	}

	char* getBufferView(size_t bufferView) {
		size_t offset = header["bufferViews"][bufferView]["byteOffset"];
		return buffer.data() + offset;
	}

	VRM::TextureData getTextureData(size_t textureIndex) {
		size_t bufferViewIndex = header["images"][textureIndex]["bufferView"];
		return {
			getBufferView(bufferViewIndex),
			header["bufferViews"][bufferViewIndex]["byteLength"]
		};
	}

	VRM::Material getMaterial(size_t materialIndex) {
		VRM::Material material;
		auto& mat = header["materials"][materialIndex];
		if (mat.contains("alphaMode"))
			material.alphaMode = mat["alphaMode"] == "MASK" ? 0 : 1;
		else
			material.alphaMode = -1;

		if (mat.contains("doubleSided"))
			material.doubleSided = mat["doubleSided"];
		else
			material.doubleSided = false;

		if (mat.contains("normalTexture"))
			material.normalTextureIndex = mat["normalTexture"]["index"];
		else
			material.normalTextureIndex = -1;

		if (mat.contains("emissiveTexture"))
			material.emissiveTextureIndex = mat["emissiveTexture"]["index"];
		else
			material.emissiveTextureIndex = -1;

		material.baseColourTextureIndex = -1;
		if (mat.contains("pbrMetallicRoughness")){
			auto& roughness = mat["pbrMetallicRoughness"];
			if (roughness.contains("baseColorTexture"))
				material.baseColourTextureIndex = roughness["baseColorTexture"]["index"];
		}

		return material;
	}

	size_t getMeshCount() {
		return header["meshes"].size();
	}

	size_t getPrimitiveCount(size_t meshIndex) {
		return header["meshes"][meshIndex]["primitives"].size();
	}

	size_t getTextureCount() {
		return header["textures"].size();
	}

	size_t getMeshBlendShapeCount(size_t meshIndex, size_t primitiveIndex) {
		auto& mesh = header["meshes"][meshIndex];
		auto& primitive = mesh["primitives"][primitiveIndex];
		return primitive["targets"].size();
	}
};

#endif
