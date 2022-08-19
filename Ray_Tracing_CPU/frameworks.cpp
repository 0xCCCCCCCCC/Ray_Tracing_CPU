//
//  frameworks.cpp
//  Ray_Tracing_CPU
//
//  Created by YNK on 2022/7/26.
//
#include <iostream>

#include "frameworks.hpp"

using std::vector;

// 更新相机参数
void Camera::updateCameraVectors(void){
    front = - glm::normalize(position);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::rotate(float radian){
    glm::vec2 rotated(position.x * cos(radian) - position.z * sin(radian),
                  position.x * sin(radian) + position.z * cos(radian));
    this->position = glm::vec3(rotated.x, position.y, rotated.y);
    updateCameraVectors();
}

// 计算观察光线
Ray Camera::getRay(float viewx, float viewy, float fov){
    glm::vec3 viewDir = front + right * (2 * (viewx + .5f) / fov - 1) +
        up * (2 * (viewy + .5f) / fov - 1) - position;
    Ray viewRay(position, viewDir);
    return viewRay;
}

// 光线与球相交
float Intersect(const Sphere &sphere, const Ray &ray, glm::vec3 *hitNorm, glm::vec3 *hitPos){
    // 将光线的表达式p=e+t·d带入球的方程(p-c)^2-R^2=0中进行求解
    glm::vec3 dist = ray.ori - sphere.center;
    float a = glm::dot(ray.dir, ray.dir);
    float b = 2.0f * glm::dot(ray.dir, dist);
    float c = glm::dot(dist, dist) - sphere.radius * sphere.radius;
    float Delta = b * b - 4.0f * a * c;
    // 若判别式小于0，则光线与球未相交，返回-1
    if (Delta < 0) return -1;
    Delta = sqrtf(Delta);
    float t1 = (-b + Delta) / 2.0f / a;
    float t2 = (-b - Delta) / 2.0f / a;
    // 获得方程的2解，较小的t2为光线进入球的位置，较大的t1为光线离开球的位置
    // 若2解均小于0，即球位于光线后面，则返回-1
    if (t1 <= 0) return -1;
    // 光线与球相交于e+t·d处，记录交点坐标、交点法向量
    float t = t2 > 0 ? t2 : t1;
    if(hitPos != NULL)
        *hitPos = ray.ori + t * ray.dir;
    if(hitNorm != NULL)
        *hitNorm = (*hitPos - sphere.center) / sphere.radius;
    return t;
}

// 场景默认初始化
Scene::Scene(){
    this->background_color = glm::vec3(.5f, .5f, .5f);
    this->light.push_back(new Light(glm::vec3(.0f, 1.0f, 1.0f), glm::vec3(2.0f, 2.0f, 2.0f)));
    this->camera = Camera(glm::vec3(.0f, .0f, 1.5f));
}

// 设置光源
void Scene::setLight(Light *light){
    this->light.clear();
    this->light.push_back(light);
}

void Scene::addLight(Light *light){
    this->light.push_back(light);
}

// 在场景中添加球体
void Scene::addSphere(Sphere *sphere){
    this->objects.push_back(sphere);
}

// 设置环境泛光、背景色
void Scene::setBackground(glm::vec3 bgcolor){
    this->background_color = bgcolor;
}

// 检测与光线相交的第一个物体（球体），若无相交物体则返回false；
// 若有相交物体，则返回true，并记录与相交的最近物体在场景向量中的序号，与交点坐标和交点法向量
bool Scene::SceneIntersect(const Ray &ray,
                    int *hitObject, glm::vec3 *hitNorm, glm::vec3 *hitPos){
    if (objects.size() < 1) return false;
    
    bool hit = false;
    float first_hit = -1;
    int first_hit_obj = -1;
    glm::vec3 first_hit_pos;
    glm::vec3 first_hit_norm;
    
    // 对场景中的每一个物体求交
    for (int i = 0; i < objects.size(); ++i) {
        glm::vec3 _hit_pos;
        glm::vec3 _hit_norm;
        float t = Intersect(*objects[i], ray, &_hit_norm, &_hit_pos);
        // 若此时交点的t值小于先前记录的最小t值，则记录此t值为最小t值，并记录此球的序号与交点的相应信息
        if (t > 0 && t > TRACE_MIN_T && (first_hit < 0 || t < first_hit)) {
            hit = true;
            first_hit_obj = i;
            first_hit = t;
            first_hit_pos = _hit_pos;
            first_hit_norm = _hit_norm;
        }
    }
    
    if(hit){
        if(hitObject != NULL) *hitObject = first_hit_obj;
        if(hitPos != NULL) *hitPos = first_hit_pos;
        if(hitNorm != NULL) *hitNorm = first_hit_norm;
    }
    return hit;
}

// 计算阴影时，测试阴影光线是否与物体相交
bool Scene::ShadowRayIntersect(const Ray &shadow_ray){
    if (objects.size() < 1) return false;
    
    for (int i = 0; i < objects.size(); ++i) {
        float t = Intersect(*objects[i], shadow_ray, NULL, NULL);
        if (t > 0 && t > TRACE_MIN_T)
            return true;
    }
    
    return false;
}

glm::vec3 Scene::Trace(const Ray& ray, int depth){
    // 递归次数达到上限，返回环境泛光值
    if(depth > TRACE_MAX_DEPTH)
        return background_color;
    int hit_index;
    glm::vec3 hit_pos, hit_norm;
    // 光线未击中任何物体，返回背景色
    if(!SceneIntersect(ray, &hit_index, &hit_norm, &hit_pos))
        return background_color;
    
    glm::vec3 c;
    Material hit_material = *(objects[hit_index]->material);
    
    if(hit_material.type == ROUGH){
        // 计算阴影
        // 使用phong模型计算颜色
        // 环境光
        c = background_color * hit_material.objColor;
        // 测试是否为阴影
        for (int i = 0; i < light.size(); ++i) {
            Ray shadow_ray(hit_pos + EPSILON * light[i]->dir, light[i]->dir);
            if(!ShadowRayIntersect(shadow_ray)){
                // 漫反射
                float diff = fmax(glm::dot(light[i]->dir, hit_norm), .0f);
                glm::vec3 diffuse = diff * light[i]->color * hit_material.objColor;
                c += diffuse;
                // 镜面高光
                glm::vec3 view_dir = glm::normalize(ray.ori - hit_pos);
                glm::vec3 reflect_dir = glm::reflect(-light[i]->dir, hit_norm);
                float Delta = glm::dot(view_dir, reflect_dir);
                if(Delta>0){
                    float specular = powf(Delta, hit_material.shininess);
                    c += specular;
                }
            }
        }
        return c;
    }

    //if(hit_material.type == REFLECTIVE)
    // 计算反射光线方向（入射光线方向与法线方向点乘结果为负，故使用减法）
    glm::vec3 reflectDir = ray.dir - 2 * glm::dot(ray.dir, hit_norm) * hit_norm;
    Ray reflectRay = Ray(hit_pos + EPSILON * reflectDir, glm::normalize(reflectDir));
    // 使用菲涅尔方程的Shlick近似方程计算反射光线颜色
    glm::vec3 cs;
    float cosTheta = - glm::dot(glm::normalize(ray.dir), hit_norm);
    float r0 = 0;
    if(hit_material.type == REFLECTIVE)
        cs = hit_material.objColor;
    else if (hit_material.type == REFRACTIVE){
        r0 = pow((hit_material.n - 1) / (hit_material.n + 1), 2);
        glm::vec3 R0(r0, r0, r0);
        cs = R0 + (glm::vec3(1.0f, 1.0f, 1.0f) - R0) * pow(1.0f - cosTheta, 5.0f);
        //cs = hit_material.R0 + (glm::vec3(1.0f, 1.0f, 1.0f) - hit_material.R0) * pow(1.0f - cosTheta, 5.0f);
    }
    // 递归调用光线追踪函数
    c = cs * Trace(reflectRay, depth + 1);
    
    if(objects[hit_index]->material->type == REFRACTIVE){
        float cos2Phi = 1 - (1 - cosTheta * cosTheta) / (hit_material.n * hit_material.n);
        if(cos2Phi >= 0){
            glm::vec3 refractDir = (ray.dir - hit_norm * glm::dot(ray.dir, hit_norm)) / hit_material.n
                - hit_norm *sqrtf(cos2Phi);
            Ray refractRay = Ray(hit_pos - EPSILON * refractDir, glm::normalize(refractDir));
            c += (glm::vec3(1.0f, 1.0f, 1.0f) - cs) * Trace(refractRay, depth + 1);
        }
    }
    
    return c;
}

// 计算场景中每一个像素点的颜色，调用Trace函数对场景中每一个像素点进行光线追踪，得到像素点颜色
void Scene::renderScene(float viewWidth, float viewHeight, void(*handleFunc) (float, float, glm::vec3)) {
    float fov = fmax(viewWidth, viewHeight);
    for (int viewx = 0; viewx < fov; viewx++) {
        for (int viewy = 0; viewy < fov; viewy++) {
            Ray viewRay = camera.getRay(viewx, viewy, fov);
            glm::vec3 pixelColor = Trace(viewRay);
            // 计算视野中各像素点的位置，并调用函数renderFunc将像素点的坐标及颜色插入主程序的端点向量中
            glm::vec2 cameraCoord = glm::vec2(1, 0) * (2 * (viewx + .5f) / fov - 1) +
                            glm::vec2(0, 1) * (2 * (viewy + .5f) / fov - 1);
            float renderX = fov == viewWidth ? cameraCoord.x : cameraCoord.x / viewWidth * viewHeight;
            float renderY = fov == viewHeight ? cameraCoord.y : cameraCoord.y / viewHeight * viewWidth;
            (*handleFunc) (renderX, renderY, pixelColor);
        }
    }
}

// 在xOz平面上绕坐标原点旋转相机
void Scene::rotateCamera(float radian){
    this->camera.rotate(radian);
}
