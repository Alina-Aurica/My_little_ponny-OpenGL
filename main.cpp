#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>

// window
gps::Window myWindow;

const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
// dir light
glm::vec3 lightDir;
glm::vec3 lightColor;
// punciform light
glm::vec3 lightPos;
glm::vec3 lightPosColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
GLint lightPosLoc;
GLint lightPosColorLoc;

//for fog
GLint fogCond;
GLint fogCondLoc;

//for night mode
GLint nightFlag;
GLint nightFlagLoc;


glm::mat4 lightRotation;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 0.0f, 8.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.1f;

GLboolean pressedKeys[1024];

// models
//gps::Model3D teapot;
gps::Model3D scena;
gps::Model3D lightCube;
gps::Model3D screenQuad;
gps::Model3D ponei;
GLfloat angle;
GLfloat lightAngle;

// shaders
gps::Shader myBasicShader;
gps::Shader lightShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;

//skybox
gps::SkyBox mySkyBox;
gps::Shader skyboxShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;
bool showDepthMap;
const GLfloat near_plane = 0.1f, far_plane = 80.0f;
std::vector<const GLchar*> faces;

//for mouse
bool firstMouse = true;
float sensitivity = 0.07f;
float pitch, yaw;
float lastX, lastY;

//for pony's animation
float delta = 0;
float movementSpeed = 0.05; // units per second
double lastTimeStamp = glfwGetTime();

//for animation prez
float angleY;
bool prezFlag = false;

//for pony animation
bool ponyFlag = false;

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_STACK_OVERFLOW:
                error = "STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                error = "STACK_UNDERFLOW";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
	//TODO
    /*
    WindowDimensions resize;
    resize.width = width;
    resize.height = height;
    myWindow.setWindowDimensions(resize);
    */
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) { //FOG
        fogCond = (fogCond + 1) % 2;
        myBasicShader.useShaderProgram();
        glUniform1i(fogCondLoc, fogCond);
    }
    
    
    if (key == GLFW_KEY_Y && action == GLFW_PRESS) { //NIGHT MODE
        nightFlag = (nightFlag + 1) % 2;
        myBasicShader.useShaderProgram();
        glUniform1i(nightFlagLoc, nightFlag);
        
    }
    

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    //TODO
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xDiff = (float)xpos - lastX;
    float yDiff = (float)ypos - lastY;
    lastX = (float)xpos;
    lastY = (float)ypos;

    xDiff *= sensitivity;
    yDiff *= sensitivity;

    yaw -= xDiff;
    pitch -= yDiff;

    //rotatiile pot functiona pana la 90 de grade
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    myCamera.rotate(pitch, yaw);

    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir));
}

float distFromTreeHouse() { //distanta catre casa din copac
    return sqrtf((myCamera.getCameraPosition().x - (-23.2051f)) * (myCamera.getCameraPosition().x - (-23.2051f)) + (myCamera.getCameraPosition().z - (-26.549f)) * (myCamera.getCameraPosition().z - (-26.549f)));
}


float distFromTwilight() { //distanta catre ponei
    return sqrtf((myCamera.getCameraPosition().x - (-12.6403f)) * (myCamera.getCameraPosition().x - (-12.6403f)) + (myCamera.getCameraPosition().z - (-12.125f)) * (myCamera.getCameraPosition().z - (-12.125f)));
}

float distFromJapanaseHouse() { //distanta catre casa japoneza
    return sqrtf((myCamera.getCameraPosition().x - (1.28373f)) * (myCamera.getCameraPosition().x - (1.28373f)) + (myCamera.getCameraPosition().z - (8.14274f)) * (myCamera.getCameraPosition().z - (8.14274f)));
}

float distFromHouse() { //distanta catre casa
    return sqrtf((myCamera.getCameraPosition().x - (-22.7173f)) * (myCamera.getCameraPosition().x - (-22.7173f)) + (myCamera.getCameraPosition().z - (-1.99801f)) * (myCamera.getCameraPosition().z - (-1.99801f)));
}

