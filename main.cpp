#include <GL/glew.h>
#include <GL/glut.h>

// these headers are reqired for shader programing
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// these headers are required for music files
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <iostream>
#include <vector>
#include <unistd.h>

#include "shader_utils.h"
#include "texture.hpp"
#include "Camera.h"
#include "MazeGenerator.h"

using namespace std;

using glm::vec4;
using glm::vec3;
using glm::vec2;

#define GAME_BEGIN 1
#define GAME_OVER 2
#define GAME_WON 3
#define GAME_ON 4
#define DOOR_ON 5
#define GAME_IN_PROGRESS 6

struct GlobalVar{
  int gameStatus;
  float X,Y,z_start;
  float door_width, door_height;
  float angle;
  GlobalVar(){
    gameStatus = GAME_BEGIN;
    X = 60.0;
    Y = 70.0;
    door_width = 20.0;
    door_height = 100.0;
    angle = 0.0;
    z_start = -1.0;
  }
};

GlobalVar G;

GLuint program;

GLint attribute_v_coord = -1;
GLint attribute_v_normal = -1;
GLint attribute_v_color = -1;
GLint attribute_v_texture = -1;
GLint uniform_m = -1, uniform_v = -1, uniform_p = -1;
GLint uniform_m_3x3_inv_transp = -1, uniform_v_inv = -1;

GLuint text;
GLuint TextureID = 0;

Camera camera(vec3(0.0, 0.0, 5.0), vec3(0.0, 0.0, -10.0));  // view in the negative z-direction
Maze m(100,100);

int compile_link_shaders(char* vshader_filename, char* fshader_filename){
    GLint link_ok = GL_FALSE;
    GLint validate_ok = GL_FALSE;
    GLuint vs, fs;

    if ((vs = create_shader(vshader_filename, GL_VERTEX_SHADER))   == 0) { return 0; }
    if ((fs = create_shader(fshader_filename, GL_FRAGMENT_SHADER)) == 0) { return 0; }

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);

    if (!link_ok) {
        fprintf(stderr, "glLinkProgram:");
        print_log(program);
        return 0;
    }

    glValidateProgram(program);
    glGetProgramiv(program, GL_VALIDATE_STATUS, &validate_ok);

    if (!validate_ok) {
        fprintf(stderr, "glValidateProgram:");
        print_log(program);
    }

    const char* attribute_name;

    attribute_name = "v_coord";
    attribute_v_coord = glGetAttribLocation(program, attribute_name);
    if (attribute_v_coord == -1) {
        fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
        return 0;
    }

    attribute_name = "v_normal";
    attribute_v_normal = glGetAttribLocation(program, attribute_name);
    if (attribute_v_normal == -1) {
        fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
        return 0;
    }

    attribute_name = "v_color";
    attribute_v_color = glGetAttribLocation(program, attribute_name);
    if (attribute_v_color == -1) {
        fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
        return 0;
    }

    attribute_name = "v_texture";
    attribute_v_texture = glGetAttribLocation(program, attribute_name);
    if (attribute_v_texture == -1) {
        fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
        return 0;
    }

    const char* uniform_name;

    uniform_name = "m";
    uniform_m = glGetUniformLocation(program, uniform_name);
    if (uniform_m == -1) {
        fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
        return 0;
    }

    uniform_name = "v";
    uniform_v = glGetUniformLocation(program, uniform_name);
    if (uniform_v == -1) {
        fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
        return 0;
    }

    uniform_name = "p";
    uniform_p = glGetUniformLocation(program, uniform_name);
    if (uniform_p == -1) {
        fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
        return 0;
    }

    uniform_name = "m_3x3_inv_transp";
    uniform_m_3x3_inv_transp = glGetUniformLocation(program, uniform_name);
    if (uniform_m_3x3_inv_transp == -1) {
        fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
        return 0;
    }

    uniform_name = "v_inv";
    uniform_v_inv = glGetUniformLocation(program, uniform_name);
    if (uniform_v_inv == -1) {
        fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
        return 0;
    }

    return 1;
}

void free_resources() { glDeleteProgram(program); }

void writeText(vec3 color, vec3 position, string s){
  glColor3f(color.r,color.g,color.b);
  glRasterPos3f(position.x,position.y,position.z);
  for (int i=0; i<s.size(); i++){
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, s[i]);
	}
}

