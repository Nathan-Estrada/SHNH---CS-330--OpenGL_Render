#include <iostream>
#include <cstdlib>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"     // Image loading Utility functions

//GLM Math headers
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//Sphere header
#include "sphere.h"

//Camera header
#include "camera.h"

using namespace std;

#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

namespace {
	const char* const winName = "Module 7 Final";

	const int winWidth = 800;
	const int winHeight = 600;

	int p = 2; //Initialize int p to an even integer. p will later be used to switch between projection styles

	//Create struct to contain mesh data
	struct Mesh {
		GLuint vao; //Vertex Array Object
		GLuint vbos[3]; //Vertex Buffer Object
		GLuint nIndices;
		GLuint numVertices; //Number of vertices
	};

	//Sphere uses its own seprate vao, vbo, and indices
	GLuint sphereVaoID;
	GLuint sphereVboID;
	GLuint sphereIboID;
	//Main GLFW window
	GLFWwindow* gWindow = nullptr;

	//Mesh shapes
	Mesh cube;
	Mesh cylinder;
	Mesh Torus;
	Mesh plane;
	Mesh pyramid;
	Sphere sphere;

	//Texture IDs, scale, and wrap mode
	GLuint mugTextureID;
	GLuint planeTextureID;
	GLuint coverTextureID;
	GLuint paperTextureID;
	GLuint pencilTextureID;
	GLuint pencilTipTextureID;
	GLuint eraserTextureID;
	GLuint teaPotTextureID;
	GLuint plasticTextureID;
	glm::vec2 gUVScale(1.0f, 1.0f);
	GLint gTexWrapMode = GL_REPEAT;

	//Shader programs
	GLuint lightProgramID;
	GLuint programID;

	// camera
	Camera cam(glm::vec3(0.0f, 0.0f, 3.0f));
	float lastX = winWidth / 2.0f;
	float lastY = winHeight / 2.0f;
	bool firstMouse = true;


	//Timing used to control speed of movement
	float timePassed = 0.0f; //Time between frames
	float lastFrame = 0.0f;
	float moveSpeed = 3.0f;

	//Light color, position, and scale
	glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);
	glm::vec3 gLightPos(3.f, 5.f, 4.f);
	glm::vec3 gLightScale(1.f);
}

bool initialize(int, char* [], GLFWwindow** window); //Initialize program
void resizeWin(GLFWwindow* window, int width, int height); //Resize window when called
void takeInput(GLFWwindow* window); //Processes user input
void mousePosCallback(GLFWwindow* window, double x, double y); //Callback for glfwSetCursorPosCallback
void mouseWheelCallback(GLFWwindow* window, double x, double y); //Callback for glfwScrollCallback
void mouseClickCallback(GLFWwindow* window, int button, int input, int mods); //Callback for glfwSetMouseButtonCallback
void buildCube(Mesh& cube);
void buildCylinder(Mesh& Cylinder);
void buildTorus(Mesh& Torus);
void buildPlane(Mesh& plane);
void buildPyramid(Mesh& pyramid);
void buildSphere(Sphere& sphere);
void destroyMeshes(Mesh& Cylinder, Mesh& Torus, Mesh& plane, Mesh& cube, Mesh& pyramid);
bool createTexture(const char* fileName, GLuint& textureID);
void destroyTexture(GLuint textureID);
void render();
bool buildShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programID);
void destroyShaderProgram(GLuint programID);

//Source code for vertex shader
const GLchar* vertShaderSource = GLSL(440,
	layout(location = 0) in vec3 position;
layout(location = 1) in vec2 textureCoordinate;
layout(location = 2) in vec3 normal;

out vec3 vertexFragmentPosition;
out vec2 vertexTextureCoordinate;
out vec3 vertexNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
	gl_Position = projection * view * model * vec4(position, 1.0f);

	vertexFragmentPosition = vec3(model * vec4(position, 1.0f));

	vertexTextureCoordinate = textureCoordinate;
	vertexNormal = mat3(transpose(inverse(model))) * normal;
}
);

//Source code for fragment shader
const GLchar* fragmentShaderSource = GLSL(440,
	in vec3 vertexNormal;
in vec3 vertexFragmentPosition;
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture;
uniform vec2 uvScale;

void main()
{
	float ambientStrength = 0.4f; // Set ambient or global lighting strength
	vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

	//Calculate Diffuse lighting*/
	vec3 norm = normalize(vertexNormal);
	vec3 lightDirection = normalize(lightPos - vertexFragmentPosition);
	float impact = max(dot(norm, lightDirection), 0.0);
	vec3 diffuse = impact * lightColor;

	//Calculate Specular lighting*/
	float specularIntensity = 0.8f;
	float highlightSize = 16.0f;
	vec3 viewDir = normalize(viewPosition - vertexFragmentPosition);
	vec3 reflectDir = reflect(-lightDirection, norm);

	float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
	vec3 specular = specularIntensity * specularComponent * lightColor;

	vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

	vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

	fragmentColor = vec4(phong, 1.0);
}
);

//Light shader source code
const GLchar* lightVertexShaderSource = GLSL(440,
	layout(location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
	gl_Position = projection * view * model * vec4(position, 1.0f);
}
);

//Light fragment shader source code
const GLchar* lightFragmentShaderSource = GLSL(440,
	out vec4 fragmentColor;

void main() {
	fragmentColor = vec4(1.0f);
}
);

void verticalFlip(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; j++)
	{
		int i1 = j * width * channels;
		int i2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; --i)
		{
			unsigned char temp = image[i1];
			image[i1] = image[i2];
			image[i2] = temp;
			i1++;
			i2++;
		}
	}
}

