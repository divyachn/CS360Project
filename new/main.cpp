#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <cmath>
#include <math.h>
#include <string.h>

#include <GL/glut.h>

#include "texture.h"

using namespace std;

#define MAZE_SIZE 13
#define GAME_START 0
#define GAME_ON 1
#define GAME_WON 2

// Game state
int gameState = 0;

// Main Window
int mainWindow;
int SizeX=800;
int SizeY=600;

// Mouse Origin
int xOrigin=SizeX/2;
int yOrigin=SizeX/2;

float cameraMoveSpeed = 0.1f;

//Booleans for tracking map states
bool mapMode = false;
bool win = false;

//Angle of rotation, direction vector of camera and position vector of camera
float angle=0.0f;
float lx=0.0f,lz=-1.0f, ly=0;
float tilt = 0;
float x=2 ,z=2, y = 0;

float origTilt;
float origLy;

//Angle for blip
float turnAngle = 0;

//Deltas for camera movement
float deltaAngle = 0.0f;
float deltaAngleY = 0.0f;
float deltaX = 0;
float deltaZ = 0;

unsigned int g_wall = 0;
unsigned int g_ground = 1;
unsigned int g_start = 2;

unsigned int g_bitmap_text_handle = 0;

//Datastructures for lights and material properties
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

//Function for handling menu events
void processMenuEvents(int option) {
	//Exit cleanly
	if (option == 2) {
		glutDestroyWindow(mainWindow);
		exit(0);
	}
}

//Menu creator function for mouse RB clicks
void createGLUTMenus() {
	int menu;

	//"processMenuEvents" will handle events
	menu = glutCreateMenu(processMenuEvents);

	//Adding entries to the menu
	glutAddMenuEntry("map view M",1);
	glutAddMenuEntry("exit game",2);

	//Attach the menu to the right button
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

//Bitmap text function
unsigned int make_bitmap_text() {
	unsigned int handle_base = glGenLists(256);
	for (size_t i=0;i<256;i++) {
		// a new list for each character
		glNewList(handle_base+i, GL_COMPILE);
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, i);
		glEndList();
	}
	return handle_base;
}

void draw_text(const char* text) {
	glListBase(g_bitmap_text_handle);
	glCallLists(int(strlen(text)), GL_UNSIGNED_BYTE, text);
}

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
        maze[i][j] = 2;
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

//Function for window resize
void reshape(int w, int h) {
	SizeX=w;
	SizeY=h;
	xOrigin = SizeX/2;
	yOrigin = SizeY/2;
	if (h == 0) //Prevent divide by 0
		h = 1;
	float ratio =  w * 1.0 / h;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, w, h);
	gluPerspective(45.0f, ratio, 0.1f, 100.0f);
}

void drawWall(float x1, float x2, float y1, float y2, float z1, float z2) {
	glBindTexture ( GL_TEXTURE_2D, g_wall);
	glBegin(GL_QUADS);
		glColor4f(1, 1, 1, 1);
		//Rightside
		glNormal3f(0, 0, 1);
		glTexCoord2f(0,0);
		glVertex3f(x1, y1, z1);
	  glTexCoord2f(1,0);
		glVertex3f(x2, y1, z1);
		glTexCoord2f(1,1);
		glVertex3f(x2, y2, z1);
		glTexCoord2f(0,1);
		glVertex3f(x1, y2, z1);

		//Leftside
		glNormal3f(0, 0, -1);
		glTexCoord2f(0,0);
		glVertex3f(x1, y1, z2);
		glTexCoord2f(1,0);
		glVertex3f(x2, y1, z2);
		glTexCoord2f(1,1);
		glVertex3f(x2, y2, z2);
		glTexCoord2f(0,1);
		glVertex3f(x1, y2, z2);

		//Top
		glNormal3f (0,1, 0);
		glTexCoord2f(0,0);
		glVertex3f(x1, y2, z1);
		glTexCoord2f(1,0);
		glVertex3f(x2, y2, z1);
		glTexCoord2f(1,1);
		glVertex3f(x2, y2, z2);
		glTexCoord2f(0,1);
		glVertex3f(x1, y2, z2);

		//Backside
		glNormal3f (1,0, 0);
		glTexCoord2f(0,0);
		glVertex3f(x1, y1, z1);
		glTexCoord2f(0,1);
		glVertex3f(x1, y2, z1);
		glTexCoord2f(1,1);
		glVertex3f(x1, y2, z2);
		glTexCoord2f(1,0);
		glVertex3f(x1, y1, z2);

		//Frontside
		glNormal3f (-1,0, 0);
		glTexCoord2f(0,0);
		glVertex3f(x2, y1, z1);
		glTexCoord2f(0,1);
		glVertex3f(x2, y2, z1);
		glTexCoord2f(1,1);
		glVertex3f(x2, y2, z2);
		glTexCoord2f(1,0);
		glVertex3f(x2, y1, z2);
	glEnd();
}

