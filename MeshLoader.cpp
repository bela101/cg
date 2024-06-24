// This has been adapted from the Vulkan tutorial

#include "Starter.hpp"
#include "TextMaker.hpp"
#define RENDER_DISTANCE 40
#define NUM_OF_TILES 16
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

struct UniformBlock
{
	alignas(16) glm::mat4 mvpMat;
};

struct UniformBufferObject
{
	alignas(16) glm::mat4 mvpMat;
	alignas(16) glm::mat4 mMat;
	alignas(16) glm::mat4 nMat;
};
struct GlobalUniformBufferObject
{
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec3 eyePos;
	alignas(16) glm::vec3 spotLightDir;
	alignas(16) glm::vec3 spotLightPos;
};

// The vertices data structures
// Example
struct Vertex
{
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 UV;
};

struct Car
{
	glm::vec3 pos;
	float speed;
};

struct Tile
{
	glm::vec3 pos;
};
struct Obstacle {
	glm::vec3 pos;
};
struct Scene {
	int gameState;
	float startTime;
	int text;
};

int camLoader = 0;
glm::vec3 camTarget;
glm::vec3 camPos;
int prevFire;

// MAIN !
class MeshLoader : public BaseProject
{
protected:
	// Current aspect ratio (used by the callback that resized the window
	float Ar;
	int initialized = 0;
	
	Car car;
	Tile tile[NUM_OF_TILES];
	Tile beach[NUM_OF_TILES * 2];
	Tile sides[NUM_OF_TILES / 4];

	std::string beachModel;
	Obstacle obstacle[NUM_OF_TILES];

	// Descriptor Layouts ["classes" of what will be passed to the shaders]
	DescriptorSetLayout DSL;
	DescriptorSetLayout DSLBlinn;
	DescriptorSetLayout DSLCookTorrance;

	// Vertex formats
	VertexDescriptor VD;
	VertexDescriptor VDBlinn;
	VertexDescriptor VDCookTorrance;

	// Pipelines [Shader couples]
	Pipeline P;
	Pipeline PBlinn;
	Pipeline PCookTorrance;

	// Models, textures and Descriptors (values assigned to the uniforms)
	// Please note that Model objects depends on the corresponding vertex structure
	// Models
	Model<Vertex> MTiles[NUM_OF_TILES];
	Model<Vertex> MBeach[NUM_OF_TILES * 2];
	Model<Vertex> MObstacles[NUM_OF_TILES];
	Model<Vertex> MOcean[NUM_OF_TILES/2];
	Model<Vertex> MSand[NUM_OF_TILES/2];
	Model<Vertex> MCar;
	Model<Vertex> MSkybox;
	Model<Vertex> MSplash1;
	Model<Vertex> MSplash2;

	// Descriptor sets
	DescriptorSet DS1, DS2, DS3; 
	DescriptorSet DSTiles[NUM_OF_TILES];
	DescriptorSet DSBeach[NUM_OF_TILES * 2];
	DescriptorSet DSObstacles[NUM_OF_TILES];
	DescriptorSet DSOcean[NUM_OF_TILES/2];
	DescriptorSet DSSand[NUM_OF_TILES/2];
	DescriptorSet DSSkybox;
	DescriptorSet DSSplash1;
	DescriptorSet DSSplash2;

	// Textures
	Texture T2;
	Texture TDungeon;
	Texture TOcean;
	Texture TSand;
	Texture TSkybox;
	Texture TSplash1;
	Texture TSplash2;

	// Text
	TextMaker score;

	// C++ storage for uniform variables
	UniformBlock ubo1, ubo2, ubo3, uboSplash1, uboSplash2; 
	UniformBufferObject ubo;
	GlobalUniformBufferObject gubo;

	// Other application parameters
	std::vector<SingleText> texts = {};
	Scene scene;

