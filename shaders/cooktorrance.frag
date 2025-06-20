#version 450
#extension GL_ARB_separate_shader_objects : enable
#define PI 3.14159265358979323846

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D texSampler;

layout(binding = 2) uniform GlobalUniformBufferObject {
	vec3 lightDir;
	vec4 lightColor;
	vec3 eyePos;
	vec3 spotLightDir;
	vec3 spotLightPos;
} gubo;

// layout(binding = 3) uniform spotLightObject {
// } slo;

const float roughness = 0.5f;
const float metallic = 0.2f;
const float F0 = 0.4f;
const float c_out = 0.85f;
const float c_in = 0.95f;
const float beta = 3.0f;


vec3 CookTorrance(vec3 V, vec3 N, vec3 L, vec3 Md){
    vec3 Ms = vec3(1.0f);
    float K = 1.0f - metallic;

    vec3 H = normalize(L + V);

    float NdotL = max(0.001, dot(N, L));
    float NdotV = max(0.001, dot(N, V));

    vec3 lambertDiffuse = Md * clamp(NdotL, 0, 1);

    float rho2 = roughness * roughness;
    float D = rho2 / (PI * pow(pow(clamp(max(0.001, dot(N, H)), 0, 1), 2) * (rho2 - 1) + 1, 2));

    float F = F0 + (1 - F0) * pow(1 - clamp(max(0.001, dot(V, H)), 0, 1), 5);

    float gV = 2 / (1 + sqrt(1 + rho2 * ((1 - pow(NdotV, 2)) / pow(NdotV, 2))));
    float gL = 2 / (1 + sqrt(1 + rho2 * ((1 - pow(NdotL, 2)) / pow(NdotL, 2))));
    float G = gV * gL;

    vec3 cookTorranceSpecular = Ms * (D * F * G) / (4 * clamp(NdotV, 0.001, 1));

    // return K*Md*clamp(NdotL, 0, 1);
    return K*lambertDiffuse + metallic*cookTorranceSpecular;
}


void main() {
    vec3 normal = normalize(vec3(abs(fragNorm.x), abs(fragNorm.y), fragNorm.z));
    vec3 viewDir = normalize(gubo.eyePos - fragPos);
    vec3 lightDir = gubo.lightDir;
    vec3 albedo = texture(texSampler, fragTexCoord).rgb;

    vec3 posRight = gubo.spotLightPos + vec3(0.5, 0, 0);
    
    vec3 diffLeft = vec3(gubo.spotLightPos.x, gubo.spotLightPos.y, gubo.spotLightPos.z) - fragPos;
    vec3 diffRight = vec3(gubo.spotLightPos.x - 1.0f, gubo.spotLightPos.y, gubo.spotLightPos.z) - fragPos;

    vec3 cook_torrance = CookTorrance(viewDir, normal, lightDir, albedo);

    float spotlightLeft = clamp(pow(50.0f / length(diffLeft), beta) * clamp((dot(normalize(diffLeft), normalize(-gubo.spotLightDir)) - c_out) / (c_in - c_out), 0.0f, 1.0f), 0.0f, 1.0f);
    float spotlightRight = clamp(pow(50.0f / length(diffRight), beta) * clamp((dot(normalize(diffRight), normalize(-gubo.spotLightDir)) - c_out) / (c_in - c_out), 0.0f, 1.0f), 0.0f, 1.0f);
    // float spotlight_right = clamp(pow(50.0f / length(diffRight), beta) * clamp((dot(normalize(diffRight), normalize(-gubo.spotLightDir)) - c_out) / (c_in - c_out), 0.0f, 1.0f), 0.0f, 1.0f);
    // vec3 left = spotlight_left * vec3(0.1f);
    vec3 left = spotlightLeft * vec3(0.05f);
    vec3 right = spotlightRight * vec3(0.05f);

    // outColor = vec4(albedo * gubo.pointLightColor.rgb * NdotL + gubo.pointLightColor.rgb * Rs, 1);
    outColor = vec4(cook_torrance + left + right , 1);
    // outColor = vec4(normalize(gubo.spotLightPosLeft), 1);
    // outColor = vec4(slo.spotLightDir , 1);
    // outColor = vec4(normal, 1);
}