void drawFloor(GLfloat x1, GLfloat x2, GLfloat z1, GLfloat z2) {
	glBindTexture ( GL_TEXTURE_2D, g_ground);
	glBegin(GL_POLYGON);
		glNormal3f( 0.0, 1.0, 0.0);
		glColor3f(0.9f, 0.9f, 0.9f);

		glTexCoord2f(0,0);
		glVertex3f( x1, -1, z2 );
		glTexCoord2f(1,0);
		glVertex3f( x2, -1, z2 );
		glTexCoord2f(1,1);
		glVertex3f( x2, -1, z1 );
		glTexCoord2f(0,1);
		glVertex3f( x1, -1, z1 );
  glEnd();
}

//Trivial collision detection based on position of cubes in the map
//Based on what is front, if close to the wall, return true
bool checkCollision() {
	float camWorldX = (x+lx)/2;
	float camWorldZ = (z+lz)/2;

	int truncX = static_cast<int>(camWorldX);
	int truncZ = static_cast<int>(camWorldZ);

	int roundedZ = round(camWorldZ);
	int roundedX = round(camWorldX);

	cout<<"camWorld: "<<camWorldX<<","<<camWorldZ<<"\n";
	cout<<"trunc: "<<truncX<<","<<truncZ<<"\n";
	cout<<"maze_value: "<<maze[roundedX][roundedZ]<<"\n\n";
	if (maze[roundedX][roundedZ] == 0) {
		// if (camWorldX > truncX+0.5 || truncZ > 1) {
			return true;
			// }
		// else return false;
	} else
	 return false;
}

//Function to compute X and Z position
void computePos(float deltaX, float deltaZ) {
	float oldx = x;
	float oldz = z;

	//Front and back movement
	x += deltaX * lx * cameraMoveSpeed;
	z += deltaX * lz * cameraMoveSpeed;

	float rightZ = -lz;
	float rightX = lx;

	//Left and right movement
	x += deltaZ * rightZ * cameraMoveSpeed;
	z += deltaZ * rightX * cameraMoveSpeed;

	//Don't allow movement is collision is true
	if (checkCollision ()) {
		x = oldx;
		z = oldz;
	}
}

