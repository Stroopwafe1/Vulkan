#version 450

struct Material {
	bool doubleSided;
	int alphaMode;
	int normalTextureIndex;
	int emissiveTextureIndex;
	int baseColourTextureIndex;
};

struct FCNSNode {
	mat4 localTransform;
	mat4 globalTransform;
	mat4 jointMatrix;
	int mesh;
	int skin;
	int parent;
	int firstChild;
	int nextSibling;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	float time;
} ubo;

layout(std140, set = 0, binding = 2) readonly buffer AnimBuffer {
	vec4 anims[];
} animBuffer;

layout(std140, set = 1, binding = 1) readonly buffer NodeBuffer {
	FCNSNode nodes[];
} nodeBuffer;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in uvec4 inJoints;
layout(location = 5) in vec4 inWeights;
layout(location = 6) in int inIndex;

layout(location = 0) out vec3 fragColour;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

layout (push_constant) uniform PushConstant {
	int materialIndex;
	float value;
	int numVertices;
	int nodeIndex;
} constants;

void main() {
	//float fun = sin(dot(inTexCoord.x, inTexCoord.y) * ubo.time) / 2.0;
	//gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition.x, inPosition.y, fun, 1.0);
	mat4 mv =   ubo.view * ubo.model;
	vec3 pos = inPosition;

// 	vec4 locPos;
// 	if (constants.nodeIndex > 0) {
// 		mat4 skinMat =
// 			inWeights.x * nodeBuffer.nodes[inJoints.x].jointMatrix +
// 			inWeights.y * nodeBuffer.nodes[inJoints.y].jointMatrix +
// 			inWeights.z * nodeBuffer.nodes[inJoints.z].jointMatrix +
// 			inWeights.w * nodeBuffer.nodes[inJoints.w].jointMatrix;
//
// 		locPos = ubo.model * nodeBuffer.nodes[constants.nodeIndex].localTransform * skinMat * vec4(pos, 1.0);
// 		locPos.z = 0;
// 	}
// 	vec3 worldPos = locPos.xyz;// / locPos.w;

	vec3 posAfterBone = vec3(0);
	for (int i = 0; i < 4; i++) {
		mat4 boneTransform = nodeBuffer.nodes[inJoints[i]].jointMatrix * nodeBuffer.nodes[constants.nodeIndex].globalTransform;
		posAfterBone += (inWeights[i] * (boneTransform * vec4(pos, 1.0))).xyz;
	}
	if (posAfterBone == vec3(0))
		posAfterBone = pos;

	posAfterBone += constants.value * (animBuffer.anims[int(constants.value) * (17 * constants.numVertices + inIndex)].xzy + animBuffer.anims[int(constants.value) * (32 * constants.numVertices + inIndex)].xzy);
 	vec4 P = mv * vec4(posAfterBone, 1.0);

	//gl_Position = ubo.proj * ubo.view * vec4(worldPos, 1.0);
	gl_Position = ubo.proj * P;
	fragColour = inColor;
	fragTexCoord = vec2(1-inTexCoord.x, inTexCoord.y);
	//fragNormal = vec3(inNormal.x * -1, inNormal.zy);
	fragNormal = pos;
}