	// Here you set the main application parameters
	void setWindowParameters()
	{
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "Mesh Loader";
		windowResizable = GLFW_FALSE;
		initialBackgroundColor = { 0.0f, 0.005f, 0.01f, 1.0f };

		// Descriptor pool sizes
		uniformBlocksInPool = 168;
		texturesInPool = 87;
		setsInPool = 87;

		Ar = (float)windowWidth / (float)windowHeight;
	}

	void onWindowResize(int w, int h) {
		Ar = (float)w / (float)h;
	}

	// Here you load and setup all your Vulkan Models and Texutures.
	// Here you also create your Descriptor set layouts and load the shaders for the pipelines
	void localInit()
	{
		// Descriptor Layouts [what will be passed to the shaders]
		DSL.init(this, {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}, // DSL DS's have one Uniform Buffer
						{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}}); // DSL DS's have one texture

		DSLBlinn.init(this, {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}, // DSLBLINN DS's have two Uniform Buffers
							 {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}, // DSLBLINN DS's have one Texture
							 {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
						});

		DSLCookTorrance.init(this, {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}, // DSLCookTorrance DS's have two Uniform Buffers
									{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}, // DSLCookTorrance DS's have one Texture
									{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
						});


		VDBlinn.init(this, {{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}},
						{{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION},
						 {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV), sizeof(glm::vec2), UV},
						 {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm), sizeof(glm::vec3), NORMAL},
						});

		VDCookTorrance.init(this, {{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}},
						{{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION},
						 {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV), sizeof(glm::vec2), UV},
						 {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm), sizeof(glm::vec3), NORMAL},
						});
		
		// Vertex descriptors
		VD.init(this, {
				{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}},
				{{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION},
				 {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV), sizeof(glm::vec2), UV},
				 });

		// Pipelines [Shader couples]

		P.init(this, &VD, "shaders/ShaderVert.spv", "shaders/ShaderFrag.spv", { &DSL });
		P.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
							  VK_CULL_MODE_NONE, false);
		PBlinn.init(this, &VDBlinn, "shaders/blinnphongVert.spv", "shaders/blinnphongFrag.spv", {&DSLBlinn});
		PBlinn.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
							  VK_CULL_MODE_NONE, false);
		PCookTorrance.init(this, &VDCookTorrance, "shaders/cooktorranceVert.spv", "shaders/cooktorranceFrag.spv", {&DSLCookTorrance});
		PCookTorrance.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
							  VK_CULL_MODE_NONE, false);

		
		
		// Models, textures and Descriptors (values assigned to the uniforms)

		// Create models

		MCar.init(this, &VDBlinn, "models/transport_sport_001_transport_sport_001.001.mgcg", MGCG);
		MSplash1.init(this, &VDBlinn, "models/Square.obj", OBJ);
		MSplash2.init(this, &VDBlinn, "models/Square.obj", OBJ);
		
		// init models for tiles and obstacles
		for (int i = 0; i < NUM_OF_TILES; i++) {
			MTiles[i].init(this, &VDCookTorrance, "models/road_tile_2x2_005.mgcg", MGCG); // init Roadtiles 
			MObstacles[i].init(this, &VDBlinn, "models/box_005.mgcg", MGCG); // init Obstacles
		}
		
		//do the same for beach tiles
		for (int i = 0; i < NUM_OF_TILES * 2; i++) {
			//choose a random beach tile this allows for random order of tiles with every run of the game
			int random = rand() % 4 + 3;
			beachModel = "models/beach_tile_1x1_00" + std::to_string(random) + ".mgcg";
			MBeach[i].init(this, &VDBlinn, beachModel, MGCG);
		}

		//vertices for square (size)
		float sides_size = 32.0f;
		for (int i = 0; i < NUM_OF_TILES/2; i++) {
			//draw ocean and sand
			glm::vec3 surface = glm::vec3({ -sides_size, 0.0f, -sides_size });
			glm::vec3 normal = glm::vec3({ 0.0f, 1.0f, 0.0f });
			glm::vec2 UV = glm::vec2({ 0.0f, 0.0f });
			MOcean[i].vertices.push_back({ surface, normal, UV });
			MSand[i].vertices.push_back({ surface, normal, UV });

			surface = glm::vec3({ -sides_size, 0.0f, sides_size });
			UV = glm::vec2({ 0.0f, 1.0f });
			MOcean[i].vertices.push_back({ surface, normal, UV });
			MSand[i].vertices.push_back({ surface, normal, UV });

			surface = glm::vec3({ sides_size * 3, 0.0f, -sides_size });
			UV = glm::vec2({ 1.0f, 0.0f });
			MOcean[i].vertices.push_back({ surface, normal, UV });
			MSand[i].vertices.push_back({ surface, normal, UV });

			surface = glm::vec3({ sides_size * 3, 0.0f, sides_size });
			UV = glm::vec2({ 1.0f, 1.0f });
			MOcean[i].vertices.push_back({ surface, normal, UV });
			MSand[i].vertices.push_back({ surface, normal, UV });

			MOcean[i].indices = { 0, 1, 2, 1, 3, 2 };
			MSand[i].indices = { 0, 1, 2, 1, 3, 2 };
			MOcean[i].initMesh(this, &VDBlinn);
			MSand[i].initMesh(this, &VDBlinn);
		}

		//Create the skybox
		glm::vec3 surface = glm::vec3({ -sides_size*2, 0.0f, -sides_size });
		glm::vec3 normal = glm::vec3({ 0.0f, 1.0f, 0.0f });
		glm::vec2 UV = glm::vec2({ 0.0f, 0.0f });
		MSkybox.vertices.push_back({ surface, normal, UV });

		surface = glm::vec3({ -sides_size*2, 0.0f, sides_size });
		UV = glm::vec2({ 0.0f, 1.0f });
		MSkybox.vertices.push_back({ surface, normal, UV });

		surface = glm::vec3({ sides_size * 2, 0.0f, -sides_size });
		UV = glm::vec2({ 1.0f, 0.0f });
		MSkybox.vertices.push_back({ surface, normal, UV });

		surface = glm::vec3({ sides_size * 2, 0.0f, sides_size });
		UV = glm::vec2({ 1.0f, 1.0f });
		MSkybox.vertices.push_back({ surface, normal, UV });

		MSkybox.indices = { 0, 1, 2, 1, 3, 2 };
		MSkybox.initMesh(this, &VDBlinn);

		// Create the textures
		T2.init(this, "textures/Textures_City.png");
		TDungeon.init(this, "textures/Textures_Dungeon.png");
		TOcean.init(this, "textures/water.jpg");
		TSand.init(this, "textures/sand.jpg");
		TSkybox.init(this, "textures/day_skybox.jpg");
		TSplash1.init(this, "textures/splashScreenStart.png");
		TSplash2.init(this, "textures/splashScreenEnd.png");

		// Init local variables

		// Initial Positions of Roadtiles and Obstacles
		for(int i = 0; i < NUM_OF_TILES; i++){
			tile[i].pos.z = i * 16;
			obstacle[i].pos.z = i * 16;
			obstacle[i].pos.x = rand() % 8 - 4;
		}
		// Initial Positions of Beachtiles
		for (int i = 0; i < NUM_OF_TILES * 2; i++) {
			beach[i].pos.z = i * 8;
		}
		// Initial Positions of Ocean and Sand
		for (int i = 0; i < NUM_OF_TILES / 2; i++) {
			sides[i].pos.z = i * 64;
		}

		gubo.lightColor = glm::vec4(0.82f, 0.2f, 0.48f, 1.0f);
		gubo.lightDir = glm::normalize(glm::vec3(2.0f, 3.0f, -3.0f));
		gubo.spotLightDir = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f));
		scene.gameState = 0;
		scene.text = 0;
        texts.push_back({1, {"Infinite Run", "", "", ""}, 0, 0});

		texts[0].l[1] = (char *)malloc(200);
		// sprintf((char *)texts[0].l[1], "Pnt: %d",Pcnt);
		texts[0].l[2] = (char *)malloc(200);
		// sprintf((char *)texts[0].l[2], "Lin: %d",Lcnt);
		texts[0].l[3] = (char *)malloc(200);
		// sprintf((char *)texts[0].l[3], "Tri: %d",Tcnt);
        score.init(this, &texts);
	}

	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit()
	{
		// This creates a new pipeline (with the current surface), using its shaders
		P.create();
		PBlinn.create();
		PCookTorrance.create();

		// Here you define the data set

		DS1.init(this, &DSL, {
			{0, UNIFORM, sizeof(UniformBlock), nullptr},
			{1, TEXTURE, 0, &T2} });

		DS2.init(this, &DSLBlinn, { 
			{0, UNIFORM, sizeof(UniformBlock), nullptr}, 
			{1, TEXTURE, 0, &T2},
			{2, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr} 
		});

		DS3.init(this, &DSL, { 
			{0, UNIFORM, sizeof(UniformBlock), nullptr}, 
			{1, TEXTURE, 0, &T2} 
		});

		for (DescriptorSet &DSTiles: DSTiles) {
            DSTiles.init(this, &DSLCookTorrance, {
				{0, UNIFORM, sizeof(UniformBufferObject), nullptr}, 
				{1, TEXTURE, 0, &T2}, 
				{2, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr} 
			});
        }

		// do the same for beach tiles
		for (DescriptorSet &DSBeach : DSBeach) {
			DSBeach.init(this, &DSLBlinn, {
				{0, UNIFORM, sizeof(UniformBlock), nullptr}, 
				{1, TEXTURE, 0, &T2},
				{2, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr} 
			});
		}
		for (DescriptorSet &DSObstacles : DSObstacles) {
			DSObstacles.init(this, &DSLBlinn, {
				{0, UNIFORM, sizeof(UniformBlock), nullptr}, 
				{1, TEXTURE, 0, &T2},
				{2, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr} 
			});
		}
		for (DescriptorSet& DSOcean : DSOcean) {
			DSOcean.init(this, &DSLBlinn, { 
				{0, UNIFORM, sizeof(UniformBlock), nullptr}, 
				{1, TEXTURE, 0, &TOcean},
				{2, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr} 
			});
		}

		for (DescriptorSet& DSSand : DSSand) {
			DSSand.init(this, &DSLBlinn, { 
				{0, UNIFORM, sizeof(UniformBlock), nullptr}, 
				{1, TEXTURE, 0, &TSand},
				{2, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr} 
			});
		}

		DSSkybox.init(this, &DSLBlinn, {
			{0, UNIFORM, sizeof(UniformBlock), nullptr},
			{1, TEXTURE, 0, &TSkybox},
			{2, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr}
		});

		DSSplash1.init(this, &DSL, {
			{0, UNIFORM, sizeof(UniformBlock), nullptr},
			{1, TEXTURE, 0, &TSplash1}
		});

		DSSplash2.init(this, &DSL, {
			{0, UNIFORM, sizeof(UniformBlock), nullptr},
			{1, TEXTURE, 0, &TSplash2}
			});
		score.pipelinesAndDescriptorSetsInit();
	}

	// Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup()
	{
		// Cleanup pipelines
		P.cleanup();
		PBlinn.cleanup();
		PCookTorrance.cleanup();

		// Cleanup datasets
		DS1.cleanup();
		DS2.cleanup();
		DS3.cleanup();
		DSSkybox.cleanup();
		DSSplash1.cleanup();
		DSSplash2.cleanup();

		for (DescriptorSet &DSTiles: DSTiles) {
            DSTiles.cleanup();
        }
		for (DescriptorSet &DSBeach : DSBeach) {
			DSBeach.cleanup();
		}
		for (DescriptorSet &DSObstacles : DSObstacles) {
			DSObstacles.cleanup();
		}
		for (DescriptorSet &DSOcean : DSOcean) {
			DSOcean.cleanup();
		}
		for (DescriptorSet &DSSand : DSSand) {
			DSSand.cleanup();
		}

		score.pipelinesAndDescriptorSetsCleanup();
	}

	// Here you destroy all the Models, Texture and Desc. Set Layouts you created!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	// You also have to destroy the pipelines: since they need to be rebuilt, they have two different
	// methods: .cleanup() recreates them, while .destroy() delete them completely
	void localCleanup()
	{
		// Cleanup textures
		T2.cleanup();
		TDungeon.cleanup();
		TOcean.cleanup();
		TSand.cleanup();
		TSkybox.cleanup();
		TSplash1.cleanup();
		TSplash2.cleanup();

		// Cleanup models
		MCar.cleanup();
		MSkybox.cleanup();
		MSplash1.cleanup();
		MSplash2.cleanup();

		for (Model<Vertex> &MTiles: MTiles){
			MTiles.cleanup();
		}
		//beach cleanup
		for (Model<Vertex> &MBeach : MBeach) {
			MBeach.cleanup();
		}
		for (Model<Vertex> &MObstacles : MObstacles) {
			MObstacles.cleanup();
		}
		for (Model<Vertex>& MOcean : MOcean) {
			MOcean.cleanup();
		}
		for (Model<Vertex>& MSand : MSand) {
			MSand.cleanup();
		}

		// Cleanup descriptor set layouts
		DSL.cleanup();
		DSLBlinn.cleanup();
		DSLCookTorrance.cleanup();

		// Destroys the pipelines
		P.destroy();
		PBlinn.destroy();
		PCookTorrance.destroy();

		score.localCleanup();

		free((char *)texts[0].l[1]);
		free((char *)texts[0].l[2]);
		free((char *)texts[0].l[3]);
	}

	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures

	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage)
	{
		// binds the pipeline
		P.bind(commandBuffer);

		// ----------Start Splash----------
		MSplash1.bind(commandBuffer);
		DSSplash1.bind(commandBuffer, P, 0, currentImage);
		vkCmdDrawIndexed(commandBuffer,
									 static_cast<uint32_t>(MSplash1.indices.size()), 1, 0, 0, 0);

		//----------End Splash----------
		MSplash2.bind(commandBuffer);
		DSSplash2.bind(commandBuffer, P, 0, currentImage);
		vkCmdDrawIndexed(commandBuffer,
									static_cast<uint32_t>(MSplash2.indices.size()), 1, 0, 0, 0);

		// binds the data set
		DS1.bind(commandBuffer, P, 0, currentImage);

		// ----------BlinnPhong Pipeline----------
		PBlinn.bind(commandBuffer);
		MCar.bind(commandBuffer);
		DS2.bind(commandBuffer, PBlinn, 0, currentImage);
		vkCmdDrawIndexed(commandBuffer,
						 static_cast<uint32_t>(MCar.indices.size()), 1, 0, 0, 0);
		

		// Bind Dataset, Model and record drawing command in command buffer
		for (int i = 0; i < NUM_OF_TILES / 2; i++) {
			// ----------Ocean----------
			MOcean[i].bind(commandBuffer);
			DSOcean[i].bind(commandBuffer, PBlinn, 0, currentImage);
			vkCmdDrawIndexed(commandBuffer,
								static_cast<uint32_t>(MOcean[i].indices.size()), 1, 0, 0, 0);
			// ----------Sand----------
			MSand[i].bind(commandBuffer);
			DSSand[i].bind(commandBuffer, PBlinn, 0, currentImage);
			vkCmdDrawIndexed(commandBuffer,
												static_cast<uint32_t>(MSand[i].indices.size()), 1, 0, 0, 0);
		}

		// ----------Obstacles----------
		for (int i = 0; i < NUM_OF_TILES; i++){
			MObstacles[i].bind(commandBuffer);
			DSObstacles[i].bind(commandBuffer, PBlinn, 0, currentImage);
			vkCmdDrawIndexed(commandBuffer,
						 static_cast<uint32_t>(MObstacles[i].indices.size()), 1, 0, 0, 0);

		}

		// ----------Beachtiles----------
		for (int i = 0; i < NUM_OF_TILES * 2; i++) {
			MBeach[i].bind(commandBuffer);
			DSBeach[i].bind(commandBuffer, PBlinn, 0, currentImage);
			vkCmdDrawIndexed(commandBuffer,
						static_cast<uint32_t>(MBeach[i].indices.size()), 1, 0, 0, 0);
		}

		// ----------Skybox----------
		MSkybox.bind(commandBuffer);
		DSSkybox.bind(commandBuffer, PBlinn, 0, currentImage);
		vkCmdDrawIndexed(commandBuffer,
									 static_cast<uint32_t>(MSkybox.indices.size()), 1, 0, 0, 0);


		// ----------CookTorrance Pipeline----------
		PCookTorrance.bind(commandBuffer);
		for (int i = 0; i < NUM_OF_TILES; i++){
			// Roadtiles
			MTiles[i].bind(commandBuffer);
			DSTiles[i].bind(commandBuffer, PCookTorrance, 0, currentImage);
			vkCmdDrawIndexed(commandBuffer,
							 static_cast<uint32_t>(MTiles[i].indices.size()), 1, 0, 0, 0);
		}

		// ----------Score----------
		score.populateCommandBuffer(commandBuffer, currentImage, scene.text);

	}
	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage)
	{
		// Standard procedure to quit when the ESC key is pressed
		if (glfwGetKey(window, GLFW_KEY_ESCAPE))
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

		// Integration with the timers and the controllers
		float deltaT;
		glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
		bool fire = false;
		getSixAxis(deltaT, m, r, fire);

		// ----------GAME STATE----------
		// gameState 0 -> START -> Press Space to start
		// gameState 1 -> INGAME -> Collide to return to START
		if (scene.gameState == 0){
			if (!initialized){
				initPositions();
			}

			// Camera
			//Orthogonal projectiion for the splash
			glm::mat4 Port = glm::scale(glm::mat4(1.0f), glm::vec3(1, -1, 1)) * glm::ortho(-Ar, Ar, -1.0f, 1.0f, 0.1f, 100.0f);
			glm::mat4 View = glm::lookAt(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
			glm::mat4 World = glm::scale(glm::mat4(1.0f), glm::vec3(1.35f, 1.0f, 1.0f)) *
							glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0, 1, 0)) *
	 						glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));
			uboSplash1.mvpMat = Port * View * World;
			DSSplash1.map(currentImage, &uboSplash1, sizeof(uboSplash1), 0);

			World = glm::scale(glm::mat4(1.0f), glm::vec3(0.0f)) * World;
			uboSplash2.mvpMat = Port * View * World;
			DSSplash2.map(currentImage, &uboSplash2, sizeof(uboSplash2), 0);

			initialized = 1;
			scene.text = 0;

			if (fire) {
				prevFire = fire;
			}

			if (!fire && prevFire) {
				scene.gameState = 1;
				prevFire = fire;
			}
		}
		else if (scene.gameState == 1) {


			// Camera
			// Setup camera and matrices for model view projection matrix 
			// Camera FOV-y, Near Plane and Far Plane
			const float FOVy = glm::radians(60.0f);
			const float nearPlane = 0.1f;
			const float farPlane = 200.0f;

			glm::mat4 Port = glm::scale(glm::mat4(1.0f), glm::vec3(1, -1, 1)) * glm::ortho(-Ar, Ar, -1.0f, 1.0f, 0.1f, 100.0f);
			glm::mat4 View = glm::lookAt(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
			glm::mat4 World = glm::scale(glm::mat4(1.0f), glm::vec3(1.35f, 1.0f, 1.0f)) *
				glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0, 1, 0)) *
				glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));

			World = glm::scale(glm::mat4(1.0f), glm::vec3(0.0f)) * World;
			uboSplash1.mvpMat = Port * View * World;
			DSSplash1.map(currentImage, &uboSplash1, sizeof(uboSplash1), 0);

			glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
			Prj[1][1] *= -1;

			scene.text = 1;
			initialized = 0;
			// Controls
			car.speed += 4 * deltaT;
			car.speed = glm::clamp(car.speed, 0.0f, 50.0f);
			car.pos.z += 1 * car.speed * deltaT; // automatic forward movement
			car.pos.x += (int)m.x * 15.0f * deltaT;
			car.pos.x = glm::clamp(car.pos.x, -4.0f, 4.0f); // keep the car on the road

			// Collision against obstacles detection
			for (int i = 0; i < NUM_OF_TILES; i++) {
				if (car.pos.z < obstacle[i].pos.z + 2.0f && car.pos.z > obstacle[i].pos.z - 2.4f) {
					if (car.pos.x < -obstacle[i].pos.x + 1.5f && car.pos.x > -obstacle[i].pos.x - 1.5f) {
						std::cout << "Collision detected" << std::endl;
						scene.gameState = 2;
						//block the car 
						car.pos.z = glm::clamp(car.pos.z, 0.0f, obstacle[i].pos.z - 2.4f);

					}
				}
			}

			// Change cameras when fire is pressed (between 0, 1 and 2)

			if (fire) {
				prevFire = fire;
			}

			if (!fire && prevFire) {
				camLoader = (camLoader + 1) % 3;
				prevFire = fire;
			}

			//Create case for each camera
			switch (camLoader) {
			case 0:
				// Original cam (camera 0)
				camTarget = glm::vec3(0, 0, car.pos.z);
				camPos = camTarget + glm::vec3(0, 5, -20) / 2.0f;
				break;

			case 1:
				//First person cam (camera 1)
				camTarget = glm::vec3(-car.pos.x, 1, car.pos.z + 1);
				camPos = camTarget + glm::vec3(0, 0.5, -2.3f) / 2.0f;
				break;

			case 2:
				//Top down cam (camera 2)
				camTarget = glm::vec3(0, 0, car.pos.z + 13);
				camPos = camTarget + glm::vec3(0, 70, -10) / 2.0f;
				break;
			}

			View = glm::lookAt(camPos, camTarget, glm::vec3(0, 1, 0));

			// translate car movement
			World = glm::translate(glm::mat4(1), glm::vec3(-car.pos.x, 0, car.pos.z));
			ubo2.mvpMat = Prj * View * World;
			DS2.map(currentImage, &ubo2, sizeof(ubo2), 0);

			// Road Generation
			// TILE SIZE 16 x 16
			glm::mat4 World_Tiles[NUM_OF_TILES];
			for (int i = 0; i < NUM_OF_TILES; i++) {
				if (tile[i].pos.z + 16 < car.pos.z) {
					tile[i].pos.z = tile[i].pos.z + (NUM_OF_TILES) * 16;
				}
				World_Tiles[i] = glm::translate(glm::mat4(1), glm::vec3(0, 0, tile[i].pos.z)) *
					glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0));
				ubo.mMat = glm::translate(glm::mat4(1), glm::vec3(0, 0, tile[i].pos.z)) *
					glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0));
				ubo.mvpMat = Prj * View * ubo.mMat;
				ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));
				DSTiles[i].map(currentImage, &ubo, sizeof(ubo), 0);
				gubo.eyePos = camPos;
				gubo.spotLightDir = glm::normalize(glm::vec3(0, 0, 1));
				gubo.spotLightPos = glm::vec3(-(car.pos.x - 0.5f), car.pos.y, car.pos.z + 1.5f);
				DSTiles[i].map(currentImage, &gubo, sizeof(gubo), 2);
			}

			// Beach 
			glm::mat4 World_Beach[NUM_OF_TILES * 2];
			for (int i = 0; i < NUM_OF_TILES * 2; i++) {
				if (beach[i].pos.z + 16 < car.pos.z) {
					beach[i].pos.z = beach[i].pos.z + (NUM_OF_TILES * 2) * 8;
				}
				World_Beach[i] = glm::translate(glm::mat4(1), glm::vec3(12, 0, beach[i].pos.z)) *
					glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0));
				ubo1.mvpMat = Prj * View * World_Beach[i];
				DSBeach[i].map(currentImage, &ubo1, sizeof(ubo1), 0);
			}

			// Obstacle Generation 
			glm::mat4 World_Obstacles[NUM_OF_TILES];
			for (int i = 0; i < NUM_OF_TILES; i++) {
				if (obstacle[i].pos.z + 16 < car.pos.z) {
					obstacle[i].pos.z = obstacle[i].pos.z + (NUM_OF_TILES) * 16;
					obstacle[i].pos.x = rand() % 8 - 4; // random x position
				}
				World_Obstacles[i] = glm::translate(glm::mat4(1), glm::vec3(obstacle[i].pos.x, 0, obstacle[i].pos.z)) *
					glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0));
				ubo3.mvpMat = Prj * View * World_Obstacles[i];
				DSObstacles[i].map(currentImage, &ubo3, sizeof(ubo3), 0);
			}

			// Ocean and sand generation
			glm::mat4 World_Sides[NUM_OF_TILES / 2];
			for (int i = 0; i < NUM_OF_TILES / 2; i++) {
				if (sides[i].pos.z + 64 < car.pos.z) {
					sides[i].pos.z = sides[i].pos.z + (NUM_OF_TILES / 4) * 64;
				}
				//Ocean
				World_Sides[i] = glm::translate(glm::mat4(1), glm::vec3(38.0f, -1.0f, sides[i].pos.z));
				ubo1.mvpMat = Prj * View * World_Sides[i];
				DSOcean[i].map(currentImage, &ubo1, sizeof(ubo1), 0);

				//Sand
				World_Sides[i] = glm::translate(glm::mat4(1), glm::vec3(-38.0f, -1.0f, sides[i].pos.z)) *
					glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 1, 0));;
				ubo3.mvpMat = Prj * View * World_Sides[i];
				DSSand[i].map(currentImage, &ubo3, sizeof(ubo3), 0);
			}

			// Skybox
			glm::mat4 World_Skybox;
			World_Skybox = glm::translate(glm::mat4(1), glm::vec3(0, 35.0f, car.pos.z + 180)) * glm::scale(glm::mat4(1), glm::vec3(2.5f)) * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 1, 0)) *
				glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));
			ubo3.mvpMat = Prj * View * World_Skybox;
			DSSkybox.map(currentImage, &ubo3, sizeof(ubo3), 0);

		}

		if (scene.gameState == 2) {

			// Orthogonal projectiion for the splash

			glm::mat4 Port = glm::scale(glm::mat4(1.0f), glm::vec3(1, -1, 1)) * glm::ortho(-Ar, Ar, -1.0f, 1.0f, 0.1f, 100.0f);
			glm::mat4 View = glm::lookAt(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
			glm::mat4 World = glm::scale(glm::mat4(1.0f), glm::vec3(1.35f, 1.0f, 1.0f)) *
				glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0, 1, 0)) *
				glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));
			uboSplash2.mvpMat = Port * View * World;
			DSSplash2.map(currentImage, &uboSplash2, sizeof(uboSplash2), 0);

			scene.text = 0;

			if (fire) {
				prevFire = fire;
			}

			// Change to start screen
			if (!fire && prevFire) {
				scene.gameState = 0;
				prevFire = fire;
			}
		}

	}

	void initPositions(){
		car.pos.z = 0;
		car.pos.x = 0;
		car.speed = 0;
		for(int i = 0; i < NUM_OF_TILES; i++){
			tile[i].pos.z = i * 16;
			obstacle[i].pos.z = i * 16;
			obstacle[i].pos.x = rand() % 8 - 4;
		}
		// Initial Positions of Beachtiles
		for (int i = 0; i < NUM_OF_TILES * 2; i++) {
			beach[i].pos.z = i * 8;
		}
		// Initial Positions of Ocean and Sand
		for (int i = 0; i < NUM_OF_TILES / 2; i++) {
			sides[i].pos.z = i * 64;
		}
	}
};

// This is the main: probably you do not need to touch this!
int main()
{
	MeshLoader app;

	try
	{
		app.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}