void texturePolygon(vector <vec3> vertices, vector <vec2> t_coord, int n){
  // for a polygon specify the texture map to be mapped
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, text);

  glPushMatrix();
  glBegin(GL_POLYGON);
    for(int i=0;i<n;i++){
      glTexCoord2f(t_coord[i].x, t_coord[i].y);
      glVertex3f(vertices[i].x, vertices[i].y, vertices[i].z);
    }
  glEnd();
  glPopMatrix();

  glDisable(GL_TEXTURE_2D);
}

vector <vec3> const_z(float x_max, float x_min, float y_max, float y_min, float z){
  vector <vec3> face = {vec3(x_min,y_min,z), vec3(x_max,y_min,z), vec3(x_max,y_max,z), vec3(x_min,y_max,z)};
  return face;
}

void gameBeginScreen(){
  vector <vec2> t = {vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0)};
  float z = -20.0;
  vector <vec3> face = const_z(100.0, -100.0, 80.0, -80.0, z);
  text = loadBMP_custom((char *) "./images/maze1.bmp");
  texturePolygon(face, t, 4);
  writeText(vec3(1.0, 1.0, 1.0), vec3(-40.0, 60.0, z+0.01), "CS360 - PROJECT - A 3D maze game");
  writeText(vec3(1.0, 1.0, 1.0), vec3(-25.0, 40.0, z+0.01), "Find the DIAMOND");
  writeText(vec3(1.0, 1.0, 1.0), vec3(-20.0, 20.0, z+0.01), "Click to play!");
  writeText(vec3(1.0, 1.0, 1.0), vec3(40.0, -60.0, z+0.01), "Divya Chauhan 160246");
  writeText(vec3(1.0, 1.0, 1.0), vec3(40.0, -70.0, z+0.01), "Rahul BS 160xxx");
}

void gameOverScreen(){
  vector <vec2> t = {vec2(0.1, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.1, 1.0)};
  float z = -20.0;
  vector <vec3> face = const_z(100.0, -100.0, 80.0, -80.0, z);
  text = loadBMP_custom((char *) "./images/gameOver.bmp");
  texturePolygon(face, t, 4);
  writeText(vec3(0.0, 0.0, 0.0), vec3(-70.0, -70.0, z+0.01), "Press q to quit and r to replay.");
}

void gameWonScreen(){
  vector <vec2> t = {vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0)};
  float z = -20.0;
  vector <vec3> face = const_z(70.0, 20.0, 50.0, -50.0, z);
  text = loadBMP_custom((char *) "./images/gameWon.bmp");
  texturePolygon(face, t, 4);
  face = const_z(-10.0, -90.0, 50.0, -50.0, z);
  text = loadBMP_custom((char *) "./images/diamond.bmp");
  texturePolygon(face, t, 4);
  writeText(vec3(0.0, 0.0, 0.0), vec3(-70.0, -70.0, z+0.01), "Press q to quit and r to replay.");
}

// Add music while game is in progress
int initMusic() {
  int NUM_BUFFERS = 1;
  int NUM_SOURCES = 1;
	alGetError();

	ALuint buffers[NUM_BUFFERS];
	int error;
	// Create the buffers
	alGenBuffers(1, buffers);
	if ((error = alGetError()) != AL_NO_ERROR) {
	  printf("ERROR::0: alGenBuffers : %d", error);
	  return 0;
	}

	ALenum     format;
	ALsizei    size;
	ALsizei    freq;
	ALboolean  loop;
	ALvoid*    data;

	char *path = (char *)"./sounds/test.wav";
	buffers[0] = alutCreateBufferFromFile(path);
	if ((error = alGetError()) != AL_NO_ERROR) {
	  printf("ERROR::1: alBufferData buffer 0 : %d\n", error);
	  alDeleteBuffers(NUM_BUFFERS, buffers);
	  return 0;
	}

	if((error = alutGetError())) { cout<<"File error:"<<alutGetErrorString(error)<<endl; }

	ALuint source[NUM_SOURCES];
	// Generate the sources
	alGenSources(NUM_SOURCES, source);
	if ((error = alGetError()) != AL_NO_ERROR) {
	  printf("ERROR::2: alGenSources : %d", error);
	  return 0;
	}
	alSourcei(source[0], AL_LOOPING, AL_TRUE);
	alSourcei(source[0], AL_BUFFER, buffers[0]);
	if ((error = alGetError()) != AL_NO_ERROR) {
	  printf("ERROR::3: alSourcei : %d", error);
	  return 0;
	}

	ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
  alSource3f (source[0], AL_POSITION, 0,0,0);
  alSource3f (source[0], AL_VELOCITY, 0,0,0);
  alSource3f (source[0], AL_DIRECTION, 0,0,1);

	// alListener3f(AL_POSITION,0,0,1.0);
	alListener3f(AL_VELOCITY,0,0,0);
	alListenerfv(AL_ORIENTATION,listenerOri);
  printf("Play the musikkk!\n");
	alSourcePlay(source[0]);
	return 1;
}