void gameProgressScreen() {
	//Check if goal is reached
	if (x > diamondx-1 && x < diamondx+1 && z > diamondz-1 && z < diamondz+1) {
		cout<<x<<":"<<z<<endl;
		gameState = GAME_WON;
	}
	//If there has been change in X and Z position, compute new position
	if (deltaX || deltaZ) {
		computePos(deltaX, deltaZ);
	}
	y = tilt+ly;

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	//Color for background
	glClearColor(0.139, 0.134, 0.130, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//Stop fps view when goal is reached
	if (gameState == GAME_WON) {
		mapMode = true;
	}

	//Incremental zoomin and zoomout
	if (mapMode) {
		if ( tilt != 75)
		tilt++;
		y = 5;
	} else if (!mapMode & tilt > origTilt) {
		tilt--;
		y = origLy;
	} else {
		tilt = 0;
		y = tilt+ly;
	}

	//Camera vector
	gluLookAt(x, tilt, z, // eye position
			  x+lx, y, z+lz, // reference point
			  0, 1, 0);  // up vector

	cout<<"Eye Position: "<<x<<" "<<tilt<<" "<<z<<endl;
	cout<<"Reference Point: "<<x+lx<<" "<<y<<" "<<z+lz<<"\n";

	//Light for triangle blip
	glDisable(GL_LIGHTING);
	glPushMatrix();
		glPointSize(5.0f);
		set_light(light_1, x+lx, z+lz);
	glPopMatrix();
	glEnable(GL_LIGHTING);

	//Drawing the blip for camera position
	glPushMatrix();
		glTranslatef(x+lx, -0.60f, z+lz);
		glRotatef(deltaAngle, 0, 1, 0);
		glScalef(0.5,1.0,0.5);
		glBegin(GL_POLYGON);
	 		glNormal3f (0,1, 0);
			glColor3f(0, 0.8, 0.8);
			glVertex3f(0.25f, 0, 0.25f);
			// glVertex3f(0, 0, 0);
			glVertex3f(-0.25f, 0, 0.25f);
			glColor3f(1, 0, 0);
			glVertex3f(0, 0, -0.75f);
		glEnd();
	glPopMatrix();

	//Changes to be made when goal is reached
	if (gameState == GAME_ON) {
		//Lighting for text
		glDisable(GL_LIGHTING);
		glPushMatrix();
			// perform any transformations of light positions here
			glPointSize(5.0f);
			set_light(light_1, -3, -4);
		glPopMatrix();
		glEnable(GL_LIGHTING);
	}
	else if(gameState == GAME_WON) {
		//Light for text
		glPushMatrix();
			glPointSize(5.0f);
			set_light(light_1, 15, 15);
		glPopMatrix();
		glEnable(GL_LIGHTING);

		//Text when goal is reached
		glPushMatrix();
			glColor3f(1, 1, 1);
			glTranslatef(16, 1, 16); // this will work
			glRasterPos2i(0, 0); // centre the text
			draw_text("Won the Game");
		glPopMatrix();
	}

	//Light for diamond
	glDisable(GL_LIGHTING);
	glPushMatrix();
		set_light(light_2, diamondx, diamondz);
	glPopMatrix();
	glEnable(GL_LIGHTING);

	//diamond
	glPushMatrix ();
		glColor3f(51.0/255.0,1.0, 1.0);
		glTranslatef(diamondx, -0.6, diamondz);
		glutSolidOctahedron();
	glPopMatrix ();

	float stary;

	for(int i = 0; i < MAZE_SIZE; i++) {
		for(int j=0; j < MAZE_SIZE; j++) {
			//Stars at varying heights
			if (i > 6 & j > 6) {
				glPointSize(3.0f);
				stary = 3;
			} else if (i < 6 & j < 6) {
				glPointSize (4.0f);
				stary = 25;
			}
			//Draw points for stars
			glPushMatrix ();
				glBegin(GL_POINTS);
					glColor3f (1, 1, 1);
					glVertex3f(50.3*i, stary, j*37.4);
				glEnd();
			glPopMatrix();

			if (maze[i][j] == 0) {
				//Draw a wall
				cout<<"Wall - (i,j): "<<i<<","<<j<<"\n";
				glPushMatrix();
					glTranslatef(2*i, 0, j*2);
					glEnable(GL_TEXTURE_2D);
					drawWall(1, -1, 1, -1, 1, -1);
					glDisable(GL_TEXTURE_2D);
				glPopMatrix();
			} else {
				//Draw a floor tile
				glPushMatrix();
					glTranslatef(2*i, 0, j*2);
					glEnable(GL_TEXTURE_2D);
					drawFloor(1, -1, 1, -1);
					glDisable(GL_TEXTURE_2D);
				glPopMatrix();

				//Add a light
				glDisable(GL_LIGHTING);
				glPushMatrix();
					// perform any transformations of light positions here
					glRotatef(0, 0, 0, 0);
					set_light(light_1, 2*i, j*2);
				glPopMatrix();
				glEnable(GL_LIGHTING);
			}
		}
	}
}

void gameBeginScreen(){
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glClearColor(0.139, 0.134, 0.130, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//Camera vector
	gluLookAt(x, tilt, z, // eye position
			  x+lx, y, z+lz, // reference point
			  0, 1, 0);  // up vector

	//Light for triangle blip
	glDisable(GL_LIGHTING);
	glPushMatrix();
		glPointSize(5.0f);
		set_light(light_1, x+lx, z+lz);
	glPopMatrix();
	glEnable(GL_LIGHTING);

	int i=1, j=0;
	GLfloat x1=1, x2=-1, y1=1, y2=-1, z=1;
	glPushMatrix();
		glTranslatef(2*i, 0, j*2);
		glEnable(GL_TEXTURE_2D);
		glBindTexture ( GL_TEXTURE_2D, g_start);
		glBegin(GL_QUADS);
			glColor4f(1, 1, 1, 1);
			glNormal3f(0, 0, 1);
			glTexCoord2f(1,1);
			glVertex3f(x1, y1, z);
		  glTexCoord2f(0,1);
			glVertex3f(x2, y1, z);
			glTexCoord2f(0,0);
			glVertex3f(x2, y2, z);
			glTexCoord2f(1,0);
			glVertex3f(x1, y2, z);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	glPopMatrix();
}

void display(){
	cout<<"gameState: "<<gameState<<"\n";
	switch(gameState){
		case GAME_START:
			gameBeginScreen();
			break;
		case GAME_ON:
			gameProgressScreen();
			break;
		default:
			break;
	}
	glFlush ();
  glutSwapBuffers();
}

//Keyboard press handler
void pressKey(unsigned char key, int xx, int yy) {
	switch (key) {
		//Change y to get a top down view and store current y and eye y
		//to revert back to it when key is released
		case 'm': mapMode = true;
			origTilt = tilt;
			origLy = ly;
			break;
		case 'c':
			cout<<"Continue the game\n";
			gameState = GAME_ON;
			glutPostRedisplay();
			break;
		case 'q' :
			glutDestroyWindow(mainWindow);
			exit(0);
			break;
		case 27:
			exit(0);
	}
}

void pressSpecialKey(int key, int xx, int yy) {
	switch (key) {
		case GLUT_KEY_UP : deltaX = 0.5f; break;
		case GLUT_KEY_DOWN : deltaX = -0.5f; break;
		case GLUT_KEY_LEFT : deltaZ = -0.5f; break;
		case GLUT_KEY_RIGHT : deltaZ = 0.5f; break;
	}
}

//Keyboard press release handler
void releaseKey(unsigned char key, int x, int y) {
	switch (key) {
		case 'm': mapMode = false;
			break;
		}
}

void releaseSpecialKey(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_UP : deltaX = 0.0f; break;
		case GLUT_KEY_DOWN : deltaX = 0.0f; break;
		case GLUT_KEY_LEFT : deltaZ = 0.0f; break;
		case GLUT_KEY_RIGHT : deltaZ = 0.0f; break;
	}
}

//Passive mouse movement callback
void mouseMove(int xx, int yy) {
	//turnAngle calculated for triangle blip rotation
	float t = turnAngle;
	turnAngle = -1 * (xx - xOrigin);
	if (turnAngle>0) {
		turnAngle = turnAngle + 10;
	} else if (turnAngle<0)
		turnAngle = turnAngle - 10;
	else
		turnAngle = 0;

	//Change in x and y values due to mouse movement
	float dAngle = deltaAngle, dAngleY = deltaAngleY;
	deltaAngle = (xx - xOrigin) * 0.01f;
	deltaAngleY = (yy - yOrigin) * 0.00f;

	//Calculating camera direction
	int lx1=lx,ly1=ly,lz1=lz;
	lx = sin(angle + deltaAngle);
	lz = -cos(angle + deltaAngle);
	ly =  -sin(deltaAngleY);
	if(checkCollision()){
		// there is collision, restore lx,ly,lz
		lx=lx1; ly=ly1; lz=lz1;
		turnAngle = t;
		deltaAngle = dAngle;
		deltaAngleY = dAngleY;
	}
	glutSetWindow(mainWindow);
}

//Loading textures
void load_and_bind_textures() {
	g_wall = load_and_bind_texture("./wall.png");
	g_ground = load_and_bind_texture("./tile.png");
	g_start = load_and_bind_texture("./start.png");
}

void init() {
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	set_material (materialM);
	mazeGen();
}

int main(int argc, char **argv) {
	// init GLUT and create window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(SizeX,SizeY);
	mainWindow = glutCreateWindow("Find the diamond");

	//Callbacks
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutIdleFunc(display);

	//Keyboard callbacks
	glutKeyboardFunc(pressKey);
	glutIgnoreKeyRepeat(1);
	glutKeyboardUpFunc(releaseKey);

	//Special Key callbacks
	glutSpecialFunc(pressSpecialKey);
	glutSpecialUpFunc(releaseSpecialKey);

	//Mouse Callbacks
	glutPassiveMotionFunc(mouseMove);
	glutWarpPointer(SizeX/2,SizeY/2);

	glEnable(GL_DEPTH_TEST);

	int max_texture_units = 0;
    	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);
    	fprintf(stderr, "Max texture units is %d\n", max_texture_units);

	load_and_bind_textures();
	init();
	g_bitmap_text_handle = make_bitmap_text();
	createGLUTMenus ();
	// enter GLUT event processing cycle
	glutMainLoop();
	return 0;
}
