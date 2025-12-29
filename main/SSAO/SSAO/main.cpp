#define GLM_ENABLE_EXPERIMENTAL

#include <iostream>
#include <random>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Camera.h"
#include "glm/gtx/rotate_vector.hpp"

#define ERROR_LOG(ErrorMessage) glfwTerminate(); std::cout << ErrorMessage << std::endl; return -1;

// global state
static int screenWidth = 1280;
static int screenHeight = 720;
static Camera camera(glm::vec3(0.0f, 0.5f, 3.0f));
static glm::vec3 LightPos = glm::vec3(2.0f, 4.0f, 3.0f);

bool keys[1024];
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode);
void Move();
GLfloat lastX=0.0f, lastY=0.0f;
bool firstMouse = true;
GLfloat deltaTime = 0.0f, lastTime = 0.0f;
void MouseCallback(GLFWwindow * window, double xPos, double yPos);
void ScrollCallback(GLFWwindow *window, double xOffset, double yOffset);

// geometry
static GLuint modelVAO = 0, modelVBO = 0;
static int modelVertexCount = 0;
static GLuint floorVAO = 0, floorVBO = 0;

// quad
static GLuint quadVAO = 0, quadVBO = 0;

// buffers
static GLuint gBuffer = 0, gPosition = 0, gNormal = 0, gAlbedo = 0, rboDepth = 0;
static GLuint ssaoFBO = 0, ssaoBlurFBO = 0;
static GLuint ssaoColorBuffer = 0, ssaoColorBufferBlur = 0, noiseTexture = 0;

// ssao data
static std::vector<glm::vec3> ssaoKernel;

// Simple OBJ loader
static bool LoadOBJ(const char* path, std::vector<GLfloat>& outVertexData)
{
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec3> temp_normals;
    std::vector<unsigned int> vertexIndices, normalIndices;
    
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "Failed to open OBJ file: " << path << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            temp_vertices.push_back(vertex);
        }
        else if (prefix == "vn") {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal);
        }
        else if (prefix == "f") {
            std::string vertex1, vertex2, vertex3;
            unsigned int vertexIndex[3], textureIndex[3], normalIndex[3];
            
            iss >> vertex1 >> vertex2 >> vertex3;
            
            // Parse f v/vt/vn format
            sscanf(vertex1.c_str(), "%d/%d/%d", &vertexIndex[0], &textureIndex[0], &normalIndex[0]);
            sscanf(vertex2.c_str(), "%d/%d/%d", &vertexIndex[1], &textureIndex[1], &normalIndex[1]);
            sscanf(vertex3.c_str(), "%d/%d/%d", &vertexIndex[2], &textureIndex[2], &normalIndex[2]);
            
            vertexIndices.push_back(vertexIndex[0]);
            vertexIndices.push_back(vertexIndex[1]);
            vertexIndices.push_back(vertexIndex[2]);
            normalIndices.push_back(normalIndex[0]);
            normalIndices.push_back(normalIndex[1]);
            normalIndices.push_back(normalIndex[2]);
        }
    }
    
    file.close();
    
    // Build vertex data with white color
    outVertexData.clear();
    for (size_t i = 0; i < vertexIndices.size(); i++) {
        glm::vec3 vertex = temp_vertices[vertexIndices[i] - 1];
        glm::vec3 normal = temp_normals[normalIndices[i] - 1];
        
        // Position
        outVertexData.push_back(vertex.x);
        outVertexData.push_back(vertex.y);
        outVertexData.push_back(vertex.z);
        
        // Color (white)
        outVertexData.push_back(0.9f);
        outVertexData.push_back(0.9f);
        outVertexData.push_back(0.9f);
        
        // Normal
        outVertexData.push_back(normal.x);
        outVertexData.push_back(normal.y);
        outVertexData.push_back(normal.z);
    }
    
    return true;
}

static void LoadModel()
{
    std::vector<GLfloat> vertexData;
    
    if (!LoadOBJ("res/models/dragon.obj", vertexData)) {
        std::cout << "Failed to load dragon model!" << std::endl;
        return;
    }
    
    modelVertexCount = static_cast<int>(vertexData.size() / 9); // 9 floats per vertex (pos + color + normal)
    
    glGenVertexArrays(1, &modelVAO);
    glGenBuffers(1, &modelVBO);
    glBindVertexArray(modelVAO);
    glBindBuffer(GL_ARRAY_BUFFER, modelVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(GLfloat), vertexData.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)0);
    
    // Color attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    
    // Normal attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    
    glBindVertexArray(0);
}

static void CreateFloor()
{
    GLfloat floorVertices[] = {
        // Position            Color              Normal
        -5.0f, -0.5f, -5.0f,  0.8f, 0.8f, 0.8f,  0.0f, 1.0f, 0.0f,
         5.0f, -0.5f, -5.0f,  0.8f, 0.8f, 0.8f,  0.0f, 1.0f, 0.0f,
         5.0f, -0.5f,  5.0f,  0.8f, 0.8f, 0.8f,  0.0f, 1.0f, 0.0f,
         5.0f, -0.5f,  5.0f,  0.8f, 0.8f, 0.8f,  0.0f, 1.0f, 0.0f,
        -5.0f, -0.5f,  5.0f,  0.8f, 0.8f, 0.8f,  0.0f, 1.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.8f, 0.8f, 0.8f,  0.0f, 1.0f, 0.0f
    };
    
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    
    glBindVertexArray(0);
}