void renderDoorWall(){
  vector <vec2> t = {vec2(0.0, 0.0), vec2(5.0, 0.0), vec2(5.0, 5.0), vec2(0.0, 5.0)};
  text = loadBMP_custom((char *) "./images/brick_wall.bmp"); // surrounding of the entrance door
  vector <vec3> face = const_z(-G.door_width, -G.X, G.door_height, -G.door_height/2, G.z_start);
  texturePolygon(face, t, 4);
  face = const_z(G.X, G.door_width, G.door_height, -G.door_height/2, G.z_start);
  texturePolygon(face, t, 4);
  t[0] = vec2(0.15, 0.0); t[1] = vec2(1.15, 0.0); t[2] = vec2(1.15, 1.0); t[3] = vec2(0.15, 1.0);
  face = const_z(G.door_width, -G.door_width, G.Y, G.door_height/2, G.z_start);
  texturePolygon(face, t, 4);
}

void renderDoor(){
  // the door itself, it has two halves : left and right half
  text = loadBMP_custom((char *) "./images/door_left.bmp");
  vector <vec2> t(4);
  t[0] = vec2(0.15, 0.0); t[1] = vec2(1.15, 0.0); t[2] = vec2(1.15, 1.0); t[3] = vec2(0.15, 1.0);
  vector <vec3> face = const_z(0.0, -G.door_width, G.door_height/2, -G.door_height/2, G.z_start);

  glPushMatrix();
  glTranslatef(-G.door_width, -G.door_height/2, G.z_start);
  glRotatef(G.angle, 0.0, 1.0, 0.0);  // Rotate about Y-axis anti-clockwise
  glTranslatef(G.door_width, G.door_height/2, -G.z_start);
  texturePolygon(face, t, 4);
  glPopMatrix();

  text = loadBMP_custom((char *) "./images/door_right.bmp");
  face = const_z(G.door_width, 0.0, G.door_height/2, -G.door_height/2, G.z_start);

  glPushMatrix();
  glTranslatef(G.door_width, -G.door_height/2, G.z_start);
  glRotatef(-G.angle, 0.0, 1.0, 0.0);  // Rotate about Y-axis anti-clockwise
  glTranslatef(-G.door_width, G.door_height/2, -G.z_start);
  texturePolygon(face, t, 4);
  glPopMatrix();
}

void gameOnScreen(){
  // argument z specifies the z-plane where to model the entrance door
  glPushMatrix();
  glTranslatef(0.0,-10.0,0.0);
  renderDoorWall();
  renderDoor();
  writeText(vec3(0.0,0.0,0.0), vec3(-G.door_width/2, G.door_height/2+10.0, G.z_start+0.1), "Press 'o' to open the door");
  glPopMatrix();
}

void openDoor(){
  if(G.angle==90.0){
    // door has been opened
    G.gameStatus = GAME_IN_PROGRESS;
  } else {
    glTranslatef(0.0,-10.0,0.0);
    renderDoorWall();
    renderDoor();
    glTranslatef(0.0,10.0,0.0);
    G.angle+=5.0;
  }
  glutPostRedisplay();
}

void gameProgressScreen(){
  struct Grid maze = m.generateMaze();
  cout<<"Game has started\n";
  glPushMatrix();
  glTranslatef(0.0,-10.0,0.0);
  renderDoorWall();
  renderDoor();
  glPopMatrix();
}

