#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D texSampler;

layout(binding = 2) uniform GlobalUniformBufferObject {
	vec3 lightDir;
	vec4 lightColor;
	vec3 eyePos;
} gubo;

void main() {
	// ambient
	float ambientReflectance = .5;
	vec3 ambient = texture(texSampler, fragTexCoord).rgb;
	vec3 ambientColor = ambient * ambientReflectance + 0 * gubo.lightColor.rgb;

	// diffuse
	float diffuseReflectance = 1.0;
	vec3 norm = normalize(fragNorm);
	// vec3 lightDir = normalize(-gubo.lightDir);
	float diff = clamp(dot(norm, normalize(gubo.eyePos - fragPos)), 0, 1);
	vec3 diffuseColor = diff * ambient * diffuseReflectance;

	// bling specular
	float magnitude = 20;
	float specularReflectance = 1.0;
	vec3 viewDir = normalize(gubo.eyePos - fragPos);
	vec3 lightDir = normalize(-gubo.lightDir);
	vec3 halfDir = normalize(viewDir + lightDir);
	float spec = pow(clamp(dot(halfDir, normalize(fragNorm)), 0, 1), magnitude);
	vec3 specular = specularReflectance * gubo.lightColor.rgb * spec;

	outColor = vec4( ambientColor + diffuseColor + specular, 1.0);
}

