#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	mat4 mvpMat;
	mat4 mMat;
	mat4 nMat;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNorm;
layout(location = 2) out vec2 fragTexCoord;

void main() {
  	// transform pos to vec3 in world space
	fragPos = (ubo.mMat * vec4(inPosition, 1.0)).xyz;
	
	// transform normals to world space
	fragNorm = mat3(ubo.nMat) * inNormal; // inverse matrix * normal

	// equal to uv
	fragTexCoord = inTexCoord;

	// NDC - normalized device coords 
	gl_Position = ubo.mvpMat * vec4(inPosition, 1.0); // model view projection matrix * position
}