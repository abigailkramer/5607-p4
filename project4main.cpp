

const char* INSTRUCTIONS = 
"***************\n"
"The goal of this game is to find the big yellow key in the maze.\n"
"\n"
"w/a/s/d - Moves the character.\n"
"mouse - Looks left and right.\n"
"***************\n"
;

//Mac OS build: g++ multiObjectTest.cpp -x c glad/glad.c -g -F/Library/Frameworks -framework SDL2 -framework OpenGL -o MultiObjTest
//Linux build:  g++ multiObjectTest.cpp -x c glad/glad.c -g -lSDL2 -lSDL2main -lGL -ldl -I/usr/include/SDL2/ -o MultiObjTest

#include "glad/glad.h"  //Include order can matter here
#if defined(__APPLE__) || defined(__linux__)
 #include <SDL2/SDL.h>
 #include <SDL2/SDL_opengl.h>
#else
 #include <SDL.h>
 #include <SDL_image.h>
 #include <SDL_opengl.h>
#endif
#include <cstdio>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

int screenWidth = 800; 
int screenHeight = 600;  
float timePast = 0;
int mapWidth = 0;
int mapHeight = 0;

float colR=1, colG=1, colB=1;

glm::vec3 cameraPos = glm::vec3(3.f, 0.f, 0.f);
glm::vec3 cameraFront = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);

float yaw = 0, pitch = 0;
float lastX = screenWidth / 2, lastY = screenHeight / 2;

bool DEBUG_ON = true;
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);
bool fullscreen = false;
void Win2PPM(int width, int height);

float rand01(){
	return rand()/(float)RAND_MAX;
}

struct modelInfo {
	int start;
	int numVerts;
};

struct player {
	float x;
	float y;
};

player self = { 0, 0 };
player goal = { 0, 0 };

struct door {
	int x;
	int y;
	int tex;
	bool unlocked;
	char key;
};

struct key {
	int x;
	int y;
	int tex;
	bool picked_up;
	char door;
};

struct wall {
	float x;
	float y;
};

vector<wall> walls;
vector<door> doors;
vector<key> keys;

void drawGeometry(int shaderProgram, modelInfo models[]);
vector<float> loadObjFile(string fileName);
bool canMove(glm::vec3 dirVector);

bool isFinished = false;

