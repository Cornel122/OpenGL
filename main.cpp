#if defined (_APPLE_)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"

#include <iostream>
#include <vector>

int glWindowWidth = 1920;
int glWindowHeight = 1080;
int retina_width, retina_height;
GLFWwindow* glWindow = nullptr;
struct RainDrop {
    glm::vec3 position;
    float speed;
};

std::vector<RainDrop> rain;
const int RAIN_COUNT = 1500;

const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

glm::mat4 model, view, projection, lightRotation;
glm::mat3 normalMatrix;
GLuint modelLoc, viewLoc, projectionLoc, normalMatrixLoc;
GLuint lightDirLoc, lightColorLoc;

glm::vec3 lightDir(0.0f, 1.0f, 1.0f);
bool pointLightEnabled = true;


glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
bool rainEnabled = true;

gps::Camera myCamera(glm::vec3(6.0f, 0.8f, 3.0f),
    glm::vec3(0.0f, 0.8f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

float cameraSpeed = 0.01f;
float angleY = 0.0f;
float lightAngle = 0.0f;
float lastX = 400, lastY = 300;
bool firstMouse = true;

float helicopterAngle = 0.0f;
bool pressedKeys[1024] = { false };

int renderMode = 0; 
bool showDepthMap = false;
bool presentationMode = false;
gps::Model3D Scena, elicopter,screenQuad,depthMap;
gps::Shader myCustomShader, lightShader, screenQuadShader, depthMapShader, skyboxShader;
gps::SkyBox mySkyBox;

GLuint shadowMapFBO, depthMapTexture;
void windowResizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
void initRain() {
    rain.clear();
    for (int i = 0; i < RAIN_COUNT; i++) {
        RainDrop drop;
        drop.position = glm::vec3(
            (rand() % 100 - 50) / 5.0f,  
            (rand() % 100) / 5.0f + 5.0f, 
            (rand() % 100 - 50) / 5.0f   
        );
        drop.speed = 0.05f + (rand() % 100) / 2000.0f;
        rain.push_back(drop);
    }
}
void updateRain() {
    for (auto& drop : rain) {
        drop.position.y -= drop.speed;

        if (drop.position.y < -2.0f) {
            drop.position.y = 6.0f;
            drop.position.x += 0.1f;
        }
    }
}
void drawRain() {
    glUseProgram(0); 
    glLineWidth(1.0f);
    glBegin(GL_LINES);

    glColor3f(0.6f, 0.6f, 0.7f); 

    for (auto& drop : rain) {
        glVertex3f(drop.position.x, drop.position.y, drop.position.z);
        glVertex3f(drop.position.x, drop.position.y - 0.2f, drop.position.z);
    }

    glEnd();
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_M && action == GLFW_PRESS)
        showDepthMap = !showDepthMap;

    if (key == GLFW_KEY_P && action == GLFW_PRESS)
        presentationMode = !presentationMode; 
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
        pointLightEnabled = !pointLightEnabled;
    
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) renderMode = 0;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) renderMode = 1;
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) renderMode = 2;
   
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        rainEnabled = !rainEnabled; 

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) pressedKeys[key] = true;
        else if (action == GLFW_RELEASE) pressedKeys[key] = false;
    }
}


void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    myCamera.rotate(yoffset * sensitivity, xoffset * sensitivity);
}

void processMovement() {
    if (pressedKeys[GLFW_KEY_Q]) angleY -= 1.0f;
    if (pressedKeys[GLFW_KEY_E]) angleY += 1.0f;
    if (pressedKeys[GLFW_KEY_J]) lightAngle -= 1.0f;
    if (pressedKeys[GLFW_KEY_L]) lightAngle += 1.0f;
    if (pressedKeys[GLFW_KEY_W]) myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
    if (pressedKeys[GLFW_KEY_S]) myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
    if (pressedKeys[GLFW_KEY_A]) myCamera.move(gps::MOVE_LEFT, cameraSpeed);
    if (pressedKeys[GLFW_KEY_D]) myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
}

void initObjects() {
	screenQuad.LoadModel("objects/quad/quad.obj");
    Scena.LoadModel("objects/Scena_Proiect/Scena_Proiect.obj");
	elicopter.LoadModel("objects/elicopter/elicopter.obj");

}

void initSkybox() {
    std::vector<const GLchar*> faces = {
        "skybox/right.tga",
        "skybox/left.tga",
        "skybox/top.tga",
        "skybox/bottom.tga",
        "skybox/back.tga",
        "skybox/front.tga"
    };
    mySkyBox.Load(faces);
}

void initShaders() {
    myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
    lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
    screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
    depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
}

