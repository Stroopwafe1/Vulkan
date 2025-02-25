#version 450
#extension GL_EXT_debug_printf : enable

struct Material {
	bool doubleSided;
	int alphaMode;
	int normalTextureIndex;
	int emissiveTextureIndex;
	int baseColourTextureIndex;
};

layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColour;

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	float time;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D texSamplers[22];

layout(std140, set = 0, binding = 1) readonly buffer MaterialBuffer {
	Material material;
} materialBuffer;

layout (push_constant) uniform PushConstant {
	int materialIndex;
	float value;
	int numVertices;
} constants;

void main() {
	//vec3 N = normalize(fragNormal);
	vec3 L = normalize(vec3(3));
	//outColour = vec4(fragTexCoord, 0, 1);
	//outColour = vec4(materialBuffer.material.baseColourTextureIndex / 22.0);
	int baseColourIndex = materialBuffer.material.baseColourTextureIndex;
	if (baseColourIndex > 0 && baseColourIndex <= 22) {
		outColour = texture(texSamplers[baseColourIndex], fragTexCoord);
	} else {
		outColour = vec4(fragTexCoord, 0, 1);
	}
	//outColour = max(dot(fragNormal, L), 0.1) * texture(texSamplers[materialBuffer.materials[matIndex.mIndex].textures[0].tex_index], fragTexCoord);
	//outColour = vec4(fragNormal, 1.0);
}