int main(int argc, char* argv[]) {
	//Create window to be displayed

	if (!initialize(argc, argv, &gWindow)) {
		return EXIT_FAILURE;
	}
	//Build shapes that are used to construct rest of meshes
	buildCylinder(cylinder);
	buildTorus(Torus);
	buildPlane(plane);
	buildPyramid(pyramid);
	buildCube(cube);

	if (!buildShaderProgram(vertShaderSource, fragmentShaderSource, programID)) {
		return EXIT_FAILURE;
	}

	if (!buildShaderProgram(lightVertexShaderSource, lightFragmentShaderSource, lightProgramID)) {
		return EXIT_FAILURE;
	}

	//Load textures and report any textures that fail to load
	const char* mugTextureName = "Textures/Gray Ceramic.png";
	if (!createTexture(mugTextureName, mugTextureID)) {
		cout << "Failed to load " << mugTextureName << endl;
	}
	else {
		cout << "Texture " << mugTextureName << " loaded successfully" << endl;
	}
	const char* planeTextureName = "Textures/marble.jpg";
	if (!createTexture(planeTextureName, planeTextureID)) {
		cout << "Failed to load " << planeTextureName << endl;
	}
	else {
		cout << "Texture " << planeTextureName << " loaded successfully" << endl;
	}
	const char* coverTexture = "Textures/notebook texture.jpg";
	if (!createTexture(coverTexture, coverTextureID)) {
		cout << "Failed to load " << coverTexture << endl;
	}
	else {
		cout << "Texture " << coverTexture << " loaded successfully" << endl;
	}
	const char* paperTexture = "Textures/paper.jpg";
	if (!createTexture(paperTexture, paperTextureID)) {
		cout << "Failed to load " << paperTexture << endl;
	}
	else {
		cout << "Texture " << paperTexture << " loaded successfully" << endl;
	}
	const char* pencilTexture = "Textures/pencil texture.png";
	if (!createTexture(pencilTexture, pencilTextureID)) {
		cout << "Failed to load " << pencilTexture << endl;
	}
	else {
		cout << "Texture " << pencilTexture << " loaded successfully" << endl;
	}
	const char* pencilTipTexture = "Textures/pencil tip texture.jpg";
	if (!createTexture(pencilTexture, pencilTipTextureID)) {
		cout << "Failed to load " << pencilTipTexture << endl;
	}
	else {
		cout << "Texture " << pencilTipTexture << " loaded successfully" << endl;
	}
	const char* eraserTexture = "Textures/eraser texture.png";
	if (!createTexture(eraserTexture, eraserTextureID)) {
		cout << "Failed to load " << eraserTexture << endl;
	}
	else {
		cout << "Texture " << eraserTexture << " loaded successfully" << endl;
	}
	const char* teaPotTexture = "Textures/steel.jpg";
	if (!createTexture(teaPotTexture, teaPotTextureID)) {
		cout << "Failed to load " << teaPotTexture << endl;
	}
	else {
		cout << "Texture " << teaPotTexture << " loaded successfully" << endl;
	}
	const char* plasticTexture = "Textures/black rubber.jpg";
	if (!createTexture(plasticTexture, plasticTextureID)) {
		cout << "Failed to load " << plasticTexture << endl;
	}
	else {
		cout << "Texture " << plasticTexture << " loaded successfully" << endl;
	}
	//Tell opengl which texture unit it belongs to
	glUseProgram(programID);
	//Set texture as texture unit 0
	glUniform1i(glGetUniformLocation(programID, "uTexture"), 0);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); //Background color of window is set to a solid black

	//Rendering loop while window is open
	while (!glfwWindowShouldClose(gWindow)) {
		float currentFrame = glfwGetTime();
		timePassed = currentFrame - lastFrame;
		lastFrame = currentFrame;

		takeInput(gWindow);

		render();

		//Swap buffers and poll inputs
		glfwPollEvents();
	}
	destroyMeshes(cylinder, Torus, plane, cube, pyramid);
	destroyTexture(mugTextureID);
	destroyTexture(planeTextureID);
	destroyTexture(coverTextureID);
	destroyShaderProgram(programID);
	destroyShaderProgram(lightProgramID);

	exit(EXIT_SUCCESS);
}

bool initialize(int argc, char* argv[], GLFWwindow** window) {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	* window = glfwCreateWindow(winWidth, winHeight, winName, NULL, NULL);
	if (*window == NULL) {
		cout << "Window creation failed" << endl;
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, resizeWin);
	glfwSetCursorPosCallback(*window, mousePosCallback); //Get mouse position
	glfwSetScrollCallback(*window, mouseWheelCallback); //Get scrollwheel inputs
	glfwSetMouseButtonCallback(*window, mouseClickCallback); //Get mouse click inputs

	//Hide mouse when program is running
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult) {
		cerr << glewGetErrorString(GlewInitResult) << endl;
		return false;
	}

	//If window creation sucessful, display OpenGL version
	cout << "Open GL Version: " << glGetString(GL_VERSION) << endl;
	return true;
}

