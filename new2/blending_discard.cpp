#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <cmath>

#define MAZE_SIZE 13
#define PI 3.1416f

// #define GAME_START 2
#define GAME_ON 0
#define GAME_WON 1

// Game state
int gameState = 0;


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(1.0f, 0.0f, 1.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//Array for map layout. 0 for wall cubes and 2 for ground
typedef struct {
	int x, y; //Node position - little waste of memory, but it allows faster generation
	void *parent; //Pointer to parent node
	char c; //Character to be displayed
	char dirs; //Directions that still haven't been explored
} Node;

Node *nodes; //Nodes array
const int width=MAZE_SIZE, height=MAZE_SIZE; //Maze dimensions
int maze[width][height];
int diamondx, diamondz;

int init_maze( ) {
	int i, j;
	Node *n;

	diamondx = 1 + 2*(rand()%(width/2)); // *2 for world coordinate
	diamondz = 1 + 2*(rand()%(height/2));

	//Allocate memory for maze
	nodes = (Node*)calloc( width * height, sizeof( Node ) );
	if ( nodes == NULL ) return 1;

	//Setup crucial nodes
	for ( i = 0; i < width; i++ ) {
		for ( j = 0; j < height; j++ ) {
			n = nodes + i + j * width;
			if ( i * j % 2 ) {
				n->x = i;
				n->y = j;
				n->dirs = 15; //Assume that all directions can be explored (4 youngest bits set)
				n->c = ' ';
			}
			else n->c = '#'; //Add walls between nodes
		}
	}
	return 0;
}

Node *link_node( Node *n ) {
	//Connects node to random neighbor (if possible) and returns
	//address of next node that should be visited
	int x, y;
	char dir;
	Node *dest;

	//Nothing can be done if null pointer is given - return
	if ( n == NULL ) return NULL;

	//While there are directions still unexplored
	while ( n->dirs ) {
		//Randomly pick one direction
		dir = ( 1 << ( rand( ) % 4 ) );

		//If it has already been explored - try again
		if ( ~n->dirs & dir ) continue;

		//Mark direction as explored
		n->dirs &= ~dir;

		//Depending on chosen direction
		switch ( dir ) {
			//Check if it's possible to go right
			case 1:
				if ( n->x + 2 < width ) {
					x = n->x + 2;
					y = n->y;
				}
				else continue;
				break;

			//Check if it's possible to go down
			case 2:
				if ( n->y + 2 < height ) {
					x = n->x;
					y = n->y + 2;
				}
				else continue;
				break;

			//Check if it's possible to go left
			case 4:
				if ( n->x - 2 >= 0 ) {
					x = n->x - 2;
					y = n->y;
				}
				else continue;
				break;

			//Check if it's possible to go up
			case 8:
				if ( n->y - 2 >= 0 ) {
					x = n->x;
					y = n->y - 2;
				}
				else continue;
				break;
		}

		//Get destination node into pointer (makes things a tiny bit faster)
		dest = nodes + x + y * width;

		//Make sure that destination node is not a wall
		if ( dest->c == ' ' ) {
			//If destination is a linked node already - abort
			if ( dest->parent != NULL ) continue;

			//Otherwise, adopt node
			dest->parent = n;

			//Remove wall between nodes
			nodes[n->x + ( x - n->x ) / 2 + ( n->y + ( y - n->y ) / 2 ) * width].c = ' ';

			//Return address of the child node
			return dest;
		}
	}

	//If nothing more can be done here - return parent's address
	return (Node*)(n->parent);
}

void draw( ) {
	//Outputs maze to terminal - nothing special
	for ( int i = 0; i < height; i++ ) {
		for ( int j = 0; j < width; j++ ) {
			cout<<nodes[j + i * width].c;
      if(nodes[j + i * width].c == '#') {
        maze[i][j] = 0;
      }
      else {
        if((rand()%2 & rand()%2 )) maze[i][j] = 2;
        else maze[i][j] = 1;
      }
		}
		cout<<"\n";
	}
}

void mazeGen() {
	Node *start, *last;
	//Seed random generator
	srand( time( NULL ) );

	//Initialize maze
	if ( init_maze( ) ) {
		fprintf( stderr, "out of memory for maze!\n" );
		exit( 1 );
	}

	//Setup start node
	start = nodes + 1 + width;
	start->parent = start;
	last = start;
	//Connect nodes until start node is reached and can't be left
	while ( ( last = link_node( last ) ) != start );
	draw();
}

bool mapView = false;
glm::vec3 oldPos, oldFront, oldUp;


int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    // glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

     // build and compile shaders
    // -------------------------
    Shader shader("3.1.blending.vs", "3.1.blending.fs");
    Shader diamondShader("3.2.materials.vs", "3.2.materials.fs");
    Shader lampShader("3.2.lamp.vs", "3.2.lamp.fs");
   

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float cubeVertices[] = {
        // brick wall
        // positions          // texture Coords
        // right face
        -0.5f, -0.5f,  0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 1.0f,

        // left face
        -0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
        
        // frontside
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
         
        // backside
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f,

        // top
        -0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
        // bottom
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f
    };

    mazeGen();

    float planeVertices[] = {
        // floor
        // positions          // texture Coords 
         0.5f, -0.5f,  0.5f,  2.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 2.0f,

         0.5f, -0.5f,  0.5f,  2.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 2.0f,
         0.5f, -0.5f, -0.5f,  2.0f, 2.0f
    };
    float transparentVertices[] = {
        // grass
        // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
        -0.2f, -0.1f,  0.0f,  0.0f,  0.0f,
        -0.2f, -0.5f,  0.0f,  0.0f,  1.0f,
         0.2f, -0.5f,  0.0f,  1.0f,  1.0f,

        -0.2f, -0.1f,  0.0f,  0.0f,  0.0f,
         0.2f, -0.5f,  0.0f,  1.0f,  1.0f,
         0.2f, -0.1f,  0.0f,  1.0f,  0.0f
    };
    // cube VAO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    // plane VAO
    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    // transparent VAO
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    // lighting
    glm::vec3 lightPos(diamondx, 2.0f, diamondz);


    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float diamondVertices[] = {
        // upper four triangular faces
        0.0, 1.0, 0.0, -1.0, 1.0, -1.0,
        -1.0, 0.0, 0.0, -1.0, 1.0, -1.0, 
        0.0, 0.0, -1.0, -1.0, 1.0, -1.0,

        0.0, 1.0, 0.0, 1.0, 1.0, -1.0,
        1.0, 0.0, 0.0, 1.0, 1.0, -1.0,
        0.0, 0.0, -1.0, 1.0, 1.0, -1.0,

        0.0, 1.0, 0.0, 1.0, 1.0, 1.0,
        1.0, 0.0, 0.0, 1.0, 1.0, 1.0,
        0.0, 0.0, 1.0, 1.0, 1.0, 1.0,

        0.0, 1.0, 0.0, -1.0, 1.0, 1.0,
        0.0, 0.0, 1.0, -1.0, 1.0, 1.0,
        -1.0, 0.0, 0.0, -1.0, 1.0, 1.0,

        0.0, -1.0, 0.0, -1.0, -1.0, -1.0,
        -1.0, 0.0, 0.0, -1.0, -1.0, -1.0,
        0.0, 0.0, -1.0, -1.0, -1.0, -1.0,

        0.0, -1.0, 0.0, 1.0, -1.0, -1.0,
        1.0, 0.0, 0.0, 1.0, -1.0, -1.0,
        0.0, 0.0, -1.0, 1.0, -1.0, -1.0,

        0.0, -1.0, 0.0, 1.0, -1.0, 1.0,
        1.0, 0.0, 0.0, 1.0, -1.0, 1.0,
        0.0, 0.0, 1.0, 1.0, -1.0, 1.0,

        0.0, -1.0, 0.0, -1.0, -1.0, 1.0,
        0.0, 0.0, 1.0, -1.0, -1.0, 1.0,
        -1.0, 0.0, 0.0, -1.0, -1.0, 1.0
    };

    float player[] = {
        -0.5,0.0,0.5, 0.0,1.0,0.0,
        0.5,0.0,0.5, 0.0,1.0,0.0,
        0.0,0.0,0.0, 0.0,1.0,0.0,

        -0.5,0.0,0.0, 0.0,1.0,0.0,
        0.5,0.0,0.0, 0.0,1.0,0.0,
        0.0,0.0,-0.5, 0.0,1.0,0.0
    };
    // first, configure the cube's VAO (and VBO)
    unsigned int VBO, diamondVAO;
    glGenVertexArrays(1, &diamondVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(diamondVertices), diamondVertices, GL_STATIC_DRAW);

    glBindVertexArray(diamondVAO);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    // second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
    unsigned int lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // note that we update the lamp's position attribute's stride to reflect the updated buffer data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int playerVBO, playerVAO;
    glGenVertexArrays(1, &playerVAO);
    glGenBuffers(1, &playerVBO);

    glBindBuffer(GL_ARRAY_BUFFER, playerVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(player), player, GL_STATIC_DRAW);

    glBindVertexArray(playerVAO);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    
    // load textures
    // -------------
    unsigned int cubeTexture = loadTexture(FileSystem::getPath("resources/textures/wall.png").c_str());
    unsigned int floorTexture = loadTexture(FileSystem::getPath("resources/textures/tile.png").c_str());
    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/grass.png").c_str());

    
    // shader configuration
    // --------------------
    shader.use();
    shader.setInt("texture1", 0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw objects
        shader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        //cubes
        glBindVertexArray(cubeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cubeTexture);
        for (GLuint i = 0; i < MAZE_SIZE; i++)
        for (GLuint j = 0; j < MAZE_SIZE; j++)
        {   
            if(maze[i][j]==0){
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(i, 0.0f, j));
                shader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        

        // floor
        glBindVertexArray(planeVAO);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
       
        for (GLuint i = 0; i < MAZE_SIZE; i++)
        for (GLuint j = 0; j < MAZE_SIZE; j++)
        {   
            if(maze[i][j]!=0){
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(i, 0.0f, j));
                shader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        // vegetation
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
       
        for (GLuint i = 0; i < MAZE_SIZE; i++)
        for (GLuint j = 0; j < MAZE_SIZE; j++)
        {   
            if(maze[i][j]==2){
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(i, 0.0f, j));
                shader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(i, 0.0f, j));
                model = glm::rotate(model,PI/4,glm::vec3(0.0f, 1.0f, 0.0f));
                shader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(i, 0.0f, j));
                model = glm::rotate(model,-PI/4,glm::vec3(0.0f, 1.0f, 0.0f));
                shader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }


        // be sure to activate shader when setting uniforms/drawing objects
        diamondShader.use();
        diamondShader.setVec3("light.position", lightPos);
        diamondShader.setVec3("viewPos", camera.Position);

        // light properties
        diamondShader.setVec3("light.ambient", 1.0f, 1.0f, 1.0f); // note that all light colors are set at full intensity
        diamondShader.setVec3("light.diffuse", 1.0f, 1.0f, 1.0f);
        diamondShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

        // material properties
        diamondShader.setVec3("material.ambient", 0.0f, 0.1f, 0.5f);
        diamondShader.setVec3("material.diffuse", 0.0f, 0.50980392f, 0.50980392f);
        diamondShader.setVec3("material.specular", 0.50196078f, 0.50196078f, 0.50196078f);
        diamondShader.setFloat("material.shininess", 32.0f);

    
        diamondShader.setMat4("projection", projection);
        diamondShader.setMat4("view", view);

        // render the cube
        if(gameState==GAME_ON){
        glBindVertexArray(diamondVAO);
        
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(diamondx, 0.0f, diamondz));
        model = glm::scale(model, glm::vec3(0.3f));
        model = glm::rotate(model,PI/8,glm::vec3(0.0f, 1.0f, 0.0f));
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 24);
        }

        if(mapView){
            glBindVertexArray(playerVAO);
            diamondShader.setVec3("material.ambient", 1.0f, 0.0f, 0.0f);
            model = glm::mat4(1.0f);
            model = glm::translate(model, oldPos);
            model = glm::scale(model, glm::vec3(0.6f));
            float theta;
            if(oldFront.x>0 && oldFront.z>0){
                theta = acos(oldFront.x)+PI/2;
            }else if(oldFront.x<0 && oldFront.z>0){
                theta = -(acos(-oldFront.x)+PI/2);
            }else if(oldFront.x<0 && oldFront.z<0){
                theta = -acos(-oldFront.z);
            }else {
                theta = acos(-oldFront.z);
            }
            model = glm::rotate(model,theta,glm::vec3(0.0f, 1.0f, 0.0f));
            shader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        
        if(!mapView){
            // also draw the lamp object
            lampShader.use();
            lampShader.setMat4("projection", projection);
            lampShader.setMat4("view", view);
            model = glm::mat4(1.0f);
            model = glm::translate(model, lightPos);
            model = glm::scale(model, glm::vec3(0.1f)); 
            lampShader.setMat4("model", model);

            glBindVertexArray(lightVAO);
            glDrawArrays(GL_TRIANGLES, 0, 24);
        }

        
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &planeVBO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------

void showMapView(){
    if(!mapView){
        // render the map view
        oldPos = camera.Position;
        oldUp = camera.Up;
        oldFront = camera.Front;
        camera.Position = glm::vec3(MAZE_SIZE/2,25.0,MAZE_SIZE/2);
        camera.Front = glm::vec3(0.0, -1.0, 0.0);
        camera.Up = glm::vec3(1.0, 0.0, 1.0);
    }
    mapView = true;
}
void showNormalView(){
    if(mapView){
        // restore the maze view
        camera.Position = oldPos;
        camera.Up = oldUp;
        camera.Front = oldFront;
    }
    mapView = false;
}



 void processKeyboard(Camera_Movement direction, float deltaTime)
    {
        if(gameState == GAME_WON || mapView) return;
        float velocity = camera.MovementSpeed * deltaTime;
        glm::vec3 newPosition = camera.Position;
        if (direction == FORWARD)
            // near plane needs to be taken care of 
            newPosition += camera.Front * velocity;
        if (direction == BACKWARD)
            newPosition -= camera.Front * velocity;
        if (direction == LEFT)
            newPosition -= camera.Right * velocity;
        if (direction == RIGHT)
            newPosition += camera.Right * velocity;

        int i,j;
        i = round(newPosition.x);
        j = round(newPosition.z);
        if(i==diamondx && j==diamondz){
            // GAME WON
            gameState = GAME_WON;
        }
        if(maze[i][j]!=0){
            camera.Position = newPosition;
        }
    }

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        processKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        processKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        processKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        processKeyboard(RIGHT, deltaTime);
    
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS){
        showMapView();        
    }
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS){
        showNormalView();
    }
        
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{   
    if(mapView) return;
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