void clickStart(int button, int state, int x, int y){
  if(G.gameStatus==GAME_BEGIN){
    // click is functinal only if welcome screen is to redirected to game screen
    G.gameStatus = GAME_ON;
    glutPostRedisplay();
  }
}

void normalKeys( unsigned char key, int x, int y ) {
  if(key==27 || key=='q') exit(0);  //escape key
  if(key=='o'){
    // open the door
    G.gameStatus = DOOR_ON;
  }
  glutPostRedisplay();
}

void display(void) {
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    cout<<"gameStatus: "<<G.gameStatus<<"\n";

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    vec4 lrbt = camera.getlrbt();
    vec2 zplanes = camera.getZplanes();
    glFrustum(lrbt.x, lrbt.y,  lrbt.z, lrbt.w, zplanes.x, zplanes.y);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    vec3 camera_coordinates = camera.getCameraCoordinates();
    vec3 look_at = camera.getLookAtVector();
    gluLookAt(camera_coordinates.x, camera_coordinates.y, camera_coordinates.z,
              look_at.x, look_at.y, look_at.z,
              0.0, 1.0, 0.0);

    // cout<<"camera_coordinates: ("<<camera_coordinates.x<<" , "<<camera_coordinates.y<<" , "<<camera_coordinates.z<<")\n";
    // cout<<"look_at: ("<<look_at.x<<" , "<<look_at.y<<" , "<<look_at.z<<")\n";
    // cout<<"left: "<<lrbt.x<<" right: "<<lrbt.y<<"\n";
    // cout<<"bottom: "<<lrbt.z<<" top: "<<lrbt.w<<"\n";
    // cout<<"near: "<<zplanes.x<<" far: "<<zplanes.y<<"\n";

    switch(G.gameStatus){
      case GAME_BEGIN:
        gameBeginScreen();
        break;
      case GAME_OVER:
        gameOverScreen();
        break;
      case GAME_WON:
        gameWonScreen();
        break;
      case GAME_ON:
        gameOnScreen();
        break;
      case DOOR_ON:
        openDoor();
        break;
      case GAME_IN_PROGRESS:
        gameProgressScreen();
        break;
      default:
        cout<<"What a strange value of gameStatus: "<<G.gameStatus<<"\n";
        break;
    }

    glFlush ();
    glutSwapBuffers();
}

void init (void) {
  glClearColor(1.0, 1.0, 1.0, 1.0); // set the background color - white
  glEnable (GL_DEPTH_TEST); //enable the depth testing
  // enableLighting();
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
}

void reshape (int w, int h) {
   glViewport(0, 0, (GLsizei) w, (GLsizei) h);
  //  glMatrixMode(GL_PROJECTION);
  //  glLoadIdentity();
  //  vec4 lrbt = camera.getlrbt();
  //  vec2 zplanes = camera.getZplanes();
  //  cout<<"left: "<<lrbt.x<<" right: "<<lrbt.y<<"\n";
  //  cout<<"bottom: "<<lrbt.z<<" top: "<<lrbt.w<<"\n";
  //  cout<<"near: "<<zplanes.x<<" far: "<<zplanes.y<<"\n";
  //  glFrustum(lrbt.x, lrbt.y,  lrbt.z, lrbt.w, zplanes.x, zplanes.y);
  //  glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
  glutInit (&argc, argv);
  glutInitDisplayMode (GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize (1200,600);
  glutInitWindowPosition (100,50);
  glutCreateWindow ("Find the Diamond");

  GLenum glew_status = glewInit();
  if (glew_status != GLEW_OK) {
      fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
      return 1;
  }

  // specify the vertex shader and fragment shader, files are hard-coded
  char* v_shader_filename = (char*) "./vertex_shader.v.glsl";
  char* f_shader_filename = (char*) "./phong_shading.f.glsl";

  int response = compile_link_shaders(v_shader_filename, f_shader_filename);
  if(response==0) return 0;

  init ();
  glutDisplayFunc (display);
  glutReshapeFunc (reshape);
  glutMouseFunc(clickStart);
  glutKeyboardFunc(normalKeys);
  glutMainLoop ();
  free_resources();
  return 0;
}