int main(int argc, char *argv[]){
	SDL_Init(SDL_INIT_VIDEO);  //Initialize Graphics (for OpenGL)

	//Ask SDL to get a recent version of OpenGL (3.2 or greater)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	//Create a window (offsetx, offsety, width, height, flags)
	SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 100, 100, screenWidth, screenHeight, SDL_WINDOW_OPENGL);

	//Create a context to draw in
	SDL_GLContext context = SDL_GL_CreateContext(window);
	
	//Load OpenGL extentions with GLAD
	if (gladLoadGLLoader(SDL_GL_GetProcAddress)){
		printf("\nOpenGL loaded\n");
		printf("Vendor:   %s\n", glGetString(GL_VENDOR));
		printf("Renderer: %s\n", glGetString(GL_RENDERER));
		printf("Version:  %s\n\n", glGetString(GL_VERSION));
	}
	else {
		printf("ERROR: Failed to initialize OpenGL context.\n");
		return -1;
	}
	

	//Load map file
	ifstream modelFile;
	modelFile.open("models/map.txt");
	int width = 0, height = 0;
	modelFile >> width >> height;
	const int max = width * height;
	mapWidth = width;
	mapHeight = height;
	
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			char cur;
			modelFile >> cur;
			if (cur == 'W') {
				wall newWall = { i, j };
				walls.push_back(newWall);
			}
			if (cur == 'A' || cur == 'B' || cur == 'C' || cur == 'D' || cur == 'E') {
				int temp = 0;
				if (cur == 'A') temp = 2;
				if (cur == 'B') temp = 3;
				if (cur == 'C') temp = 4;
				if (cur == 'D') temp = 5;
				if (cur == 'E') temp = 6;
				//doors[doorNum] = { i, j, temp, false, static_cast<char>(tolower(cur)) };
				door newDoor = { i, j, temp, false, static_cast<char>(tolower(cur)) };
				doors.push_back(newDoor);
			}
			if (cur == 'a' || cur == 'b' || cur == 'c' || cur == 'd' || cur == 'e') {
				int temp = 0;
				if (cur == 'a') temp = 2;
				if (cur == 'b') temp = 3;
				if (cur == 'c') temp = 4;
				if (cur == 'd') temp = 5;
				if (cur == 'e') temp = 6;
				//keys[keyNum] = { i, j, temp, false, cur };
				key newKey = { i, j, temp, false, cur };
				keys.push_back(newKey);
			}
			if (cur == 'S') {
				self.x = i;
				self.y = j;
			}
			if (cur == 'G') {
				goal.x = i;
				goal.y = j;
			}
		}
	}
	modelFile.close();

	// add outer wall objects

	for (int i = 0; i < mapWidth; i++) {
		wall wall_one = { -1.f, i };
		walls.push_back(wall_one);

		wall wall_two = { mapHeight, i };
		walls.push_back(wall_two);
	}

	for (int i = 0; i < mapHeight; i++) {
		wall wall_one = { i, -1.f };
		walls.push_back(wall_one);

		wall wall_two = { i, mapWidth };
		walls.push_back(wall_two);
	}

	int numLines = 0;

	// Load Door object
	vector<float> model1_info = loadObjFile("models/door/swapped.obj");
	numLines = model1_info.size();
	float* model1 = new float[numLines];
	
	for (int i = 0; i < numLines; i++) {
		model1[i] = model1_info[i];
	}
	int numVerts1 = numLines/8;

	// Load Key object
	vector<float> model2_info = loadObjFile("models/keys/key.obj");
	numLines = model2_info.size();
	float* model2 = new float[numLines];

	for (int i = 0; i < numLines; i++) {
		model2[i] = model2_info[i];
	}

	int numVerts2 = numLines/8;

	//Load Cube
	modelFile.open("models/cube.txt");
	numLines = 0;
	modelFile >> numLines;
	float* model3 = new float[numLines];
	for (int i = 0; i < numLines; i++) {
		modelFile >> model3[i];
	}
	printf("%d\n",numLines);
	int numVerts3 = numLines/8;
	modelFile.close();


	//Concatenate model arrays
	float* modelData = new float[(numVerts1 + numVerts2 + numVerts3)*8];
	copy(model1, model1+ numVerts1 *8, modelData);
	copy(model2, model2+ numVerts2 *8, modelData+ numVerts1 *8);
	copy(model3, model3+ numVerts3 *8, modelData+(numVerts1 + numVerts2)*8);
	int totalNumVerts = numVerts1 + numVerts2 + numVerts3;
	int startVert1 = 0;
	int startVert2 = numVerts1;
	int startVert3 = numVerts1 + numVerts2;

	modelInfo models[3];
	
	models[0] = { startVert1, numVerts1 };
	models[1] = { startVert2, numVerts2 };
	models[2] = { startVert3, numVerts3 };
	
	//// Allocate Texture 0 (walls) ///////
	SDL_Surface* surface = SDL_LoadBMP("models/textures/light-brick.bmp");
	if (surface==NULL){ //If it failed, print the error
        printf("Error: \"%s\"\n",SDL_GetError()); return 1;
    }
    GLuint tex0;
    glGenTextures(1, &tex0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w,surface->h, 0, GL_BGR,GL_UNSIGNED_BYTE,surface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    SDL_FreeSurface(surface);
    //// End Allocate Texture ///////


	//// Allocate Texture 1 (floor) ///////
	SDL_Surface* surface1 = SDL_LoadBMP("models/textures/dark-wood.bmp");
	//SDL_Surface* surface1 = SDL_LoadFile("")
	if (surface1==NULL){ //If it failed, print the error
        printf("Error: \"%s\"\n",SDL_GetError()); return 1;
    }
    GLuint tex1;
    glGenTextures(1, &tex1);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface1->w,surface1->h, 0, GL_BGR,GL_UNSIGNED_BYTE,surface1->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    SDL_FreeSurface(surface1);

	//// End Allocate Texture ///////


	//// Allocate Texture 2 ///////
	SDL_Surface* surface2 = SDL_LoadBMP("models/textures/fuschia.bmp");
	if (surface2 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex2;
	glGenTextures(1, &tex2);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, tex2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface2->w, surface2->h, 0, GL_BGR, GL_UNSIGNED_BYTE, surface2->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	SDL_FreeSurface(surface2);
	//// End Allocate Texture ///////


	//// Allocate Texture 3 ///////
	SDL_Surface* surface3 = SDL_LoadBMP("models/textures/orange.bmp");
	if (surface3 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex3;
	glGenTextures(1, &tex3);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, tex3);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface3->w, surface3->h, 0, GL_BGR, GL_UNSIGNED_BYTE, surface3->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	SDL_FreeSurface(surface3);
	//// End Allocate Texture ///////

	//// Allocate Texture 4 ///////
	SDL_Surface* surface4 = SDL_LoadBMP("models/textures/dark-green.bmp");
	if (surface2 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex4;
	glGenTextures(1, &tex4);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, tex4);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface4->w, surface4->h, 0, GL_BGR, GL_UNSIGNED_BYTE, surface4->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	SDL_FreeSurface(surface4);
	//// End Allocate Texture ///////

	//// Allocate Texture 5 ///////
	SDL_Surface* surface5 = SDL_LoadBMP("models/textures/blue.bmp");
	if (surface5 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex5;
	glGenTextures(1, &tex5);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, tex5);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface5->w, surface5->h, 0, GL_BGR, GL_UNSIGNED_BYTE, surface5->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	SDL_FreeSurface(surface5);
	//// End Allocate Texture ///////

	//// Allocate Texture 6 ///////
	SDL_Surface* surface6 = SDL_LoadBMP("models/textures/purple.bmp");
	if (surface6 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex6;
	glGenTextures(1, &tex6);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, tex6);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface6->w, surface6->h, 0, GL_BGR, GL_UNSIGNED_BYTE, surface6->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	SDL_FreeSurface(surface6);
	//// End Allocate Texture ///////

	//// Allocate Texture 6 ///////
	SDL_Surface* surface7 = SDL_LoadBMP("models/textures/birch_wood.bmp");
	if (surface7 == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex7;
	glGenTextures(1, &tex7);

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, tex7);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface7->w, surface7->h, 0, GL_BGR, GL_UNSIGNED_BYTE, surface7->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	SDL_FreeSurface(surface7);
	//// End Allocate Texture ///////

	
	//Build a Vertex Array Object (VAO) to store mapping of shader attributse to VBO
	GLuint vao;
	glGenVertexArrays(1, &vao); //Create a VAO
	glBindVertexArray(vao); //Bind the above created VAO to the current context

	//Allocate memory on the graphics card to store geometry (vertex buffer object)
	GLuint vbo[1];
	glGenBuffers(1, vbo);  //Create 1 buffer called vbo
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); //Set the vbo as the active array buffer (Only one buffer can be active at a time)
	glBufferData(GL_ARRAY_BUFFER, totalNumVerts*8*sizeof(float), modelData, GL_STATIC_DRAW); //upload vertices to vbo
	//GL_STATIC_DRAW means we won't change the geometry, GL_DYNAMIC_DRAW = geometry changes infrequently
	//GL_STREAM_DRAW = geom. changes frequently.  This effects which types of GPU memory is used
	
	int texturedShader = InitShader("textured-Vertex.glsl", "textured-Fragment.glsl");	
	
	//Tell OpenGL how to set fragment shader input 
	GLint posAttrib = glGetAttribLocation(texturedShader, "position");
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), 0);
	  //Attribute, vals/attrib., type, isNormalized, stride, offset
	glEnableVertexAttribArray(posAttrib);
	
	//GLint colAttrib = glGetAttribLocation(phongShader, "inColor");
	//glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
	//glEnableVertexAttribArray(colAttrib);
	
	GLint normAttrib = glGetAttribLocation(texturedShader, "inNormal");
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(5*sizeof(float)));
	glEnableVertexAttribArray(normAttrib);
	
	GLint texAttrib = glGetAttribLocation(texturedShader, "inTexcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));

	GLint uniView = glGetUniformLocation(texturedShader, "view");
	GLint uniProj = glGetUniformLocation(texturedShader, "proj");

	glBindVertexArray(0); //Unbind the VAO in case we want to create a new one	

	cameraPos = glm::vec3(self.x,self.y,0.5);
	
	glEnable(GL_DEPTH_TEST);  
	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_WarpMouseInWindow(window, screenWidth / 2, screenHeight / 2);

	printf("%s\n",INSTRUCTIONS);
	

	SDL_Event windowEvent;
	float lastX = 0;
	float lastY = 0;
	float currX = screenWidth / 2;
	float currY = screenHeight / 2;
	
	bool quit = false;
	while (!quit){
		while (SDL_PollEvent(&windowEvent)){  //inspect all events in the queue
			if (isFinished) quit = true;
			if (windowEvent.type == SDL_QUIT) quit = true;
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE) 
				quit = true; //Exit event loop
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_q)
				quit = true; //Exit event loop


			if (windowEvent.type == SDL_MOUSEMOTION) {
				//float xpos = windowEvent.motion.x;
				//float ypos = windowEvent.motion.y;

				float xOffset = windowEvent.motion.xrel;// xpos - lastX;
				float yOffset = windowEvent.motion.yrel;// lastY - ypos;

				xOffset *= 0.07f;
				yOffset *= 0.07f;

				yaw -= xOffset;
				pitch -= yOffset;

				//if (pitch > 179.0f) pitch = 179.0f;
				//if (pitch < -179.0f) pitch = -179.0f;

				//if (yaw > 179.0f) yaw = 179.0f;
				//if (yaw < -179.0f) yaw = -179.0f;

				glm::vec3 direction;
				direction.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
				direction.y = sin(glm::radians(yaw));
				direction.z = 0;// sin(glm::radians(pitch))* cos(glm::radians(yaw));
				cameraFront = glm::normalize(direction);

				//pitch += 0.06f * windowEvent.motion.xrel;
				//yaw -= 0.06f * windowEvent.motion.yrel;

				SDL_WarpMouseInWindow(window, screenWidth / 2, screenHeight / 2);
			}

			
			// check for collisions + avoid changing cameraPos.z
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_w) {
				glm::vec3 temp = (cameraPos + 0.1f * cameraFront);
				if (canMove(temp)) {
					cameraPos += 0.1f * cameraFront;
				}
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_s) {
				glm::vec3 temp = (cameraPos - 0.1f * cameraFront);
				if (canMove(temp)) {
					cameraPos -= 0.1f * cameraFront;
				}
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_a) {
				glm::vec3 temp = cameraPos - (glm::normalize(glm::cross(cameraFront, cameraUp)) * 0.1f);
				if (canMove(temp)) {
					cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * 0.1f;
				}
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_d) {
				glm::vec3 temp = cameraPos + glm::normalize(glm::cross(cameraFront, cameraUp)) * 0.1f;
				if (canMove(temp)) {
					cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * 0.1f;
				}
				
			}

		}

		self.x = cameraPos.x;
		self.y = cameraPos.y;
        
		// Clear the screen to default color
		glClearColor(.2f, 0.4f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(texturedShader);


		timePast = SDL_GetTicks()/1000.f;
		

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex0);
		glUniform1i(glGetUniformLocation(texturedShader, "tex0"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex1);
		glUniform1i(glGetUniformLocation(texturedShader, "tex1"), 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, tex2);
		glUniform1i(glGetUniformLocation(texturedShader, "tex2"), 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, tex3);
		glUniform1i(glGetUniformLocation(texturedShader, "tex3"), 3);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, tex4);
		glUniform1i(glGetUniformLocation(texturedShader, "tex4"), 4);

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, tex5);
		glUniform1i(glGetUniformLocation(texturedShader, "tex5"), 5);

		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, tex6);
		glUniform1i(glGetUniformLocation(texturedShader, "tex6"), 6);

		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, tex7);
		glUniform1i(glGetUniformLocation(texturedShader, "tex7"), 7);

		glBindVertexArray(vao);
		drawGeometry(texturedShader, models);
		
		GLint uniProj = glGetUniformLocation(texturedShader, "proj");
		glm::mat4 proj = glm::perspective(3.14f / 4, screenWidth / (float)screenHeight, 0.01f, 10.0f); //FOV, aspect, near, far
		glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
		
		
		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));	

		SDL_GL_SwapWindow(window); //Double buffering
		
	}
	
	//Clean Up
	glDeleteProgram(texturedShader);
    glDeleteBuffers(1, vbo);
    glDeleteVertexArrays(1, &vao);

	SDL_GL_DeleteContext(context);
	SDL_Quit();
	return 0;
}

