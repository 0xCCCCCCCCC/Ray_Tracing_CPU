#version 330 core
#define PI 3.1415926

in vec3 screenColor;

out vec4 FragColor;

void main(){
    FragColor = vec4(screenColor.xyz, 1.0f);
}