//Assign key bindings to allow user to navigate and interact with program
void takeInput(GLFWwindow* window) {

	//Bind escape key to closing window
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	float camOffset = moveSpeed * timePassed;

	bool keyPress = false;

	//Move camera forward upon pressing W
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		cam.ProcessKeyboard(FORWARD, timePassed);
		cout << "Input: W" << endl;
		keyPress = true;
	}

	//Move camera to the left upon pressing A
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		cam.ProcessKeyboard(LEFT, timePassed);;
		cout << "Input: A" << endl;
		keyPress = true;
	}

	//Move camera backwards upon pressing S
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		cam.ProcessKeyboard(BACKWARD, timePassed);
		cout << "Input: S" << endl;
		keyPress = true;
	}

	//Move camera to the right upon pressing D
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		cam.ProcessKeyboard(RIGHT, timePassed);
		cout << "Input: D" << endl;
		keyPress = true;
	}

	//Move camera upward by pressing Q
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		cam.ProcessKeyboard(UPWARD, timePassed);
		cout << "Input: Q" << endl;
		keyPress = true;
	}

	//Move camera downward by pressing E
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		cam.ProcessKeyboard(DOWNWARD, timePassed);
		cout << "Input: E" << endl;
		keyPress = true;
	}

	//Every time P is pressed, one is added to int p, acting as a switch for the projection style
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
		p += 1;
		cout << "Input: P" << endl;
		keyPress = true;
	}


	//Upon pressing a key, give coordinates of cursor position at time of input
	if (keyPress) {
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		cout << "Mouse coordinates: " << x << ", " << y << endl;
	}
}

//Calls whenever mouse moves
void mousePosCallback(GLFWwindow* window, double x, double y) {
	if (firstMouse) {
		lastX = x;
		lastY = y;
		firstMouse = false;
	}

	float xStep = x - lastX;
	float yStep = lastY - y;

	lastX = x;
	lastY = y;

	cam.ProcessMouseMovement(xStep, yStep);
	cout << "Cursor at " << x << ", " << y << endl;
}

//Calls whenever scrollwheel is used
void mouseWheelCallback(GLFWwindow* window, double x, double y) {
	//Change speed
	if (y == 1) {
		moveSpeed += 9.0f;
	}
	else if (y == -1) {
		moveSpeed -= 2.0f;
	}
	cout << "Scrollwheel at " << x << ", " << y << endl;
}

//Calls whenever a mouse button is pressed
void mouseClickCallback(GLFWwindow* window, int button, int input, int mods) {
	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		if (input == GLFW_PRESS) {
			cout << "M1 pressed" << endl;
		}
		else {
			cout << "M1 released" << endl;
		}
		break;

	case GLFW_MOUSE_BUTTON_MIDDLE:
		if (input == GLFW_PRESS) {
			cout << "M3 pressed" << endl;
		}
		else {
			cout << "M3 released" << endl;
		}
		break;
	case GLFW_MOUSE_BUTTON_RIGHT:
		if (input == GLFW_PRESS) {
			cout << "M2 pressed" << endl;
		}
		else {
			cout << "M2 released" << endl;
		}
		break;

		//Default to take computer mice with more than 3 buttons into consideration
	default:
		cout << "Unrecognized mouse input" << endl;
		break;
	}
}

