// This has been adapted from the Vulkan tutorial

#include "Starter.hpp"
#include "TextMaker.hpp"
#define RENDER_DISTANCE 40
#define NUM_OF_TILES 16

// The uniform buffer objects data structures
// Remember to use the correct alignas(...) value
//        float : alignas(4)g
//        vec2  : alignas(8)
//        vec3  : alignas(16)
//        vec4  : alignas(16)
//        mat3  : alignas(16)
//        mat4  : alignas(16)
// Example:
struct UniformBlock
{
	alignas(16) glm::mat4 mvpMat;
};
struct GlobalUniformBlock{
	alignas(16) glm::vec3 DirectLightColor;
	alignas(16) glm::vec3 DirectLightPos;
	alignas(16) glm::vec3 AmbientLightColor;
	alignas(16) glm::vec3 CameraPos;
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
	// 
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

	// Descriptor sets
	DescriptorSet DS1, DS2, DS3; // DS4;
	DescriptorSet DSTiles[NUM_OF_TILES];
	DescriptorSet DSBeach[NUM_OF_TILES * 2];
	DescriptorSet DSObstacles[NUM_OF_TILES];
	DescriptorSet DSOcean[NUM_OF_TILES/2];
	DescriptorSet DSSand[NUM_OF_TILES/2];
	DescriptorSet DSSkybox;

	// Textures
	Texture /*T1,*/ T2;
	Texture TDungeon;
	Texture TOcean;
	Texture TSand;
	Texture TSkybox;

	// Text
	TextMaker score;

	// C++ storage for uniform variables
	UniformBlock ubo1, ubo2, ubo3; // ubo4;
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
		uniformBlocksInPool = 200;
		texturesInPool = 200;
		setsInPool = 200;

