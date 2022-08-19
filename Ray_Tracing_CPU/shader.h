//
//  shader.h
//  Ray_Tracing
//
//  Created by YNK on 2022/7/25.
//

#ifndef shader_h
#define shader_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef Shader
#undef Shader
#endif

#define Shader GLuint*

char *readShaderFile(const char *fn);

void shaderGenShader(const char *vertexShaderFile,
                     const char *fragmentShaderFile, unsigned int *vertexShader,
                     unsigned int *fragmentShader);

void shaderAttachShader(unsigned int *programHandle, unsigned int *vertexShader,
                        unsigned int *fragmentShader);

void shaderDeleteShader(unsigned int *vertexShader, unsigned int *fragmentShader);

Shader shaderCreate(const char *vertexShaderFile, const char *fragmentShaderFile);

void shaderUse(Shader shader);

void shaderDelete(Shader shader);

#ifdef __cplusplus
}
#endif

#endif /* shader_h */