void resizeWin(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void render() {


	//Enables z-depth
	glEnable(GL_DEPTH_TEST);

	//Clear frame and z buffers
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Guides camera on the z axis
	glm::mat4 view = glm::translate(glm::vec3(0.0f, 15.0f, -30.0f));

	//Camera transformation 
	glm::mat4 pov = cam.GetViewMatrix();

	//Perspective projection
	glm::mat4 projection = glm::perspective(glm::radians(cam.Zoom), (GLfloat)winWidth / (GLfloat)winHeight, 0.1f, 100.0f);

	//Previously defined int p will determine projection style. For every time the user presses the P key, 1 is added to int p. If p is not an even number, 
	//projection style will switch to orthographic
	if (p % 2 != 0) {
		projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 5.0f);
	}

	//Set desired shader
	glUseProgram(programID);

	GLint lightColorLoc = glGetUniformLocation(programID, "lightColor");
	GLint lightPositionLoc = glGetUniformLocation(programID, "lightPos");
	glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
	glUniform3f(lightPositionLoc, gLightPos.x, gLightPos.y, gLightPos.z);

	//Pass previously defined matrices to shader
	GLint modelLocation = glGetUniformLocation(programID, "model");
	GLint viewLocation = glGetUniformLocation(programID, "view");
	GLint projLocation = glGetUniformLocation(programID, "projection");

	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(pov));
	glUniformMatrix4fv(projLocation, 1, GL_FALSE, glm::value_ptr(projection));

	GLint UVScaleLoc = glGetUniformLocation(programID, "uvScale");
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

	//Model matrix for mug base
	glm::mat4 scaleCylinder = glm::scale(glm::vec3(3.0f, 3.0f, 3.5f));
	glm::mat4 rotateCylinder = glm::rotate(-30.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 translateCylinder = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
	glm::mat4 modelCylinder = translateCylinder * rotateCylinder * scaleCylinder;
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelCylinder));

	//Activate VBOs for mug base
	glBindVertexArray(cylinder.vao);

	//Bind textures on mug base
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mugTextureID);

	//Draw triangles that make up mug base
	glDrawElements(GL_TRIANGLES, cylinder.nIndices, GL_UNSIGNED_SHORT, NULL);

	//Deactivate VAO and texture for mug base
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Model matrix for mug handle
	glm::mat4 scaleTorus = glm::scale(glm::vec3(1.0f, 1.0f, 2.0f)); //Set z axis to 2 to give more of a curved shape for the handle for the perspective given
	glm::mat4 rotateTorus = glm::rotate(100.5f, glm::vec3(1.0f, 0.0f, 0.0f)); //Rotate torus along x axis
	glm::mat4 translateTorus = glm::translate(glm::vec3(1.0f, 0.0f, 0.0f)); //Set Torus slightly towards right of origin so half of it clips into cylinder (Illusion of handle in mug)
	glm::mat4 modelTorus = translateTorus * rotateTorus * scaleTorus;
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelTorus));

	//Activate VBOs for mug handle
	glBindVertexArray(Torus.vao);

	//Bind textures on mug handle
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mugTextureID);

	//Draw triangles that make up mug handle
	glDrawElements(GL_TRIANGLES, Torus.nIndices, GL_UNSIGNED_SHORT, NULL);

	//Deactivate VAO for mug handle
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Model matrix for plane  (Countertop)
	glm::mat4 scalePlane = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
	glm::mat4 rotatePlane = glm::rotate(100.5f, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 translatePlane = glm::translate(glm::vec3(0.0f, 3.94f, -1.0f));
	glm::mat4 modelPlane = scalePlane * rotatePlane * translatePlane;

	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelPlane));

	//Activate VBOs for plane
	glBindVertexArray(plane.vao);

	//Bind textures on plane
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, planeTextureID);

	// Draw the triangles that make up the plane
	glDrawArrays(GL_TRIANGLES, 0, 6);

	//Deactivate VAO for plane
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Model matrix for plane (Notebook top cover)
	glm::mat4 scaleCover = glm::scale(glm::vec3(.23f, .25f, .25f));
	glm::mat4 rotateCover = glm::rotate(98.85f, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 translateCover = glm::translate(glm::vec3(14.0f, 2.0f, 11.0f));
	glm::mat4 modelCover = scaleCover * rotateCover * translateCover;

	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelCover));

	glBindVertexArray(plane.vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, coverTextureID);

	//Draw top cover
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	//Model matrix for plane(Notebook bottom cover)
	glm::mat4 translateTopCover = glm::translate(glm::vec3(14.0f, 1.f, 11.0f));
	glm::mat4 modelTopCover = scaleCover * rotateCover * translateTopCover;

	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelTopCover));

	glBindVertexArray(plane.vao);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Model matrix for cube (Paper)
	glm::mat4 scaleCube = glm::scale(glm::vec3(3.8f, 0.2f, 5.f));
	glm::mat4 rotateCube = glm::rotate(-0.1f, glm::vec3(.0f, 1.0f, .0f));
	glm::mat4 translateCube = glm::translate(glm::vec3(-3.f, -0.87f, 3.2f));
	//translate * rotate * scale
	glm::mat4 modelPaperCube = translateCube * rotateCube * scaleCube;

	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelPaperCube));

	glBindVertexArray(cube.vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, paperTextureID);

	glDrawArrays(GL_TRIANGLES, 0, cube.numVertices);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Model matrix for notebook rings (Toruses)
	GLfloat j = 0.7f;
	GLfloat k = -4.7f;
	for (int i = 0; i < 16; i++) {
		glm::mat4 scaleRing = glm::scale(glm::vec3(.2f, .2f, .4f));
		glm::mat4 rotateRing = glm::rotate(-0.1f, glm::vec3(.0f, 1.0f, .0f));
		glm::mat4 translateRing = glm::translate(glm::vec3(k, -0.87f, j));
		j += 0.3f;
		k -= 0.025f;
		glm::mat4 modelRing = translateRing * rotateRing * scaleRing;
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelRing));
		glBindVertexArray(Torus.vao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mugTextureID);
		glDrawElements(GL_TRIANGLES, Torus.nIndices, GL_UNSIGNED_SHORT, NULL);
		//Texture for mug handle will work for notebook rings as well
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Model matrix for pencil (Uses same cylinder as Cylinder)
	glm::mat4 scalePencil = glm::scale(glm::vec3(.2f, 2.f, .2f));
	glm::mat4 rotatePencil = glm::rotate(1.7f, glm::vec3(0.f, .35f, 1.f));
	glm::mat4 translatePencil = glm::translate(glm::vec3(-3.5f, -1.f, 6.2f));
	glm::mat4 modelPencil = translatePencil * rotatePencil * scalePencil;
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelPencil));
	glBindVertexArray(cylinder.vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pencilTextureID);
	glDrawElements(GL_TRIANGLES, cylinder.nIndices, GL_UNSIGNED_SHORT, NULL);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Model matrix for pencil tip (pyramid)
	glm::mat4 scaleTip = glm::scale(glm::vec3(.043f, .043f, .043f));
	glm::mat4 rotateTip = glm::rotate(1.85f, glm::vec3(0.f, .35f, 1.f));
	glm::mat4 translateTip = glm::translate(glm::vec3(-4.19f, -1.01f, 6.46f));
	glm::mat4 modelTip = translateTip * rotateTip * scaleTip;
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelTip));
	glBindVertexArray(pyramid.vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pencilTipTextureID);
	glDrawArrays(GL_TRIANGLES, 0, pyramid.numVertices);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Model matrix for eraser (sphere)
	glm::mat4 scaleEraser = glm::scale(glm::vec3(.07f, .07f, .07f));
	glm::mat4 rotateEraser = glm::rotate(0.1f, glm::vec3(.01f, .01f, .01f));
	glm::mat4 translateEraser = glm::translate(glm::vec3(-2.8f, -1.f, 5.94f));
	glm::mat4 modelEraser = translateEraser * rotateEraser * scaleEraser;
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelEraser));

	glGenVertexArrays(1, &sphereVaoID);
	glBindVertexArray(sphereVaoID);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, eraserTextureID);
	glGenBuffers(1, &sphereVboID);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVboID);        
	glBufferData(GL_ARRAY_BUFFER,            
		sphere.getInterleavedVertexSize(),
		sphere.getInterleavedVertices(),  
		GL_STATIC_DRAW);                  

	glGenBuffers(1, &sphereIboID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIboID);   
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,        
		sphere.getIndexSize(),        
		sphere.getIndices(),           
		GL_STATIC_DRAW);    

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	int stride = sphere.getInterleavedStride();  
	glVertexAttribPointer(0, 3, GL_FLOAT, false, stride, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, false, stride, (void*)(sizeof(float) * 3));
	glVertexAttribPointer(2, 2, GL_FLOAT, false, stride, (void*)(sizeof(float) * 6));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(sphereVaoID);
	glDrawElements(GL_TRIANGLES,           
		sphere.getIndexCount(),        
		GL_UNSIGNED_INT,                 
		(void*)0);                 

	// unbind VAO
	glBindVertexArray(0);

	//Model matrix for tea pot base(sphere)
	glm::mat4 scalePotBase = glm::scale(glm::vec3(2.5f, 2.5f, 2.5f));
	glm::mat4 rotatePotBase = glm::rotate(0.18f, glm::vec3(0.f, 1.0f, .0f));
	glm::mat4 translatePotBase = glm::translate(glm::vec3(4.5f, -1.1f, -2.f));
	glm::mat4 modelPotBase = translatePotBase * rotatePotBase * scalePotBase;
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelPotBase));

	glGenVertexArrays(1, &sphereVaoID);
	glBindVertexArray(sphereVaoID);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, teaPotTextureID);
	glGenBuffers(1, &sphereVboID);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVboID);          
	glBufferData(GL_ARRAY_BUFFER,              
		sphere.getInterleavedVertexSize(),
		sphere.getInterleavedVertices(),  
		GL_STATIC_DRAW);                  

	glGenBuffers(1, &sphereIboID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIboID);   
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,         
		sphere.getIndexSize(),            
		sphere.getIndices(),        
		GL_STATIC_DRAW); 

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(0, 3, GL_FLOAT, false, stride, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, false, stride, (void*)(sizeof(float) * 3));
	glVertexAttribPointer(2, 2, GL_FLOAT, false, stride, (void*)(sizeof(float) * 6));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(sphereVaoID);
	glDrawElements(GL_TRIANGLES,                 
		sphere.getIndexCount(),         
		GL_UNSIGNED_INT,                 
		(void*)0);                   

	// unbind VAO
	glBindVertexArray(0);

	//Model matrix for tea pot lid handle(sphere)
	glm::mat4 scaleHandle = glm::scale(glm::vec3(.3f, .3f, .3f));
	glm::mat4 rotateHandle = glm::rotate(0.18f, glm::vec3(0.f, 1.0f, .0f));
	glm::mat4 translateHandle = glm::translate(glm::vec3(4.5f, 1.6f, -2.f));
	glm::mat4 modelHandle = translateHandle * rotateHandle * scaleHandle;
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelHandle));

	glGenVertexArrays(1, &sphereVaoID);
	glBindVertexArray(sphereVaoID);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, plasticTextureID);
	glGenBuffers(1, &sphereVboID);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVboID);         
	glBufferData(GL_ARRAY_BUFFER,                
		sphere.getInterleavedVertexSize(), 
		sphere.getInterleavedVertices(),   
		GL_STATIC_DRAW);                   

	glGenBuffers(1, &sphereIboID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIboID);   
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
		sphere.getIndexSize(),
		sphere.getIndices(), 
		GL_STATIC_DRAW);          

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(0, 3, GL_FLOAT, false, stride, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, false, stride, (void*)(sizeof(float) * 3));
	glVertexAttribPointer(2, 2, GL_FLOAT, false, stride, (void*)(sizeof(float) * 6));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(sphereVaoID);
	glDrawElements(GL_TRIANGLES,           
		sphere.getIndexCount(),         
		GL_UNSIGNED_INT,               
		(void*)0);                     

	// unbind VAO
	glBindVertexArray(0);


	//Model matrix for tea pot handle
	glm::mat4 scalePotHandle = glm::scale(glm::vec3(2.0f, 2.0f, 3.0f)); //Set z axis to 2 to give more of a curved shape for the handle for the perspective given
	glm::mat4 rotatePotHandle = glm::rotate(100.55f, glm::vec3(1.0f, 0.0f, 0.0f)); //Rotate torus along x axis
	glm::mat4 translatePotHandle = glm::translate(glm::vec3(4.4f, 1.0f, -2.f)); //Set Torus slightly towards right of origin so half of it clips into cylinder (Illusion of handle in mug)
	glm::mat4 modelPotHandle = translatePotHandle * rotatePotHandle * scalePotHandle;
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelPotHandle));

	//Activate VBOs for mug handle
	glBindVertexArray(Torus.vao);

	//Bind textures on mug handle
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, plasticTextureID);

	//Draw triangles that make up mug handle
	glDrawElements(GL_TRIANGLES, Torus.nIndices, GL_UNSIGNED_SHORT, NULL);

	//Deactivate VAO for mug handle
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Model matrix for tea pot spout
	glm::mat4 scaleSpout = glm::scale(glm::vec3(1.f, 1.f, 1.f));
	glm::mat4 rotateSpout = glm::rotate(3.7f, glm::vec3(.0f, .0f, 1.0f));
	glm::mat4 translateSpout = glm::translate(glm::vec3(2.2f, 0.f, -2.f));
	glm::mat4 modelSpout = translateSpout * rotateSpout * scaleSpout;
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelSpout));

	//Activate VBOs for mug base
	glBindVertexArray(cylinder.vao);

	//Bind textures on mug base
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, teaPotTextureID);

	//Draw triangles that make up mug base
	glDrawElements(GL_TRIANGLES, cylinder.nIndices, GL_UNSIGNED_SHORT, NULL);

	//Deactivate VAO and texture for mug base
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Draw light cube
	glUseProgram(lightProgramID);
	glm::mat4 modelCube = glm::translate(gLightPos) * glm::scale(gLightScale);

	GLint lightModelLocation = glGetUniformLocation(lightProgramID, "model");
	GLint lightViewLocation = glGetUniformLocation(lightProgramID, "view");
	GLint lightProjLocation = glGetUniformLocation(lightProgramID, "projection");

	glUniformMatrix4fv(lightModelLocation, 1, GL_FALSE, glm::value_ptr(modelCube));
	glUniformMatrix4fv(lightViewLocation, 1, GL_FALSE, glm::value_ptr(pov));
	glUniformMatrix4fv(lightProjLocation, 1, GL_FALSE, glm::value_ptr(projection));

	//Activate VBOs for light cube
	glBindVertexArray(cube.vao);

	//Draw arrays that make up light cube
	glDrawArrays(GL_TRIANGLES, 0, cube.numVertices);

	//Deactivate VAO for cube
	glBindVertexArray(0);
	glUseProgram(0);

	glfwSwapBuffers(gWindow);
}

