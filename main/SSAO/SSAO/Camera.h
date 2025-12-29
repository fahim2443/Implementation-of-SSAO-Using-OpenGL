#pragma once

#include <GL/glew.h>
#include "glm/glm.hpp"

const GLfloat          PITCH =   0.0f;
const GLfloat          YAW   = -90.0f;
const GLfloat          SPEED = 6.0f;
const GLfloat          SENSITIVITY = 0.25f;
const GLfloat          ZOOM = 60.0f;
enum class Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};
class Camera
{
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), GLfloat yaw = YAW, GLfloat pitch = PITCH):front(glm::vec3(0.0f, 0.0f, -1.0f)), speed(SPEED), sensitivity(SENSITIVITY), zoom(ZOOM)
    {
        this->position = position;
        this->worldUp = up;
        this->yaw = yaw;    //up and down
        this->pitch = pitch;   //left and right
        updateCameraVectors();
    }
    Camera(GLfloat posX, GLfloat posY, GLfloat posZ, GLfloat upX, GLfloat upY, GLfloat upZ, GLfloat yaw = YAW, GLfloat pitch = PITCH):front(glm::vec3(0.0f, 0.0f, -1.0f)), speed(SPEED), sensitivity(SENSITIVITY), zoom(ZOOM)
    {
        this->position = glm::vec3(posX, posY, posZ);
        this->worldUp = glm::vec3(upX, upY, upZ);
        this->yaw = yaw;
        this->pitch = pitch;
        updateCameraVectors();
    }
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(this->position, this->position + this->front, this->up);
    }
    
    GLfloat GetZoom(){
        return zoom;
    }
    
    void ProcessKeyboard(Camera_Movement direction, GLfloat deltaTime)
    {
        GLfloat velocity = speed * deltaTime;
        if (direction == Camera_Movement::FORWARD){
            this->position += this->front * velocity;
        }
        if (direction == Camera_Movement::BACKWARD){
            this->position -= this->front * velocity;
        }
        if (direction == Camera_Movement::RIGHT){
            this->position += this->right * velocity;
        }
        if (direction == Camera_Movement::LEFT){
            this->position -= this->right * velocity;
        }
    }
    void ProcessMouse(GLfloat xOffset, GLfloat yOffset){
        xOffset *= sensitivity;
        yOffset *= sensitivity;
        this->yaw += xOffset;
        this->pitch += yOffset;
        updateCameraVectors();
    }
    void ProcessScroll(GLfloat yOffset){
        zoom += yOffset;
    }
    glm::vec3 GetPosition(){
        return position;
    }
private:
    GLfloat yaw, pitch;
    GLfloat speed, sensitivity;
    GLfloat zoom;
    glm::vec3 position, front, up, right, worldUp;
    void updateCameraVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(this->pitch)) * cos(glm::radians(this->yaw));
        front.y = sin(glm::radians(this->pitch));
        front.z = cos(glm::radians(this->pitch)) * sin(glm::radians(this->yaw));
        this->front = glm::normalize(front);
        this->right = glm::normalize(glm::cross(this->front, this->worldUp));
        this->up = glm::normalize(glm::cross(this->right, this->front));
    }
};
