#version 450

struct Material {
	bool doubleSided;
	int alphaMode;
	int normalTextureIndex;
	int emissiveTextureIndex;
	int baseColourTextureIndex;
};

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	float time;
} ubo;

layout(std140, binding = 3) readonly buffer AnimBuffer {
	vec4 anims[];
} animBuffer;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in int inIndex;

layout(location = 0) out vec3 fragColour;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

layout (push_constant) uniform PushConstant {
	int materialIndex;
	float value;
	int numVertices;
} constants;

void main() {
	//float fun = sin(dot(inTexCoord.x, inTexCoord.y) * ubo.time) / 2.0;
	//gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition.x, inPosition.y, fun, 1.0);
	mat4 mv =   ubo.view * ubo.model;
	vec3 pos = inPosition;
	if (constants.value > .5)
		pos += animBuffer.anims[17 * constants.numVertices + inIndex].xzy + animBuffer.anims[32 * constants.numVertices + inIndex].xzy;
	vec4 P = mv * vec4(pos, 1.0);

	gl_Position = ubo.proj * P;
	fragColour = inColor;
	fragTexCoord = vec2(1-inTexCoord.x, inTexCoord.y);
	//fragNormal = vec3(inNormal.x * -1, inNormal.zy);
	fragNormal = pos.xyz;
}