void buildCube(Mesh& cube) {
	{
		GLfloat verts[] = {
		   -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
			0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
			0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
			0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
		   -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
		   -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

		  -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
		   0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
		   0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
		   0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
		  -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
		  -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

		 -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
		 -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
		 -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		 -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		 -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
		 -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

	   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
	   -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
	   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
		};

		const GLuint floatsPerVertex = 3;
		const GLuint floatsPerNormal = 3;
		const GLuint floatsPerUV = 2;

		cube.numVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

		glGenVertexArrays(1, &cube.vao); // we can also generate multiple VAOs or buffers at the same time
		glBindVertexArray(cube.vao);

		// Create 2 buffers: first one for the vertex data; second one for the indices
		glGenBuffers(1, cube.vbos);
		glBindBuffer(GL_ARRAY_BUFFER, cube.vbos[0]); // Activates the buffer
		glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

		// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
		GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

		// Create Vertex Attribute Pointers
		glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
		glEnableVertexAttribArray(2);
	}
}

void buildCylinder(Mesh& Cylinder) {

	const float pi = glm::pi<float>();
	float radius = 0.25f;
	float height = 0.7f;
	int numSegments = 7000; //Number of segments per circle

	//Vertex and index data
	int numVerts = (numSegments + 1) * 2; // Two circles, each with numSegments vertices (Include center vertex or program will not run!)
	int indexCount = numSegments * 6; // Each quad has 2 triangles (6 vertices)

	GLfloat* vertices = new GLfloat[numVerts * 10]; // Each vertex has 7 floats (3 for position, 4 for color) EDIT:CHANGED TO 10
	GLushort* indices = new GLushort[indexCount];

	int vIndex = 0;
	int index = 0;

	//Generating top and bottom of cylinder
	for (int i = 0; i <= numSegments; ++i)
	{
		//Angle per segment
		float theta = 2.0f * pi * i / numSegments;
		float x = radius * std::cos(theta);
		float y = height / 2.0f;
		float z = radius * std::sin(theta);

		glm::vec3 vertexPosition(x, height / 2.0f, z);

		float r = 0.5f;
		float g = 0.5f;
		float b = 0.5f;
		float a = 1.0f;
		GLfloat u = static_cast<GLfloat>(i) / numSegments;

		//Top circle vertex coordinates in x,y,z format
		vertices[vIndex++] = x;
		vertices[vIndex++] = y;
		vertices[vIndex++] = z;


		//Top circle color coordinates in R G B A format(This combination makes gray)
		vertices[vIndex++] = 0.5f;
		vertices[vIndex++] = 0.5f;
		vertices[vIndex++] = 0.5f;
		vertices[vIndex++] = 1.0f;

		//Top circle normal color coordinates
		vertices[vIndex++] = 0.0f;
		vertices[vIndex++] = 1.f;
		vertices[vIndex++] = 0.0f;

		// Bottom circle vertex coordinates
		vertices[vIndex++] = x;
		vertices[vIndex++] = -y; //This line ensures the bottom circle is opposite from the top on the y axis
		vertices[vIndex++] = z;

		//Bottom circle color coordinates (also gray)
		vertices[vIndex++] = r;
		vertices[vIndex++] = g;
		vertices[vIndex++] = b;
		vertices[vIndex++] = a;

		//Bottom circle normal color coordinates
		vertices[vIndex++] = 0.0f;
		vertices[vIndex++] = 0.f;
		vertices[vIndex++] = 0.0f;
	}

	//Connecting the two circles to make the base of the cylinder
	for (int i = 0; i < numSegments; i++)
	{
		//Vertices for top
		int vertex0 = i;
		int vertex1 = i + 1;
		//Vertices for bottom
		int vertex2 = i + numSegments + 1;
		int vertex3 = i + numSegments + 2;

		//First half of quad
		indices[index++] = vertex0;
		indices[index++] = vertex2;
		indices[index++] = vertex1;

		//Second half of quad
		indices[index++] = vertex1;
		indices[index++] = vertex2;
		indices[index++] = vertex3;
	}
	//Generate and bind VAO
	glGenVertexArrays(1, &Cylinder.vao);
	glBindVertexArray(Cylinder.vao);

	glGenBuffers(3, Cylinder.vbos);

	//Bind vertex data to array buffer
	glBindBuffer(GL_ARRAY_BUFFER, Cylinder.vbos[0]);
	//Copy vertex data to VBO, pass size of data in bytes. Remember, there are 10 floats per triangle
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numVerts * 10, vertices, GL_STATIC_DRAW);

	//Bind fragment data to array buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Cylinder.vbos[1]);
	//Copy intex data to VBO, pass size of data in bites
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indexCount, indices, GL_STATIC_DRAW);

	//Bind fragment data to array buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Cylinder.vbos[2]);
	//Copy inDex data to VBO, pass size of data in bites
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indexCount, indices, GL_STATIC_DRAW);

	GLint stride = sizeof(float) * 10;

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * 3));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * 7));
	glEnableVertexAttribArray(1);

	delete[] vertices;
	delete[] indices;

	Cylinder.nIndices = indexCount;
}

