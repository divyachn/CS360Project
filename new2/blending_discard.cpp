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

#define MAZE_SIZE 13
#define PI 3.1416f

#define GAME_START 0
#define GAME_ON 1
#define GAME_WON 2

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
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
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

	diamondx = 2*(1 + 2*(rand()%(width/2))); // *2 for world coordinate
	diamondz = 2*(1 + 2*(rand()%(height/2)));

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

struct materials_t {
	float ambient[4];
	float diffuse[4];
	float specular[4];
	float shininess;
};

struct light_t {
	int id;
	size_t name;
	float ambient[4];
	float diffuse[4];
	float specular[4];
};

//Materials
const materials_t materialM = {
	{0.33f, 0.22f, 0.03f, 1.0f},
	{0.78f, 0.57f, 0.11f, 1.0f},
	{0.90f, 0.8f, 0.7f, 1.0f},
	27.8f
};

//Universal light (Shade of white)
light_t light_1 = {
	1, GL_LIGHT1,
	{ 0.01, 0.01, 0.20, 1 },
	{ 0.5, 0.5, 0.6, 1 },
	{0.3f, 0.3f, 0.3f, 1.0f}
};

// For welcome screen
light_t light_3 = {
	1, GL_LIGHT1,
	{ 0.8, 0.7, 0.7, 1 },
	{ 0.5, 0.5, 0.6, 1 },
	{0.3f, 0.3f, 0.3f, 1.0f}
};

//Yellowish light for diamond
light_t light_2 = {
	2, GL_LIGHT2,
	{ 0.4, 0.8, 0.1, 1 },
	{ 0.7, 0.8, 0.5, 1 },
	{0.5f, 0.5f, 0.5f, 1.0f}
};

//Properties of a given material
void set_material(const materials_t& mat) {
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat.ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat.diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat.specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mat.shininess);
}

//Set a given light
void set_light(const light_t& light, float lightx, float lightz) {
	float lighty;
	float intensity;
	float dir;

	//Different properties for diamond light
	if (light.id == 1) {
		lighty = 5;
		intensity = 91;
		dir = 1;
	} else {
		lighty = 2;
		intensity = 50;
		dir = -1;
	}

	float position[4] = {lightx, lighty, lightz, 1};
	glLightfv(light.name, GL_AMBIENT, light.ambient);
	glLightfv(light.name, GL_DIFFUSE, light.diffuse);
	glLightfv(light.name, GL_SPECULAR, light.specular);
	glLightfv(light.name, GL_POSITION, position);

	//Spotlight adjustment calls
	float direction[3] = {0, dir, 0};
	glLightfv(light.name, GL_SPOT_DIRECTION, direction);
	glLightf(light.name, GL_SPOT_CUTOFF, intensity);
	glEnable(light.name);
}


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
    glfwSetScrollCallback(window, scroll_callback);

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
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	set_material (materialM);
    

     // build and compile shaders
    // -------------------------
    Shader shader("3.1.blending.vs", "3.1.blending.fs");

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

    // load textures
    // -------------
    unsigned int cubeTexture = loadTexture(FileSystem::getPath("resources/textures/wall.png").c_str());
    unsigned int floorTexture = loadTexture(FileSystem::getPath("resources/textures/metal.png").c_str());
    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/grass.png").c_str());

    // transparent vegetation locations
    // --------------------------------
    // vector<glm::vec3> vegetation 
    // {
    //     glm::vec3( 1.5f, 0.0f, 1.5f),
    //     glm::vec3( 3.5f, 0.0f, 9.5f),
    //     glm::vec3( 5.0f, 0.0f, 3.7f),
    //     glm::vec3( 7.3f, 0.0f, 5.3f),
    //     glm::vec3 (9.5f, 0.0f, 7.6f)
    // };

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
        // cubes
        // glBindVertexArray(cubeVAO);
        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, cubeTexture);
        // for (GLuint i = 0; i < MAZE_SIZE; i++)
        // for (GLuint j = 0; j < MAZE_SIZE; j++)
        // {   
        //     if(maze[i][j]==0){
        //         model = glm::mat4(1.0f);
        //         model = glm::translate(model, glm::vec3(i, 0.0f, j));
        //         shader.setMat4("model", model);
        //         glDrawArrays(GL_TRIANGLES, 0, 36);
        //     }
        // }

        

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

        
        glMatrixMode(GL_MODELVIEW);
	    glLoadIdentity();
        
        glDisable(GL_LIGHTING);
		glPushMatrix();
			set_light(light_2, diamondx, diamondz);
		glPopMatrix();
		glEnable(GL_LIGHTING);

		//diamond
		glPushMatrix ();
			glColor3f(51.0/255.0,1.0, 1.0);
			glTranslatef(diamondx, 0, diamondz);
			glBegin(GL_TRIANGLE_STRIP);
                glVertex3f(0, 2, 0);
                glVertex3f(-1, 0, 1);
                glVertex3f(1, 0, 1);
                glVertex3f(0, 0, -1.4);
                glVertex3f(0, 2, 0);
                glVertex3f(-1, 0, 1);
            glEnd();
		glPopMatrix ();

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
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
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
