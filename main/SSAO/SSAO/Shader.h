#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#define SHADER 0x0001
#define PROGRAM 0x0002

typedef void(*CheckStatus)(GLuint, GLenum, GLint*);
typedef void(*ReadLog)(GLuint, GLsizei, GLsizei*, GLchar*);
#define CheckErrorAndLog(CheckStatus, ReadLog, obj, checkStatus) CheckStatus(obj, checkStatus, &success);if(!success) {GLchar infoLog[1024];ReadLog(obj, 1024, NULL, infoLog);std::cout<<"ERROR::SHADER::COMPILATION_FAILED\n" <<infoLog << std::endl; exit(-1);}

class Shader{
public:
    Shader(const char* vertexPath, const char* fragmentPath){
        shaderProgram = glCreateProgram();
        if (shaderProgram == 0){
            std::cout <<"ERROR Creating Shader Program!" << std::endl;
            exit(-1);
        }
        CompileShaderFromFile(vertexPath, GL_VERTEX_SHADER);
        CompileShaderFromFile(fragmentPath, GL_FRAGMENT_SHADER);
        LinkShaderProgram();
    }
    ~Shader(){
        glDeleteProgram(shaderProgram);
    }
    void UseProgram(){
        glUseProgram(shaderProgram);
    }
    void SetInt(const std::string &name, int value) const{
        glUniform1i(glGetUniformLocation(shaderProgram, name.c_str()), value);
    }
    void SetFloat(const std::string &name, float value) const {
        glUniform1f(glGetUniformLocation(shaderProgram, name.c_str()), value);
    }
    void SetVec2f(const std::string& name, glm::vec2 value){
        glUniform2f(glGetUniformLocation(shaderProgram, name.c_str()), value.x, value.y);
    }
    void SetVec3f(const std::string& name, glm::vec3 value){
        glUniform3f(glGetUniformLocation(shaderProgram, name.c_str()), value.x, value.y, value.z);
    }
    void SetVec3fv(const std::string& name, int count, const glm::vec3* value){
        glUniform3fv(glGetUniformLocation(shaderProgram, name.c_str()), count, glm::value_ptr(value[0]));
    }
    void SetMatrix4fv(const std::string& name, glm::mat4 matrix) const{
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
    }
private:
    GLuint shaderProgram;
    void CompileShaderFromFile(const char* path, GLenum shaderType){
        std::string code;
        std::ifstream shaderFile;
        shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try{
            shaderFile.open(path);
            std::stringstream shaderStream;
            shaderStream << shaderFile.rdbuf();
            shaderFile.close();
            code = shaderStream.str();
        } catch (std::ifstream::failure &e){
            std::cout << "ERROR::SHADER::FILE_NOT_READ" << e.what() << std::endl;
        }
        const GLchar* shaderCode = code.c_str();
        GLuint shader = glCreateShader(shaderType);
        glShaderSource(shader, 1, &shaderCode, NULL);
        glCompileShader(shader);
        CheckErrors(shader, SHADER, GL_COMPILE_STATUS);
        glAttachShader(shaderProgram, shader);
        glDeleteShader(shader);
    }
    void CheckErrors(GLuint obj, int type, int checkStatus){
        GLint success;
        if (type == SHADER) {
            CheckErrorAndLog(glGetShaderiv, glGetShaderInfoLog, obj, checkStatus);
        } else if (type == PROGRAM){
            CheckErrorAndLog(glGetProgramiv, glGetProgramInfoLog, obj, checkStatus);
        }
    }
    void LinkShaderProgram(){
        glLinkProgram(shaderProgram);
        CheckErrors(shaderProgram, PROGRAM, GL_LINK_STATUS);
#ifdef __WIN64
        glValidateProgram(shaderProgram);
        CheckErrors(shaderProgram, PROGRAM, GL_VALIDATE_STATUS);
#endif
    }
};