void buildTorus(Mesh& Torus) {
	const float inRadius = 0.08f; //Thickness of Torus (Mug handle)
	const float outRadius = 0.8f; //Overall radius of Torus
	const int numSegments = 10; // Segments in Torus
	const int numSlices = 100; // Slices in Torus, determines smoothness on mug handle

	const float pi = glm::pi<float>(); //Use GLM Math to acquire pi float

	// Calculate vertex and index data for the torus
	int vertexCount = numSlices * numSegments; //Each segment represents a quad composed of 2 triangles
	int indexCount = numSlices * numSegments * 6; //Each quad has 6 vertices

	GLfloat* vertices = new GLfloat[vertexCount * 7]; //7 floats per vertex, 3 being postion values (x y z), 4 being color (r g b a)
	GLushort* indices = new GLushort[indexCount];

	int vertexIndex = 0;
	int index = 0;

	//Generate indices and vertices for Torus
	for (int slice = 0; slice < numSlices; slice++)
	{
		//Find angle for slice
		float phi = 2 * pi * slice / numSlices;
		float cosPhi = cos(phi);
		float sinPhi = sin(phi);

		for (int segment = 0; segment < numSegments; ++segment)
		{
			//Angle per segment
			float theta = 2.0f * pi * segment / numSegments;
			float cosTheta = cos(theta);
			float sinTheta = sin(theta);

			//Calculate x, y, and z coordinates of Torus
			float x = (outRadius + inRadius * cosTheta) * cosPhi;
			float y = (outRadius + inRadius * cosTheta) * sinPhi;
			float z = inRadius * sinTheta;

			//First three floats determine vertex position
			vertices[vertexIndex++] = x;
			vertices[vertexIndex++] = y;
			vertices[vertexIndex++] = z;

			//Last 4 floats determine color, this combination produces gray
			vertices[vertexIndex++] = 0.5;
			vertices[vertexIndex++] = 0.5;
			vertices[vertexIndex++] = 0.5;
			vertices[vertexIndex++] = 1.0;

			//Indices for the next slice and next segment
			int nextSlice = (slice + 1) % numSlices;
			int nextSegment = (segment + 1) % numSegments;

			//Indices for each vector (corner) per quad
			int vertex0 = slice * numSegments + segment;
			int vertex1 = slice * numSegments + nextSegment;
			int vertex2 = nextSlice * numSegments + nextSegment;
			int vertex3 = nextSlice * numSegments + segment;

			//First half of quad (First triangle)
			indices[index++] = vertex0;
			indices[index++] = vertex1;
			indices[index++] = vertex2;
			//Second half of quad (Second triangle)
			indices[index++] = vertex2;
			indices[index++] = vertex3;
			indices[index++] = vertex0;
		}
	}

	//Generate and bind VAO
	glGenVertexArrays(1, &Torus.vao);
	glBindVertexArray(Torus.vao);

	glGenBuffers(2, Torus.vbos);

	//Bind vertex data to array buffer
	glBindBuffer(GL_ARRAY_BUFFER, Torus.vbos[0]);
	//Copy vertex data to VBO, pass size of data in bytes. Remember, there are 7 floats per triangle
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertexCount * 7, vertices, GL_STATIC_DRAW);

	//Bind fragment data to array buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Torus.vbos[1]);
	//Copy intex data to VBO, pass size of data in bites
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indexCount, indices, GL_STATIC_DRAW);

	GLint stride = sizeof(float) * 7;

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * 3));
	glEnableVertexAttribArray(1);

	Torus.nIndices = indexCount;
}

