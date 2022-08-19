//
//  main.cpp
//  Ray_Tracing_CPU
//
//  Created by YNK on 2022/7/26.
//

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm.hpp>

#include <vector>

#include "shader.h"
#include "frameworks.hpp"
#include "material_aux.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define PI 3.141592654

using std::vector;

void loadPixel(float fx, float fy, glm::vec3 fragColor);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);

vector<float> vertices;

//于scene类子函数renderScene中调用，用于将计算所得的像素点颜色与坐标加入vertices向量中
void loadPixel(float fx, float fy, glm::vec3 fragColor){
    vertices.push_back(fx);
    vertices.push_back(fy);
    vertices.push_back(.0f);
    vertices.push_back(fragColor.x);
    vertices.push_back(fragColor.y);
    vertices.push_back(fragColor.z);
}

int main(int argc, char * argv[]){
    //OpenGL初始化
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //Mac系统需要
    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
    
    //创建窗口
    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Ray Tracing CPU", NULL, NULL);
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    //glfwSetKeyCallback(window, key_call_back);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    //加载编译着色器
    //MARK: 若要使用相对路径，须在Product->Scheme->Edit Scheme->Run->Options->Use custom working directory中设置为项目路径
    Shader shader = shaderCreate("vertex_shader.vert", "fragment_shader.frag");
    
    //创建场景，在场景中添加数个球体，并执行光线追踪算法，计算各像素点颜色，存储于向量vertices中
    Scene mainScene;
    mainScene.setBackground(glm::vec3(.461f, .141f, .062f));
    mainScene.setLight(new Light(glm::vec3(.5f, 1.0f, .5f), glm::vec3(1.5f, 1.5f, 1.5f)));
    //mainScene.addLight(new Light(glm::vec3(.0f, .0f, 1.0f), glm::vec3(.8f, .8f, .8f)));
    mainScene.addSphere(new Sphere(glm::vec3(.2f, .0f, .0f), .1f, roughMaterial(blue)));
    mainScene.addSphere(new Sphere(glm::vec3(-.2f, .0f, .0f), .1f, roughMaterial(red)));
    mainScene.addSphere(new Sphere(glm::vec3(.0f, .0f, -.2f), .1f, roughMaterial(brown)));
    mainScene.addSphere(new Sphere(glm::vec3(.0f, .0f, .2f), .1f, defaultRefractive));
    mainScene.addSphere(new Sphere(glm::vec3(.0f, .2f, .0f), .1f, metalReflective));
    mainScene.addSphere(new Sphere(glm::vec3(.0f, -200.15f, .0f), 200.0f, defaultReflective));
    /*mainScene.addSphere(new Sphere(glm::vec3(.0f, -.06f, .8f), .04f, new Material(glm::vec3(0, .15, 0), 128)));
    mainScene.addSphere(new Sphere(glm::vec3(-.11f, -.04f, .6f), .06f, defaultRefractive));
    mainScene.addSphere(new Sphere(glm::vec3(-.25f, .05f, .2f), .15f, new Material(glm::vec3(.42f, .35f, .1f), 16)));
    mainScene.addSphere(new Sphere(glm::vec3(.2f, .02f, .2f), .12f, new Material(1.8f)));
    mainScene.addSphere(new Sphere(glm::vec3(.0f, .2f, -.6f), .3f, metalReflective));
    mainScene.addSphere(new Sphere(glm::vec3(.0f, -200.1f, .0f), 200.0f, defaultReflective));*/
    mainScene.rotateCamera(.125 * PI);
    vertices.clear();
    mainScene.renderScene(2 * WINDOW_WIDTH, 2 * WINDOW_HEIGHT, loadPixel);
     
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    //启动OpenGL窗口
    while (!glfwWindowShouldClose(window)) {
        glClearColor(.0f, .0f, .0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        processInput(window);

        shaderUse(shader);

        glDrawArrays(GL_POINTS, 0, (int)vertices.size()/6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    shaderDelete(shader);
    
    glfwTerminate();
    return 0;
    }

//窗口被拖拽时恢复原先大小
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

//按下键盘esc键退出
void processInput(GLFWwindow *window){
    //按下Esc键时关闭窗口
    if(glfwGetKey(window, GLFW_KEY_ESCAPE)==GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