bool canMove(glm::vec3 dirVector) {

	for (auto& d : doors) {
		if (!d.unlocked) {
			float minX = d.x - 0.5;
			float maxX = d.x + 0.5;
			float minY = d.y - 0.5;
			float maxY = d.y + 0.5;

			bool intersects = (dirVector.x >= minX && dirVector.x <= maxX && dirVector.y >= minY && dirVector.y <= maxY);
			if (intersects) {
				return false;
			}
		}
	}

	for (auto& w : walls) {
		float minX = w.x - 0.5;
		float maxX = w.x + 0.5;
		float minY = w.y - 0.5;
		float maxY = w.y + 0.5;

		// if that's true, return false
		bool intersects = (dirVector.x >= minX && dirVector.x <= maxX && dirVector.y >= minY && dirVector.y <= maxY);
		if (intersects) {
			return false;
		}
	}
	

	return true;
}


void drawGeometry(int shaderProgram, modelInfo models[]) {

	GLint uniColor = glGetUniformLocation(shaderProgram, "inColor");
	glm::vec3 colVec(0.7, 0.7, 0.1);
	glUniform3fv(uniColor, 1, glm::value_ptr(colVec));


	GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
	glm::mat4 model = glm::mat4(1);
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader
	GLint uniScale = glGetUniformLocation(shaderProgram, "textureScale");
	glm::vec2 scale = glm::vec2(1);


	//**************************
	// 	   in-map objects
	//**************************

	// goal object - big yellow key
	model = glm::mat4(1);
	model = glm::translate(model, glm::vec3(goal.x, goal.y, 1));
	model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));
	model = glm::rotate(model, glm::radians(90.f), glm::vec3(0, 1, 0));
	scale = glm::vec2(1, 1);
	glUniform2fv(uniScale, 1, glm::value_ptr(scale));
	glUniform1i(uniTexID, -1);
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
	glDrawArrays(GL_TRIANGLES, models[1].start, models[1].numVerts);
	if (abs(self.x - goal.x) < 1.f && abs(self.y - goal.y) < 1.f) {
		isFinished = true;
	}

	for (auto& d : doors) {
		if (!d.unlocked) {
			model = glm::mat4(1);
			model = glm::translate(model, glm::vec3(d.x, d.y, -0.5));
			scale = glm::vec2(1, 1);
			glUniform2fv(uniScale, 1, glm::value_ptr(scale));
			glUniform1i(uniTexID, d.tex);
			glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
			glDrawArrays(GL_TRIANGLES, models[0].start, models[0].numVerts);

			if (abs(self.x - d.x) < 1.f && abs(self.y - d.y) < 1.f) {
				for (auto& k : keys) {
					if (k.picked_up && k.door == d.key) {
						d.unlocked = true;
					}
				}
			}
		}
		else {
			continue;
		}
	}


	for (auto& k : keys) {
		if (!k.picked_up) {
			model = glm::mat4(1);
			model = glm::translate(model, glm::vec3(k.x, k.y, 0.3));
			model = glm::scale(model, glm::vec3(0.3, 0.3, 0.3));
			model = glm::rotate(model, glm::radians(90.f), glm::vec3(0, 1, 0));

			scale = glm::vec2(1, 1);
			glUniform2fv(uniScale, 1, glm::value_ptr(scale));
			glUniform1i(uniTexID, k.tex);
			glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
			glDrawArrays(GL_TRIANGLES, models[1].start, models[1].numVerts);

			// check if close enough
			if (abs(self.x - k.x) < 1.f && abs(self.y - k.y) < 1.f) {
				k.picked_up = true;
			}
		}
		else {
			continue;
		}
	}


	for (auto& w : walls) {
		model = glm::mat4(1);
		model = glm::translate(model, glm::vec3(w.x, w.y, 0.5));
		model = glm::scale(model, glm::vec3(1, 1, 2));
		scale = glm::vec2(2, 4);
		glUniform2fv(uniScale, 1, glm::value_ptr(scale));
		glUniform1i(uniTexID, 0);
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, models[2].start, models[2].numVerts);
	}

	//**************************
	// 	   floor & ceiling
	//**************************

	//floor
	model = glm::mat4(1);
	model = glm::translate(model, glm::vec3(mapWidth / 2, mapHeight / 2, -0.75));
	model = glm::scale(model, glm::vec3(mapWidth + 1, mapHeight + 1, 0.5));
	scale = glm::vec2(mapWidth*2, mapHeight*2);
	glUniform2fv(uniScale, 1, glm::value_ptr(scale));
	glUniform1i(uniTexID, 1);
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
	glDrawArrays(GL_TRIANGLES, models[2].start, models[2].numVerts);

	//ceiling
	model = glm::mat4(1);
	model = glm::translate(model, glm::vec3(mapWidth / 2, mapHeight / 2, 1.75));
	model = glm::scale(model, glm::vec3(mapWidth + 1, mapHeight + 1, 0.5));
	scale = glm::vec2(mapWidth*2, mapHeight*2);
	glUniform2fv(uniScale, 1, glm::value_ptr(scale));
	glUniform1i(uniTexID, 7);
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
	glDrawArrays(GL_TRIANGLES, models[2].start, models[2].numVerts);

}