void buildPyramid(Mesh& pyramid)
{
	// Position and Color data, commented by their indices 
	GLfloat vertices[] = {
		// Vertex Positions        //Normals                  // Texture coordinates

		-1.0f, -1.0f, -1.0f,		0.0f, -1.0f,  0.0f,        0.f, 1.f,
		 1.0f, -1.0f, -1.0f,		0.0f, -1.0f,  0.0f,        0.f, 0.f,
		-1.0f, -1.0f, 1.0f,			0.0f, -1.0f,  0.0f,        1.f, 0.f,

		  1.0f, -1.0f, -1.0f,		1.0f,  0.0f,  0.0f,        0.f, 0.f,
		1.0f, -1.0f, 1.0f,			1.0f,  0.0f,  0.0f,        1.f, 1.f,
		-1.0f, -1.0f, 1.0f,			1.0f,  0.0f,  0.0f,        1.f, 0.f,

		0.0f, 1.0f, 0.0f,			0.0f,  1.0f,  0.0f,        1.f, 0.f,
		-1.0f, -1.0f, 1.0f,		    0.0f,  1.0f,  0.0f,        0.f, 1.f,
		1.0f, -1.0f, 1.0f,			0.0f,  1.0f,  0.0f,        0.5f, 1.f,

		0.0f, 1.0f, 0.0f,			0.0f,  0.0f, -1.0f,        1.f, 0.f,
		1.0f, -1.0f, 1.0f,			0.0f,  0.0f, -1.0f,        0.f, 1.f,
		-1.0f, -1.0f, -1.0f,		0.0f,  0.0f, -1.0f,        0.5f, 1.f,

		 0.0f, 1.0f, 0.0f,			-1.0f,  0.0f,  0.0f,       1.f, 0.f,
		 1.0f, -1.0f, -1.0f,		-1.0f,  0.0f,  0.0f,       0.f, 1.f,
		-1.0f, -1.0f, -1.0f,		-1.0f,  0.0f,  0.0f,       0.5f, 1.f,

		//Front facing triangle 
		0.0f, 1.0f, 0.0f,			0.0f,  0.0f,  1.0f,        1.f, 0.f,
		-1.0f, -1.0f, -1.0f,		0.0f,  0.0f,  1.0f,        0.f, 1.f,
		-1.0f, -1.0f, 1.0f,		    0.0f,  0.0f,  1.0f,        0.5f, 1.f,
	};
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerTexture = 2;

	pyramid.numVertices = sizeof(vertices) / (sizeof(vertices[0]) * (floatsPerVertex + floatsPerNormal + floatsPerTexture));

	glGenVertexArrays(1, &pyramid.vao);
	glBindVertexArray(pyramid.vao);

	//Create 2 buffers for vertex buffer and index data
	glGenBuffers(1, &pyramid.vbos[0]);

	//Buffer for vertex data
	glBindBuffer(GL_ARRAY_BUFFER, pyramid.vbos[0]); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerTexture);

	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void buildSphere(Sphere& sphere) {
	sphere.build(3.0, 18, 36, 1);
}