void initUniforms() {
    myCustomShader.useShaderProgram();
    glm::vec3 pointLightPos(6.5f, -1.0f, 1.8f);
    glm::vec3 pointLightColor(1.0f, 0.6f, 0.2f);
    glUseProgram(myCustomShader.shaderProgram);
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "pointLight.color"), 1, glm::value_ptr(pointLightColor));
    float pointConstant = 1.0f;
    float pointLinear = 0.02f;
    float pointQuadratic = 0.005f;
    myCustomShader.useShaderProgram();
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "pointLight.position"), 1, glm::value_ptr(pointLightPos));
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "pointLight.color"), 1, glm::value_ptr(pointLightColor));
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "pointLight.constant"), pointConstant);
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "pointLight.linear"), pointLinear);
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "pointLight.quadratic"), pointQuadratic);

    model = glm::mat4(1.0f);
    modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(model));

    normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 100.0f);
    projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

    lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
}

void initFBO() {
    glGenFramebuffers(1, &shadowMapFBO);

    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0,1.0,1.0,1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
glm::mat4 computeLightSpaceTrMatrix(float lightAngle) {
    float radius = 10.0f; 
    float lx = sin(glm::radians(lightAngle)) * radius;
    float lz = cos(glm::radians(lightAngle)) * radius;
    float ly = 5.0f; 

    glm::vec3 lightPos(lx, ly, lz);     
    glm::vec3 target(0.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);     

    glm::mat4 lightView = glm::lookAt(lightPos, target, up);
    glm::mat4 lightProj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 50.0f);

    return lightProj * lightView;       
}



void drawObjects(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();

    
    float radius = 5.0f; 
    float angleRad = glm::radians(helicopterAngle);

    float x = radius * cos(angleRad);
    float z = radius * sin(angleRad);
    float y = 1.5f; 

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, z)); 
    model = glm::rotate(model, angleRad, glm::vec3(0.0f, 1.0f, 0.0f)); 
    model = glm::rotate(model, glm::radians(-180.0f), glm::vec3(0, 0, 1));
    model = glm::scale(model, glm::vec3(0.5f));

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    elicopter.Draw(shader);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(0, -4.2, 1));
    model = glm::rotate(model, glm::radians(-180.0f), glm::vec3(0, 0, 1));
    model = glm::rotate(model, glm::radians(angleY), glm::vec3(0, 1, 0));

   

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(model));

        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    Scena.Draw(shader);

   
}

void renderScene() {
    if (renderMode == 0) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else if (renderMode == 1) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

    
    depthMapShader.useShaderProgram();
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix(lightAngle)));
    drawObjects(depthMapShader, true);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
   
    
    glViewport(0, 0, retina_width, retina_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (showDepthMap) {
        screenQuadShader.useShaderProgram();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);
        glDisable(GL_DEPTH_TEST);
        screenQuad.Draw(screenQuadShader);
        glEnable(GL_DEPTH_TEST);
        return;
    }
    float deltaTime = 0.01f; 
    helicopterAngle += 30.0f * deltaTime; 
    if (helicopterAngle > 360.0f) helicopterAngle -= 360.0f;


    myCustomShader.useShaderProgram();
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "fogDensity"), 0.005f);
    
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "fogDensity"), 0.01f);

    glm::vec3 camPos = glm::vec3(0.0f, 0.8f, 3.0f);
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "viewPos"), 1, glm::value_ptr(camPos));


    view = myCamera.getViewMatrix();
    if (presentationMode) {
        float t = glfwGetTime(); 
        float radius = 5.0f;
        float camY = 2.0f;
        glm::vec3 camPos = glm::vec3(radius * sin(t * 0.3f), camY, radius * cos(t * 0.3f));
        glm::vec3 target = glm::vec3(4.0f, 2.0f, 0.0f); 
        view = glm::lookAt(camPos, target, glm::vec3(0, 1, 0));
    }   
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0, 1, 0));
    glm::vec3 rotatedLightDir = glm::mat3(lightRotation) * lightDir;
    glm::vec3 lightDirView = glm::mat3(view) * rotatedLightDir;

    glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirView));

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);
    glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix(lightAngle)));
    drawObjects(myCustomShader, false);

    
    mySkyBox.Draw(skyboxShader, view, projection);
}

int main() {
    if (!glfwInit()) return -1;

    glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Scene", nullptr, nullptr);
    if (!glWindow) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(glWindow);

#if not defined(_APPLE_)
    glewExperimental = GL_TRUE;
    glewInit();
#endif

    glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);
    glEnable(GL_DEPTH_TEST);
    glfwSetKeyCallback(glWindow, keyboardCallback);
    glfwSetCursorPosCallback(glWindow, mouseCallback);
    glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
    glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    initRain();

    initObjects();
    initSkybox();
    initShaders();
    initUniforms();
    initFBO();

    while (!glfwWindowShouldClose(glWindow)) {
        processMovement();
        renderScene();
        if (rainEnabled) {
            updateRain();

            drawRain();
        }
        glfwSwapBuffers(glWindow);
        glfwPollEvents();
    }

    glDeleteTextures(1, &depthMapTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &shadowMapFBO);
    glfwTerminate();
    return 0;
}