static void CreateQuad()
{
    if (quadVAO != 0) return;
    GLfloat quadVertices[] = {
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glBindVertexArray(0);
}

static void RenderQuad()
{
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

static void SetupGBuffer()
{
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    glGenTextures(1, &gAlbedo);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);

    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "GBuffer incomplete" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void SetupSSAO()
{
    // Helper lerp function from the book
    auto lerp = [](float a, float b, float f) -> float {
        return a + f * (b - a);
    };
    
    std::uniform_real_distribution<GLfloat> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;
    ssaoKernel.clear();
    ssaoKernel.reserve(64);
    for (GLuint i = 0; i < 64; i++)
    {
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator)
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        
        // Accelerating interpolation function to distribute more samples closer to origin
        float scale = (float)i / 64.0f;
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    std::vector<glm::vec3> ssaoNoise;
    for (GLuint i = 0; i < 16; i++)
    {
        glm::vec3 noise(randomFloats(generator) * 2.0f - 1.0f,
                        randomFloats(generator) * 2.0f - 1.0f,
                        0.0f);
        ssaoNoise.push_back(noise);
    }
    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenFramebuffers(1, &ssaoFBO);
    glGenFramebuffers(1, &ssaoBlurFBO);
    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidth, screenHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "SSAO FBO incomplete" << std::endl;
    }

    glGenTextures(1, &ssaoColorBufferBlur);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidth, screenHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "SSAO Blur FBO incomplete" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void SetViewport(GLFWwindow* window)
{
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    glViewport(0, 0, screenWidth, screenHeight);
}

static void RenderGeometry(Shader &shader, const glm::mat4& view, const glm::mat4& projection)
{
    shader.UseProgram();
    shader.SetMatrix4fv("view", view);
    shader.SetMatrix4fv("projection", projection);

    // Render the floor
    glBindVertexArray(floorVAO);
    glm::mat4 model = glm::mat4(1.0f);
    shader.SetMatrix4fv("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // Render the dragon model (no rotation)
    glBindVertexArray(modelVAO);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, -1.0f));
    model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
    shader.SetMatrix4fv("model", model);
    glDrawArrays(GL_TRIANGLES, 0, modelVertexCount);
    glBindVertexArray(0);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Fahim Boss", NULL, NULL);
    if (window == NULL) {ERROR_LOG("Failed to get window")}
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetScrollCallback(window, ScrollCallback);
    if (GLEW_OK != glewInit()) {ERROR_LOG("Failed to init glew")}
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    LoadModel();
    CreateFloor();
    CreateQuad();
    SetupGBuffer();
    SetupSSAO();

    Shader shaderGeometry = Shader("res/shaders/ssao_geometry.vs", "res/shaders/ssao_geometry.fs");
    Shader shaderLighting = Shader("res/shaders/ssao_lighting.vs", "res/shaders/ssao_lighting.fs");
    Shader shaderSSAO = Shader("res/shaders/ssao_quad.vs", "res/shaders/ssao.fs");
    Shader shaderSSAOBlur = Shader("res/shaders/ssao_quad.vs", "res/shaders/ssao_blur.fs");

    shaderSSAO.UseProgram();
    shaderSSAO.SetInt("gPosition", 0);
    shaderSSAO.SetInt("gNormal", 1);
    shaderSSAO.SetInt("texNoise", 2);
    shaderSSAOBlur.UseProgram();
    shaderSSAOBlur.SetInt("ssaoInput", 0);
    shaderLighting.UseProgram();
    shaderLighting.SetInt("gPosition", 0);
    shaderLighting.SetInt("gNormal", 1);
    shaderLighting.SetInt("gAlbedo", 2);
    shaderLighting.SetInt("ssao", 3);

    while (!glfwWindowShouldClose(window))
    {
        GLfloat currentTime = glfwGetTime();
        deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        glfwPollEvents();
        Move();
        SetViewport(window);

        glm::mat4 view = camera.GetViewMatrix();
        float fov = glm::radians(glm::clamp(camera.GetZoom() + 15.0f, 1.0f, 89.0f));
        glm::mat4 projection = glm::perspective(fov, static_cast<GLfloat>(screenWidth) / static_cast<GLfloat>(screenHeight), 0.1f, 100.0f);

        // geometry pass
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderGeometry(shaderGeometry, view, projection);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ssao pass
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        shaderSSAO.UseProgram();
        shaderSSAO.SetMatrix4fv("projection", projection);
        shaderSSAO.SetVec3fv("samples", 64, &ssaoKernel[0]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        RenderQuad();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // blur pass
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        shaderSSAOBlur.UseProgram();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
        RenderQuad();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // lighting combine
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shaderLighting.UseProgram();
        shaderLighting.SetVec3f("light.Position", glm::vec3(view * glm::vec4(LightPos, 1.0f)));
        shaderLighting.SetVec3f("light.Color", glm::vec3(1.4f, 1.3f, 1.2f));
        shaderLighting.SetVec3f("viewPos", camera.GetPosition());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedo);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
        RenderQuad();


        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode){
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if (key >= 0 && key < 1024){
        if (action == GLFW_PRESS){
            keys[key] = true;
        }else if (action == GLFW_RELEASE){
            keys[key] = false;
        }
    }
}

void Move(){
    if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP]){
        // forward
        camera.ProcessKeyboard(Camera_Movement::FORWARD, deltaTime );
    }
    if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT]){
        // left
        camera.ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
    }
    if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN]){
        // backward
        camera.ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
    }
    if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]){
        // right
        camera.ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
    }
}

void MouseCallback(GLFWwindow * window, double xPos, double yPos){
    if (firstMouse){
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
    }
    GLfloat xOffset = xPos - lastX;
    GLfloat yOffset = lastY - yPos;
    lastX = xPos;
    lastY = yPos;
    camera.ProcessMouse(xOffset, yOffset);
}

void ScrollCallback(GLFWwindow *window, double xOffset, double yOffset){
    camera.ProcessScroll(yOffset);
    };