void buildPlane(Mesh& plane) {
	float vertices[] = {
		//Position data			//Texture Coordinates		//Normals
		-10.0f, -5.0f, -8.0f,    0.f, 1.f,					0.0f, 1.0f, 0.0f,//Top left		
		 10.0f, -5.0f, -8.0f,    1.f, 1.f,					0.0f, 1.0f, 0.0f,//Top right
		 10.0f, -5.0f,  8.0f,    1.f, 0.f,					0.0f, 1.0f, 0.0f,//Bottom right
		 10.0f, -5.0f,  8.0f,    1.f, 0.f,					0.0f, 1.0f, 0.0f,//Bottom right
		-10.0f, -5.0f,  8.0f,    0.f, 0.f,					0.0f, 1.0f, 0.0f,//Bottom left
		-10.0f, -5.0f, -8.0f,    0.f, 1.f,					0.0f, 1.0f, 0.0f,//Top left
	};

	//Generate and bind VAO
	glGenVertexArrays(1, &plane.vao);
	glBindVertexArray(plane.vao);

	//Generate and bind VBOs
	glGenBuffers(1, &plane.vbos[0]);
	glBindBuffer(GL_ARRAY_BUFFER, plane.vbos[0]);
	//Copy vertex data to VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	GLint vertexLoc = 0; //Column for vertex data
	GLint textureLoc = 1; // Column for color data
	GLint normalLoc = 2; //Column for normals data

	GLint stride = sizeof(float) * 8;

	glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(vertexLoc);

	glVertexAttribPointer(textureLoc, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(textureLoc);

	glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 5));
	glEnableVertexAttribArray(normalLoc);

	plane.nIndices = 6; //nIndices = numRows in vertices array
}

void destroyMeshes(Mesh& Cylinder, Mesh& Torus, Mesh& plane, Mesh& cube, Mesh& pyramid) {
	glDeleteVertexArrays(1, &Cylinder.vao);
	glDeleteBuffers(1, &Cylinder.vbos[0]);

	glDeleteVertexArrays(1, &Torus.vao);
	glDeleteBuffers(1, &Torus.vbos[0]);

	glDeleteVertexArrays(1, &plane.vao);
	glDeleteBuffers(1, &plane.vbos[0]);

	glDeleteVertexArrays(1, &cube.vao);
	glDeleteBuffers(1, &cube.vbos[0]);

}

bool createTexture(const char* fileName, GLuint& textureID) {
	int width, height, channels;
	unsigned char* image = stbi_load(fileName, &width, &height, &channels, 0);
	if (image)
	{
		verticalFlip(image, width, height, channels);

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);


		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			cout << "Not implemented to handle image with " << channels << " channels" << endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0);

		return true;
	}
}

void destroyTexture(GLuint textureID) {
	glGenTextures(1, &textureID);
}

bool buildShaderProgram(const char* vertShaderSource, const char* fragShaderSource, GLuint& programID) {
	programID = glCreateProgram();

	GLuint vertShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertShaderID, 1, &vertShaderSource, NULL);
	glShaderSource(fragShaderID, 1, &fragShaderSource, NULL);

	//Compile vertex and fragment shaders
	glCompileShader(vertShaderID);
	glCompileShader(fragShaderID);

	//Attatch shaders to shader program
	glAttachShader(programID, vertShaderID);
	glAttachShader(programID, fragShaderID);

	//Link shader program
	glLinkProgram(programID);

	//Use shader program
	glUseProgram(programID);

	return true;
}

void destroyShaderProgram(GLuint programID) {
	glDeleteProgram(programID);
}