void processMovement() {
	if (pressedKeys[GLFW_KEY_W]) {
        //poti merge atat timp cand nu intri in obiecte
        if(distFromTreeHouse() > 4.0f && distFromTwilight() > 2.5f && distFromJapanaseHouse() > 5.0f && distFromHouse() > 4.0f)
		    myCamera.move(gps::MOVE_FORWARD, cameraSpeed);

		//update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_S]) {
        //if (distFromTreeHouse() > 2)
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_A]) {
        //poti merge atat timp cand nu intri in obiecte
        if (distFromTreeHouse() > 4.0f && distFromTwilight() > 2.5f)
		    myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_D]) {
        //poti merge atat timp cand nu intri in obiecte
        if (distFromTreeHouse() > 4.0f && distFromTwilight() > 2.5f)
		    myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

    if (pressedKeys[GLFW_KEY_Q]) {
        angle -= 1.0f;
        // update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        angle += 1.0f;
        // update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }

    if (pressedKeys[GLFW_KEY_J]) { //rotire cubul de lumina
        lightAngle -= 1.0f;
        // update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_L]) { //rotire cubul de lumina
        lightAngle += 1.0f;
        // update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_O]) { //SOLID
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (pressedKeys[GLFW_KEY_P]) { //WIREFRAME
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    if (pressedKeys[GLFW_KEY_R]) { //POINT
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    }

    
    if (pressedKeys[GLFW_KEY_N]) { //ANIMATIE SCENA 
        prezFlag = true;
    }

    if (pressedKeys[GLFW_KEY_M]) { //OPRIRE ANIMATIE SCENA
        prezFlag = false;
    }

    if (pressedKeys[GLFW_KEY_G]) { //PORNIRE PONEI SA LEVITEZE
        ponyFlag = true;
    }

    if (pressedKeys[GLFW_KEY_H]) { //OPRIRE PONEI SA LEVITEZE
        ponyFlag = false;
    }

    
}

void initOpenGLWindow() {
    myWindow.Create(1980, 1012, "OpenGL Project Core");
}

void setWindowCallbacks() { 
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
    //teapot.LoadModel("models/teapot/teapot20segUT.obj");
    scena.LoadModel("models/scena/scena_cu_ponei_si_iarba.obj", "models/scena//");
    lightCube.LoadModel("models/cube/cube.obj");
    screenQuad.LoadModel("models/quad/quad.obj");
    ponei.LoadModel("models/rainbow_dash/rainbowDash.obj", "models/rainbow_dash//");
    
}

void initShaders() {
	myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    myBasicShader.useShaderProgram();

    lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
    lightShader.useShaderProgram();
    screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
    screenQuadShader.useShaderProgram();
    depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
    depthMapShader.useShaderProgram();
}

void initUniforms() {
	myBasicShader.useShaderProgram();

    // create model matrix for scena
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");
    //send matrix data to vertex shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	// send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for scena
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
	//glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

    //set the light punctif (direction towards the light)
    lightPos = glm::vec3(-4.80162f, -3.58064f, -20.3976f); //probabil ca aici pun pozitia lampii -- nu imi pleaca de la pozitia care trebuie
    lightPosLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightPos");
    // send light punctif to shader
    //glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightPos));

	//set light dir color
    lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light dir color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    //set light point color
    lightPosColor = glm::vec3(1.0f, 0.0f, 0.0f); //red light
    lightPosColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightPosColor");
    // send light point color to shader
    glUniform3fv(lightPosColorLoc, 1, glm::value_ptr(lightPosColor));

    //set fogCond
    fogCond = 1;
    fogCondLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogCond");
    glUniform1i(fogCondLoc, fogCond);

    //set nightMode
    nightFlag = 1;
    nightFlagLoc = glGetUniformLocation(myBasicShader.shaderProgram, "nightFlag");
    glUniform1i(nightFlagLoc, nightFlag);

    //for lightShader
    lightShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void initFBO() {
    //TODO - Create the FBO, the depth texture and attach the depth texture to the FBO

    //generate FBO ID
    glGenFramebuffers(1, &shadowMapFBO);

    //create depth texture for FBO
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    //attach texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    //unbind our frame buffer, otherwise it would be the only frame buffer used by OpenGl
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
	//TODO - Return the light-space transformation matrix
	
	glm::mat4 lightView = glm::lookAt((glm::mat3(lightRotation) * 25.0f * lightDir), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	
	//const GLfloat near_plane = 0.1f, far_plane = 90.0f;
	glm::mat4 lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;

	return lightSpaceTrMatrix;

}

void initSkyBox() {

    faces.push_back("skybox/right.tga");
    faces.push_back("skybox/left.tga");
    faces.push_back("skybox/top.tga");
    faces.push_back("skybox/bottom.tga");
    faces.push_back("skybox/back.tga");
    faces.push_back("skybox/front.tga");

    mySkyBox.Load(faces);

    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    skyboxShader.useShaderProgram();

    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE,
        glm::value_ptr(view));

    projection = glm::perspective(glm::radians(45.0f), (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height, 0.1f, 1000.0f);
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

}

/*
void renderTeapot(gps::Shader shader) {
    // select active shader program
    shader.useShaderProgram();

    //send teapot model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //send teapot normal matrix data to shader
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // draw teapot
    teapot.Draw(shader);
}
*/

void updateDelta(double elapsedSeconds) {
    delta = delta + movementSpeed * elapsedSeconds;
}

void animationScene() { 
    if (prezFlag == true) {
        angleY += 0.5f;
        myCamera.animScene(angleY);
    }
}

void renderPonei(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();

    // get current time

    //double currentTimeStamp = glfwGetTime();
    //updateDelta(currentTimeStamp - lastTimeStamp);
    //lastTimeStamp = currentTimeStamp;

    //delta += 0.01;
    if (ponyFlag == true) {
        double currentTimeStamp = glfwGetTime();
        updateDelta(currentTimeStamp - lastTimeStamp);
        lastTimeStamp = currentTimeStamp;
        model = glm::translate(model, glm::vec3(0, delta, 0));
    }
    else
        model = glm::mat4(1.0f);
    //model = glm::mat4(1.0f);
    modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");
    //glCheckError();
    //send matrix data to vertex shader
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    //glCheckError();

    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
 
    ponei.Draw(shader);
    
}

void renderScena(gps::Shader shader, bool depthPass) {

    // select active shader program
    shader.useShaderProgram();

    //send scena model matrix data to shader
    //glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //send scena normal matrix data to shader
    //glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    //model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    // draw scena
    scena.Draw(shader);
    
    
    /*
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    */
}

void renderScene() {
    // render the scene to the depth buffer
    
    depthMapShader.useShaderProgram();
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
 
	glClear(GL_DEPTH_BUFFER_BIT);

    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
        1,
        GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix()));

    
    renderScena(depthMapShader, true);
    renderPonei(depthMapShader, true);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    

    // final scene rendering pass (with shadows)
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    myBasicShader.useShaderProgram();
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

    //bind the shadow map
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
            1,
            GL_FALSE,
            glm::value_ptr(computeLightSpaceTrMatrix()));

    renderScena(myBasicShader, false);
    renderPonei(myBasicShader, false);

    //draw a white cube around the light
    lightShader.useShaderProgram();

    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

    model = lightRotation;
    model = glm::translate(model, 1.0f * lightDir);
    model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    lightCube.Draw(lightShader);

    mySkyBox.Draw(skyboxShader, view, projection);


	// render the teapot
	//renderTeapot(myBasicShader);

    //render the scene
	//renderScena(myBasicShader);

}


void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initFBO();
    initOpenGLState();
	initModels();
	initShaders();
	initUniforms();
    initSkyBox();
    setWindowCallbacks();

	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
	    renderScene();
        animationScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

        glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
