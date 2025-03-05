#ifndef VRMIMPORTER_HPP
#define VRMIMPORTER_HPP

#include "../json.hpp"
#include <iostream>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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
		alignas(16) glm::mat4 localTransform;
		alignas(16) glm::mat4 globalTransform;
		alignas(16) glm::mat4 jointMatrix;
		int mesh;
		int skin;
		int parent;
		int firstChild;
		int nextSibling;
	};
}

class VRMImporter {
public:
	nlohmann::json m_Header;
	std::vector<char> m_Buffer;
	std::vector<VRM::FCNSNode> m_Nodes;
	std::unordered_map<size_t, std::vector<glm::mat4>> m_Joints;
	void loadModel(const std::string& path);

	void loadNodes();

	template<class T>
	Array<T> getMeshAttribute(int meshIndex, int primitiveIndex, const std::string& attribute) {
		auto& mesh = m_Header["meshes"][meshIndex];
		auto& primitive = mesh["primitives"][primitiveIndex];
		if (!primitive["attributes"].contains(attribute)) {
			return {nullptr, 0};
		}
		int accessorIndex = primitive["attributes"][attribute];
		auto& accessor = m_Header["accessors"][accessorIndex];
		size_t bufferViewIndex = accessor["bufferView"];
		auto& bufferView = m_Header["bufferViews"][bufferViewIndex];
		size_t byteOffset = bufferView["byteOffset"];
		size_t count = accessor["count"];
		return {reinterpret_cast<T*>(&m_Buffer.at(byteOffset)), count};
	}

	template<class T>
	Array<T> getMeshProperty(int meshIndex, int primitiveIndex, const std::string& property) {
		auto& mesh = m_Header["meshes"][meshIndex];
		auto& primitive = mesh["primitives"][primitiveIndex];
		int accessorIndex = primitive[property];
		auto& accessor = m_Header["accessors"][accessorIndex];
		size_t bufferViewIndex = accessor["bufferView"];
		auto& bufferView = m_Header["bufferViews"][bufferViewIndex];
		size_t byteOffset = bufferView["byteOffset"];
		size_t count = accessor["count"];
		return {reinterpret_cast<T*>(&m_Buffer.at(byteOffset)), count};
	}

	template<class T>
	Array<T> getMeshMorph(int meshIndex, int primitiveIndex, size_t morphIndex, const std::string& attribute) {
		auto& mesh = m_Header["meshes"][meshIndex];
		auto& primitive = mesh["primitives"][primitiveIndex];
		int accessorIndex = primitive["targets"][morphIndex][attribute];
		auto& accessor = m_Header["accessors"][accessorIndex];
		size_t bufferViewIndex = accessor["bufferView"];
		auto& bufferView = m_Header["bufferViews"][bufferViewIndex];
		size_t byteOffset = bufferView["byteOffset"];
		size_t count = accessor["count"];
		return {reinterpret_cast<T*>(&m_Buffer.at(byteOffset)), count};
	}

	size_t getMeshMaterialIndex(size_t meshIndex, size_t primitiveIndex) {
		return m_Header["meshes"][meshIndex]["primitives"][primitiveIndex]["material"];
	}

	char* getBufferView(size_t bufferView) {
		size_t offset = m_Header["bufferViews"][bufferView]["byteOffset"];
		return m_Buffer.data() + offset;
	}

	VRM::TextureData getTextureData(size_t textureIndex) {
		size_t bufferViewIndex = m_Header["images"][textureIndex]["bufferView"];
		return {
			getBufferView(bufferViewIndex),
			m_Header["bufferViews"][bufferViewIndex]["byteLength"]
		};
	}

	VRM::Material getMaterial(size_t materialIndex) {
		VRM::Material material;
		auto& mat = m_Header["materials"][materialIndex];
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

	int findNodeFromMeshIndex(size_t meshIndex) {
		for (size_t i = 0; i < m_Nodes.size(); i++) {
			if (m_Nodes[i].mesh == meshIndex)
				return i;
		}
		return -1;
	}

	size_t getMeshCount() {
		return m_Header["meshes"].size();
	}

	size_t getPrimitiveCount(size_t meshIndex) {
		return m_Header["meshes"][meshIndex]["primitives"].size();
	}

	size_t getTextureCount() {
		return m_Header["textures"].size();
	}

	size_t getMeshBlendShapeCount(size_t meshIndex, size_t primitiveIndex) {
		auto& mesh = m_Header["meshes"][meshIndex];
		auto& primitive = mesh["primitives"][primitiveIndex];
		return primitive["targets"].size();
	}

	size_t getBoneCount() {
		auto& accessor = m_Header["accessors"][0];
		size_t count = accessor["count"];
		return count;
	}

	void calculateJoints();
	void recalculateMatrices();
};

#endif