		Ar = (float)windowWidth / (float)windowHeight;
	}

	// What to do when the window changes size - don't remove this for now 
	void onWindowResize(int w, int h) {
		Ar = (float)w / (float)h;
	}

	// Here you load and setup all your Vulkan Models and Texutures.
	// Here you also create your Descriptor set layouts and load the shaders for the pipelines
	void localInit()
	{
		// Descriptor Layouts [what will be passed to the shaders]
		DSL.init(this, {// this array contains the bindings:
						// first  element : the binding number
						// second element : the type of element (buffer or texture)
						//                  using the corresponding Vulkan constant
						// third  element : the pipeline stage where it will be used
						//                  using the corresponding Vulkan constant
						{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}, // DSL DS's have one Uniform Buffer
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
		VD.init(this, {// this array contains the bindings
					   // first  element : the binding number
					   // second element : the stride of this binging
					   // third  element : whether this parameter change per vertex or per instance
					   //                  using the corresponding Vulkan constant
					   {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}},
				{{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION},
				 {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV), sizeof(glm::vec2), UV},
				// this array contains the location
				 // first  element : the binding number
				 // second element : the location number
				 // third  element : the offset of this element in the memory record
				 // fourth element : the data type of the element
				 //                  using the corresponding Vulkan constant
				 // fifth  elmenet : the size in byte of the element
				 // sixth  element : a constant defining the element usage
				 //                   POSITION - a vec3 with the position
				 //                   NORMAL   - a vec3 with the normal vector
				 //                   UV       - a vec2 with a UV coordinate
				 //                   COLOR    - a vec4 with a RGBA color
				 //                   TANGENT  - a vec4 with the tangent vector
				 //                   OTHER    - anything else
				 //
				 // ***************** DOUBLE CHECK ********************
				 //    That the Vertex data structure you use in the "offsetoff" and
				 //	in the "sizeof" in the previous array, refers to the correct one,
				 //	if you have more than one vertex format!
				 // ***************************************************
				 });

		// Pipelines [Shader couples]
		// The second parameter is the pointer to the vertex definition
		// Third and fourth parameters are respectively the vertex and fragment shaders
		// The last array, is a vector of pointer to the layouts of the sets that will
		// be used in this pipeline. The first element will be set 0, and so on..
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
		// The second parameter is the pointer to the vertex definition for this model
		// The third parameter is the file name
		// The last is a constant specifying the file type: currently only OBJ or GLTF
		// MCar.init(this, &VD, "models/transport_sport_001_transport_sport_001.001.mgcg", MGCG);
		MCar.init(this, &VDBlinn, "models/transport_sport_001_transport_sport_001.001.mgcg", MGCG);
		
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

		scene.gameState = 0;
		scene.text = 0;
        texts.push_back({1, {"Infinite Run - Press Space to Start", "", "", ""}, 0, 0});
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
			// the second parameter, is a pointer to the Uniform Set Layout of this set
			// the last parameter is an array, with one element per binding of the set.
			// first  elmenet : the binding number
			// second element : UNIFORM or TEXTURE (an enum) depending on the type
			// third  element : only for UNIFORMs, the size of the corresponding C++ object. For texture, just put 0
			// fourth element : only for TEXTUREs, the pointer to the corresponding texture object. For uniforms, use nullptr
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

		// DS4.cleanup();

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
		// T1.cleanup();
		T2.cleanup();
		TDungeon.cleanup();
		TOcean.cleanup();
		TSand.cleanup();
		TSkybox.cleanup();

		// Cleanup models
		MCar.cleanup();
		MSkybox.cleanup();

		// M4.cleanup();
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
		// std::cout << "HI";
		// binds the pipeline
		// For a pipeline object, this command binds the corresponding pipeline to the command buffer passed in its parameter
		P.bind(commandBuffer);

		// binds the data set
		DS1.bind(commandBuffer, P, 0, currentImage);
		// For a Dataset object, this command binds the corresponding dataset
		// to the command buffer and pipeline passed in its first and second parameters.
		// The third parameter is the number of the set being bound
		// As described in the Vulkan tutorial, a different dataset is required for each image in the swap chain.
		// This is done automatically in file Starter.hpp, however the command here needs also the index
		// of the current image in the swap chain, passed in its last parameter

		// binds the model
		// For a Model object, this command binds the corresponding index and vertex buffer
		// to the command buffer passed in its parameter

		// record the drawing command in the command buffer
		// the second parameter is the number of indexes to be drawn. For a Model object,
		// this can be retrieved with the .indices.size() method.

		// ----------BlinnPhong Pipeline----------
		PBlinn.bind(commandBuffer);
		MCar.bind(commandBuffer);
		DS2.bind(commandBuffer, PBlinn, 0, currentImage);
		vkCmdDrawIndexed(commandBuffer,
						 static_cast<uint32_t>(MCar.indices.size()), 1, 0, 0, 0);
		

		// Bind Dataset, Model and record drawing command in command buffer
		for (int i = 0; i < NUM_OF_TILES / 2; i++) {
			// Ocean
			MOcean[i].bind(commandBuffer);
			DSOcean[i].bind(commandBuffer, PBlinn, 0, currentImage);
			vkCmdDrawIndexed(commandBuffer,
								static_cast<uint32_t>(MOcean[i].indices.size()), 1, 0, 0, 0);
			// Sand
			MSand[i].bind(commandBuffer);
			DSSand[i].bind(commandBuffer, PBlinn, 0, currentImage);
			vkCmdDrawIndexed(commandBuffer,
												static_cast<uint32_t>(MSand[i].indices.size()), 1, 0, 0, 0);
		}


		// Bind Dataset, Model and record drawing command in command buffer
		for (int i = 0; i < NUM_OF_TILES; i++){
			// Obstacles
			MObstacles[i].bind(commandBuffer);
			DSObstacles[i].bind(commandBuffer, PBlinn, 0, currentImage);
			vkCmdDrawIndexed(commandBuffer,
						 static_cast<uint32_t>(MObstacles[i].indices.size()), 1, 0, 0, 0);

		}

		// Bind Dataset, Model and record drawing command in command buffer
		for (int i = 0; i < NUM_OF_TILES * 2; i++) {
			// Beachtiles
			MBeach[i].bind(commandBuffer);
			DSBeach[i].bind(commandBuffer, PBlinn, 0, currentImage);
			vkCmdDrawIndexed(commandBuffer,
						static_cast<uint32_t>(MBeach[i].indices.size()), 1, 0, 0, 0);
		}

		// ----------CookTorrance Pipeline----------
		PCookTorrance.bind(commandBuffer);
		for (int i = 0; i < NUM_OF_TILES; i++){
			// Roadtiles
			MTiles[i].bind(commandBuffer);
			DSTiles[i].bind(commandBuffer, PCookTorrance, 0, currentImage);
			vkCmdDrawIndexed(commandBuffer,
							 static_cast<uint32_t>(MTiles[i].indices.size()), 1, 0, 0, 0);
		}

		// ----------Skybox----------
		MSkybox.bind(commandBuffer);
		DSSkybox.bind(commandBuffer, PBlinn, 0, currentImage);
		vkCmdDrawIndexed(commandBuffer,
									 static_cast<uint32_t>(MSkybox.indices.size()), 1, 0, 0, 0);

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
		// getSixAxis() is defined in Starter.hpp in the base class.
		// It fills the float point variable passed in its first parameter with the time
		// since the last call to the procedure.
		// It fills vec3 in the second parameters, with three values in the -1,1 range corresponding
		// to motion (with left stick of the gamepad, or ASWD + RF keys on the keyboard)
		// It fills vec3 in the third parameters, with three values in the -1,1 range corresponding
		// to motion (with right stick of the gamepad, or Arrow keys + QE keys on the keyboard, or mouse)
		// If fills the last boolean variable with true if fire has been pressed:
		//          SPACE on the keyboard, A or B button on the Gamepad, Right mouse button


		// Camera
		// Setup camera and matrices for model view projection matrix 
		// Camera FOV-y, Near Plane and Far Plane
		const float FOVy = glm::radians(60.0f);
		const float nearPlane = 0.1f;
		const float farPlane = 200.0f;

		glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
		Prj[1][1] *= -1;

		// ----------GAME STATE----------
		// gameState 0 -> START -> Press Space to start
		// gameState 1 -> INGAME -> Collide to return to START
		if (scene.gameState == 0){
			if (!initialized){
				initPositions();
			}
			initialized = 1;
			texts.pop_back();
			scene.text = 0;

			if (fire) {
				prevFire = fire;
			}

			if (!fire && prevFire) {
				scene.gameState = 1;
				prevFire = fire;
			}
		} else if (scene.gameState == 1) {
			scene.text = 1;
			initialized = 0;
			// Controls
			car.pos.z += 1 * 50.0f * deltaT; // allow only forward movement UNCOMMENT WHEN COLLISION DETECTION IS IMPLEMENTED
			// car.pos.z += m.z * 70.0f * deltaT; // until COLLISION DETECTION is implemented we allow backwards driving for testing 
			car.pos.x += (int)m.x * 20.0f * deltaT;
			car.pos.x = glm::clamp(car.pos.x, -4.0f, 4.0f); // keep the car on the road

			// Collision against obstacles detection
			for (int i = 0; i < NUM_OF_TILES; i++) {
				if (car.pos.z < obstacle[i].pos.z + 2.0f && car.pos.z > obstacle[i].pos.z - 2.4f) {
					if (car.pos.x < -obstacle[i].pos.x + 1.5f && car.pos.x > -obstacle[i].pos.x - 1.5f) {
						std::cout << "Collision detected" << std::endl;
						scene.gameState = 0;
						//block the car 
						car.pos.z = glm::clamp(car.pos.z, 0.0f, obstacle[i].pos.z-2.4f);
						
					}
				}
			}

			// Change cameras when fire is pressed (between 0, 1 and 2)
			if (fire) {
				static int cam = 0;
				cam = (cam + 1) % 3;
			}

			// Change cameras when fire is pressed (between 0, 1 and 2)
			// just change when fire was pressed and it gets released
			if (fire) {
				prevFire = fire;
			}

			if (!fire && prevFire) {
				camLoader = (camLoader + 1) % 3;
				prevFire = fire;
			}
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

		// glm::vec3 camPos = camTarget + glm::vec3(0, 50, -80) / 2.0f; //debugging cam

		// Init Gubo
		gubo.lightColor = glm::vec4(0.82f, 0.2f, 0.48f, 1.0f);
		gubo.lightDir = glm::normalize(glm::vec3(2.0f, 3.0f, -3.0f));
		gubo.eyePos = camPos;


		glm::mat4 View = glm::lookAt(camPos, camTarget, glm::vec3(0, 1, 0));
		glm::mat4 World;

		// translate car movement
		World = glm::translate(glm::mat4(1), glm::vec3(-car.pos.x, 0, car.pos.z));
		ubo2.mvpMat = Prj * View * World;
		DS2.map(currentImage, &ubo2, sizeof(ubo2), 0);
		// the .map() method of a DataSet object, requires the current image of the swap chain as first parameter
		// the second parameter is the pointer to the C++ data structure to transfer to the GPU
		// the third parameter is its size
		// the fourth parameter is the location inside the descriptor set of this uniform block


		// Road Generation
		// TILE SIZE 16 x 16
		glm::mat4 World_Tiles [NUM_OF_TILES];
		for (int i = 0; i < NUM_OF_TILES ; i++){
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
			DSTiles[i].map(currentImage, &gubo, sizeof(gubo), 2); 
		}

		// Beach (8 x 8)
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
		for (int i = 0; i < NUM_OF_TILES ; i++){
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
		glm::mat4 World_Sides[NUM_OF_TILES/2];
		for (int i = 0; i < NUM_OF_TILES/2; i++) {
			if (sides[i].pos.z + 64 < car.pos.z) {
				sides[i].pos.z = sides[i].pos.z + (NUM_OF_TILES/4) * 64;
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


		 
		glm::mat4 World_Skybox;
		World_Skybox = glm::translate(glm::mat4(1), glm::vec3(0, 35.0f, car.pos.z + 180)) * glm::scale(glm::mat4(1), glm::vec3(2.5f)) * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 1, 0)) *
		 		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));
		 ubo3.mvpMat = Prj * View * World_Skybox;
		 DSSkybox.map(currentImage, &ubo3, sizeof(ubo3), 0);

		// DS3.map(currentImage, &ubo3, sizeof(ubo3), 0);

	}

	void initPositions(){
		car.pos.z = 0;
		car.pos.x = 0;
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