vector<float> loadObjFile(string fileName) {
	ifstream objFile;

	int numLines = 0;
	vector< glm::vec3 > v;
	vector< glm::vec2 > vt;
	vector< glm::vec3 > vn;
	vector< float > info;

	objFile.open(fileName);

	//string type;
	string line;
	while (getline(objFile, line)) {
		istringstream ss(line);
		string type;
		ss >> type;

		if (type[0] == '#') {
			//cout << "skipping comment line" << endl;
			continue;
		}
		else if (type == "v") {
			string string_x, string_y, string_z;
			ss >> string_x >> string_y >> string_z;
			float x, y, z;
			x = atof(string_x.c_str());
			y = atof(string_y.c_str());
			z = atof(string_z.c_str());
			v.push_back(glm::vec3(x, y, z));
		}
		else if (type == "vt") {
			string string_u, string_v;
			ss >> string_u >> string_v;
			float u, v;
			u = atof(string_u.c_str());
			v = atof(string_v.c_str());
			vt.push_back(glm::vec2(u, v));
		}
		else if (type == "vn") {
			string string_x, string_y, string_z;
			ss >> string_x >> string_y >> string_z;
			float xn, yn, zn;
			xn = atof(string_x.c_str());
			yn = atof(string_y.c_str());
			zn = atof(string_z.c_str());
			vn.push_back(glm::vec3(xn, yn, zn));
		}
		else if (type == "f") {	// only reads triangulated .obj files
			vector< glm::vec3 > ref;
			string vert;

			int numVerts = 0;
			while (ss >> vert) {
				istringstream vertSS(vert);
				string subv, subvt, subvn;
				getline(vertSS, subv, '/');
				getline(vertSS, subvt, '/');
				getline(vertSS, subvn, '/');
				int v_num = atoi(subv.c_str());
				int vt_num = atoi(subvt.c_str());
				int vn_num = atoi(subvn.c_str());
				ref.push_back(glm::vec3(v_num, vt_num, vn_num));
				numVerts++;
			}

			for (int i = 0; i < numVerts; i++) {
				int v_pos = ref[i].x - 1;
				int vt_pos = ref[i].y - 1;
				int vn_pos = ref[i].z - 1;

				info.push_back(v[v_pos].x);
				info.push_back(v[v_pos].y);
				info.push_back(v[v_pos].z);

				info.push_back(vt[vt_pos].x);
				info.push_back(vt[vt_pos].y);

				info.push_back(vn[vn_pos].x); // go back and do each of them (x,y,z) (x,y) (x,y,z)
				info.push_back(vn[vn_pos].y);
				info.push_back(vn[vn_pos].z);
			}

		}
		else {
			//cout << "WARNING: unknown command" << endl;
			continue;
		}
	}
	objFile.close();

	return info;
}


// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(const char* shaderFile){
	FILE *fp;
	long length;
	char *buffer;

	// open the file containing the text of the shader code
	fp = fopen(shaderFile, "r");

	// check for errors in opening the file
	if (fp == NULL) {
		printf("can't open shader source file %s\n", shaderFile);
		return NULL;
	}

	// determine the file size
	fseek(fp, 0, SEEK_END); // move position indicator to the end of the file;
	length = ftell(fp);  // return the value of the current position

	// allocate a buffer with the indicated number of bytes, plus one
	buffer = new char[length + 1];

	// read the appropriate number of bytes from the file
	fseek(fp, 0, SEEK_SET);  // move position indicator to the start of the file
	fread(buffer, 1, length, fp); // read all of the bytes

	// append a NULL character to indicate the end of the string
	buffer[length] = '\0';

	// close the file
	fclose(fp);

	// return the string
	return buffer;
}

// Create a GLSL program object from vertex and fragment shader files
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName){
	GLuint vertex_shader, fragment_shader;
	GLchar *vs_text, *fs_text;
	GLuint program;

	// check GLSL version
	printf("GLSL version: %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Create shader handlers
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	// Read source code from shader files
	vs_text = readShaderSource(vShaderFileName);
	fs_text = readShaderSource(fShaderFileName);

	// error check
	if (vs_text == NULL) {
		printf("Failed to read from vertex shader file %s\n", vShaderFileName);
		exit(1);
	} else if (DEBUG_ON) {
		printf("Vertex Shader:\n=====================\n");
		printf("%s\n", vs_text);
		printf("=====================\n\n");
	}
	if (fs_text == NULL) {
		printf("Failed to read from fragent shader file %s\n", fShaderFileName);
		exit(1);
	} else if (DEBUG_ON) {
		printf("\nFragment Shader:\n=====================\n");
		printf("%s\n", fs_text);
		printf("=====================\n\n");
	}

	// Load Vertex Shader
	const char *vv = vs_text;
	glShaderSource(vertex_shader, 1, &vv, NULL);  //Read source
	glCompileShader(vertex_shader); // Compile shaders
	
	// Check for errors
	GLint  compiled;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		printf("Vertex shader failed to compile:\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(vertex_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}
	
	// Load Fragment Shader
	const char *ff = fs_text;
	glShaderSource(fragment_shader, 1, &ff, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);
	
	//Check for Errors
	if (!compiled) {
		printf("Fragment shader failed to compile\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(fragment_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}

	// Create the program
	program = glCreateProgram();

	// Attach shaders to program
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	// Link and set program to use
	glLinkProgram(program);

	return program;
}
