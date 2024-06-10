// This has been adapted from the Vulkan tutorial

#include "Starter.hpp"
#include "TextMaker.hpp"

std::vector<SingleText> outText = {
	{4, {"Press SPACE to save the screenshot", "01234567890123456789", "01234567890123456789", "01234567890123456789"}, 0, 0},
	{2, {"Screenshot Saved!","Press ESC to quit","",""}, 0, 0}
};

// The uniform buffer object used in this example
#define NSHIP 16 //?
struct BlinnUniformBufferObject {
	alignas(16) glm::mat4 mvpMat[NSHIP];
	alignas(16) glm::mat4 mMat[NSHIP];
	alignas(16) glm::mat4 nMat[NSHIP];
};

struct BlinnMatParUniformBufferObject {
	alignas(4)  float Power;
};

// The vertices data structures
// Example
struct Vertex {
	glm::vec3 pos;
	glm::vec2 UV;
	glm::vec4 color; 
};

struct BlinnVertex {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 UV;
};

#include "figure.hpp"

struct GlobalUniformBufferObject {
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec3 eyePos;
};

struct Camera {
    glm::vec3 pos;
    float yaw, yawNew;
    float pitch, pitchNew;
    float roll, rollNew;
};

class InfiniteRun : public BaseProject {
	
	public:

	protected:
	// Descriptor Layouts ["classes" of what will be passed to the shaders]
	DescriptorSetLayout DSLGlobal; // For Global values
	// DescriptorSetLayout DSLBlinn;	// For Blinn Objects

	DescriptorSetLayout DSL;
	// Vertex formats
	// VertexDescriptor VD;
	VertexDescriptor VMesh;

	// Pipelines [Shader couples]
	Pipeline PRoad;
	
	// Models, textures and Descriptors (values assigned to the uniforms)
	// Please note that Model objects depends on the corresponding vertex structure
	// Models
	Model<Vertex> MRoad;
	// Descriptor sets
	DescriptorSet DSGlobal;

	//DescriptorSet DSGubo;

	//DescriptorSet Road; 
	// ... DescriptorSet Road
	// ... DescriptorSet Terrain
	// ... DescriptorSet StreetLights
	// ... 
	// Textures
	// Texture Road;
	// ... Texture Car;
	// ... Texture StreetLight;

	//Uniform blocks

	// updates global uniforms
	// Global
	// GlobalUniformBufferObject gubo;

	TextMaker time;
	// TextMaker score;

	void setWindowParameters() {
		windowWidth = 1280;
        windowHeight = 720;
        windowTitle = "Infinite Run";
        windowResizable = GLFW_FALSE;
        initialBackgroundColor = {0.0f, 1.0f, 1.0f, 1.0f};

		// TODO check mem allocation of pool sizes
        uniformBlocksInPool = 76;
        texturesInPool = 147;
        setsInPool = 77;

        Ar = (float) windowWidth / (float) windowHeight;
	}

	void localInit() {
        // Init Descriptor Layouts [what will be passed to the shaders]
		DSLGlobal.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject), 1}
				});
		

		DSL.init(this, {
					{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
				});

		// Vertex descriptors
		VD.init(this, {
				  {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
				}, {
				  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
				         sizeof(glm::vec3), POSITION},
				  {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
				         sizeof(glm::vec2), UV},
				  {0, 2, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color),
				         sizeof(glm::vec4), COLOR}
				});
		VDBlinn.init(this, {
				{0, sizeof(BlinnVertex), VK_VERTEX_INPUT_RATE_VERTEX}
				}, {
				{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(BlinnVertex, pos),
						sizeof(glm::vec3), POSITION},
				{0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(BlinnVertex, norm),
						sizeof(glm::vec3), NORMAL},
				{0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(BlinnVertex, UV),
						sizeof(glm::vec2), UV}
				});
		VMesh.init(this, {{0, sizeof(VertexMesh), VK_VERTEX_INPUT_RATE_VERTEX}},
				{{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexMesh, pos),  sizeof(glm::vec3), POSITION},
				{0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexMesh, norm), sizeof(glm::vec3), NORMAL},
				{0, 2, VK_FORMAT_R32G32_SFLOAT,    offsetof(VertexMesh, UV),   sizeof(glm::vec2), UV}});

		// Init Pipelines [Shader couples]
		P.init(this, &VD, "shaders/ShaderVert.spv", "shaders/ShaderFrag.spv", {&DSL});
		P.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
 								    VK_CULL_MODE_NONE, false);
		//PBlinn.init(this, &VDBlinn,  "shaders/BlinnVert.spv",    "shaders/BlinnFrag.spv", {&DSLGlobal, &DSLBlinn});
		
		// Init Models 
		MRoad.init(this, &VDEmission, "models/road_tile_2x2_005.mgcg", MCGC);


		// Init Textures
		TStreetLight[0].init(this, "textures/streetlight.png");

		// Text for Score or other stuff
		
		resetGame();
									
		
		//road initialization
		const int period = 12;

		for (int i = 0; i < 4 * period; i++) {
			glm::vec3 surface = glm::vec3({10.0f, 0.0f, (-i * 10.0f) + 5.0f});
			glm::vec3 normal = glm::vec3({0.0f, 1.0f, 0.0f});
			glm::vec2 UV = glm::vec2({0.0f, 0.0f});
			MRoad.vertices.push_back({surface, normal, UV});

			surface = glm::vec3({-10.0f, 0.0f, (-i * 10.0f) + 5.0f});
			UV = glm::vec2({0.0f, 1.0f});
			MRoad.vertices.push_back({surface, normal, UV});

			surface = glm::vec3({-10.0f, 0.0f, (-i * 10.0f) - 5.0f});
			UV = glm::vec2({1.0f, 1.0f});
			MRoad.vertices.push_back({surface, normal, UV});

			surface = glm::vec3({10.0f, 0.0f, (-i * 10.0f) - 5.0f});
			UV = glm::vec2({1.0f, 0.0f});
			MRoad.vertices.push_back({surface, normal, UV});
		}
		for (int i = 0; i < 4 * period; ++i) {
			MRoad.indices.push_back(i * 4);
			MRoad.indices.push_back(i * 4 + 2);
			MRoad.indices.push_back(i * 4 + 1);

			MRoad.indices.push_back(i * 4 + 2);
			MRoad.indices.push_back(i * 4);
			MRoad.indices.push_back(i * 4 + 3);
		}

		MRoad.initMesh(this, &VMesh);
		TRoad.init(this, "textures/road.png");
	}

	void pipelinesAndDescriptorSetsInit() {
		// create a new pipeline
		P.create();

		DSGlobal.init(this, &DSLGlobal, {});
	}
	
	void pipelinesAndDescriptorSetsCleanup() {
		DSGlobal.cleanup();

	}

	void localCleanup() {
		MRoad.cleanup();
		DSLGlobal.cleanup();
	}

	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
	    
		//Road : Add projection and DS
		//don't forget to bind projection to global ds!
		
		MRoad.bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(MRoad.indices.size()), 1, 0, 0, 0);

	}
	
	
    void updateUniformBuffer(uint32_t currentImage) {
		// here we put in the logic of the application
	}

	resetGame() {
		// reset camera pos 
		camera.yaw = 0.0f;
		camera.yawNew = 0.0f;
		camera.pos = glm::vec3(0.0f, 0.0f, 0.0f);
		camera.pitch = ((3.14159265358979323846264338327950288 / (2.5f)));
		camera.pitchNew = ((3.14159265358979323846264338327950288 / (2.5f)));
	}
}

// This is the main: probably you do not need to touch this!
int main() {
	InfiniteRun app;	
	
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}