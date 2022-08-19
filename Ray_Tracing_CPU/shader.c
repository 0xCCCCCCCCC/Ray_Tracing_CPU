//
//  shader.c
//  Ray_Tracing
//
//  Created by YNK on 2022/7/25.
//

#include "shader.h"

char *readShaderFile(const char *fn)
{
    FILE *fp;
    long lSize;
    char *buffer;

    fp = fopen(fn, "r");
    if (!fp) {
        perror(fn);
        exit(1);
    }

    fseek(fp, 0L, SEEK_END);
    lSize = ftell(fp);
    rewind(fp);

    buffer = (char *)calloc(1, lSize + 1);
    if (!buffer) {
        fclose(fp);
        fputs("memory alloc fails", stderr);
        exit(1);
    }

    if (1 != fread(buffer, lSize, 1, fp)) {
        fclose(fp);
        free(buffer);
        fputs("entire read fails", stderr);
        exit(1);
    }

    fclose(fp);
    return buffer;
}

void shaderGenShader(const char *vertexShaderFile,
                     const char *fragmentShaderFile, unsigned int *vertexShader,
                     unsigned int *fragmentShader){
    *vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if(0 == *vertexShader){
        fprintf(stderr, "Create vertex shader failed");
        exit(1);
    }
    
    const GLchar* vertexShaderCode = readShaderFile(vertexShaderFile);
    const GLchar* vertexShaderCodeArray[1] = {vertexShaderCode};
    
    glShaderSource(*vertexShader, 1, vertexShaderCodeArray, NULL);
    
    glCompileShader(*vertexShader);
    
    GLint compileResult;
    glGetShaderiv(*vertexShader, GL_COMPILE_STATUS, &compileResult);
    if(0 == compileResult){
        GLint logLen;
        glGetShaderiv(*vertexShader, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 0)
        {
            char *log = (char *)malloc(logLen);
            GLsizei written;
            glGetShaderInfoLog(*vertexShader, logLen, &written, log);
            fprintf(stderr, "vertex shader compile log : \n");
            fprintf(stderr, "%s \n", log);
            free(log);
        }
    }
    
    *fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if(0 == *fragmentShader){
        fprintf(stderr, "Create fragment shader failed");
        exit(1);
    }
    
    const GLchar* fragmentShaderCode = readShaderFile(fragmentShaderFile);
    const GLchar* fragmentShaderCodeArray[1] = {fragmentShaderCode};
    
    glShaderSource(*fragmentShader, 1, fragmentShaderCodeArray, NULL);
    
    glCompileShader(*fragmentShader);
    
    glGetShaderiv(*fragmentShader, GL_COMPILE_STATUS, &compileResult);
    if(0 == compileResult){
        GLint logLen;
        glGetShaderiv(*fragmentShader, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 0)
        {
            char *log = (char *)malloc(logLen);
            GLsizei written;
            glGetShaderInfoLog(*fragmentShader, logLen, &written, log);
            fprintf(stderr, "fragment shader compile log : \n");
            fprintf(stderr, "%s \n", log);
            free(log);
        }
    }
}

void shaderAttachShader(unsigned int *programHandle, unsigned int *vertexShader,
                        unsigned int *fragmentShader){
    *programHandle = glCreateProgram();

    if (0 == *programHandle) {
        fprintf(stderr, "Create program failed.\n");
        exit(1);
    }

    glAttachShader(*programHandle, *vertexShader);
    glAttachShader(*programHandle, *fragmentShader);
    glLinkProgram(*programHandle);

    GLint linkStatus;
    glGetProgramiv(*programHandle, GL_LINK_STATUS, &linkStatus);
    if (GL_FALSE == linkStatus) {
        fprintf(stderr, "link shader program failed\n");
        GLint logLen;
        glGetProgramiv(*programHandle, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 0) {
            char *log = (char *)malloc(logLen);
            GLsizei written;
            glGetProgramInfoLog(*programHandle, logLen, &written, log);
            fprintf(stderr, "Program log : \n");
            fprintf(stderr, "%s \n", log);
        }
    }
}

void shaderDeleteShader(unsigned int *vertexShader, unsigned int *fragmentShader){
    glDeleteShader(*vertexShader);
    glDeleteShader(*fragmentShader);
}

Shader shaderCreate(const char *vertexShaderFile, const char *fragmentShaderFile){
    Shader shader = (Shader)calloc(1, sizeof(*shader));
    *shader = glCreateProgram();
    GLuint vertexShader, fragmentShader;
    shaderGenShader(vertexShaderFile, fragmentShaderFile, &vertexShader, &fragmentShader);
    shaderAttachShader(shader, &vertexShader, &fragmentShader);
    shaderDeleteShader(&vertexShader, &fragmentShader);
    return shader;
}

void shaderUse(Shader shader){
    glUseProgram(*shader);
}

void shaderDelete(Shader shader){
    free(shader);
}

//参考learnOpenGL
