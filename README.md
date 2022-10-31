# 一、前言
这个项目是数字媒体技术专业必修课《计算机图形学》的课程设计之一，当时大二学这门课的时候由于比较菜没有写出来，现在大三比较空了就抽时间（摸鱼）重新写了一下，花了一个星期看了一堆大佬的文章，缝合了一堆大佬的代码，总算是能够实现了。由于我依旧是这么的菜，如果写的内容出现问题了也请见谅；

下面对课程设计要求、环境配置方法、最终实现效果等进行说明，直接goto代码可以跳过： 

>
> **注：** 项目参考了计算机图形学教材《计算机图形学》Peter Shirley著（第2版）和一些老师给的可课件、代码和伪代码；
>
> **实验内容：** 基于C++（也可选择其它编程语言，但需要在实现中体现面向对象的思想）实现完整的含递归调用的光线跟踪算法；
>
> **场景要求：** 至少包含两个球体；
>
> **实验效果：** 要求实现Phong局部光照明、物体之间的镜面反射、阴影这三种效果；如果想拿更好的成绩，可以加入折射的效果；
>
> **实验环境：** Macbook Pro 2019, intel i9 处理器；系统：macOS Monterey 12.5；使用IDE: Xcode v13.4.1；

### 最终效果图：

![最终效果图](https://img-blog.csdnimg.cn/ac02286076dd4e239e1dd65ca5f45633.png '最终效果图')

（如图所示，场景中包含6个球体，分别为红色、黄色、蓝色的3个粗糙材质球体，上方的反射材质球体，右前方的一个折射材料球体以及下方充当场景地面的反射材质大球体，满足实验场景要求）

### OpenGL环境配置：

1. Xcode配置OpenGL开发环境：

[MAC下的XCode配置OpenGL环境](https://blog.csdn.net/m0_46583678/article/details/112538989)

2. 下载第三方库glm：

为了方便之后的向量运算，需要使用glm(OpenGl Mathematics)库；

[github地址](https://github.com/g-truc/glm) 下载解压后拖进项目路径，然后在在项目设置页面中的Search Paths->User Header Search Paths中添加glm文件夹的路径，使用时只需#include "glm.hpp" 即可。

### 项目地址：

[项目地址](https://github.com/0xCCCCCCCCC/Ray_Tracing_CPU) （完整Xcode项目，运行结果与上图一致）



# 二、项目实现与说明

本实验要求实现含有完整的递归调用的光线追踪算法，由于GLSL不支持递归调用，故使用C++在CPU中运行包含递归调用的光线追踪算法，得出每一个像素的颜色值进行绘制；

## 1. 数据结构设计

包括光线、材质、物体（球体）、相机、场景等，以便实现光线追踪算法的运算；

### 1.1 光线 Ray

光线是光线追踪算法中最基本的数据结构，在光线追踪算法中，我们追踪从相机射向场景上每一个像素点的光线，检测光线与场景中物体的碰撞、反射、折射，以计算出该像素点的颜色。

根据书本上光线的表达式：，取向量 $\vec{d} = \vec{s} - \vec{e}$，即光线的方向，则可以将任意光线表达为由光源e出发沿着方向d前进的射线，表达式为，故在项目中定义光线结构体 Ray如下：

```cpp
struct Ray{
    glm::vec3 ori;   //光线源点
    glm::vec3 dir;   //光线方向
    
    Ray(glm::vec3 ori, glm::vec3 dir) : ori(ori), dir(dir) {}    //光线结构体初始化
};
```

---

### 1.2 材质 Material

在本项目的实验要求中，要求实现反射、折射效果，故定义材质这一数据结构以存储场景中物体的材质类型以及相关参数。

材质分为3种类型，即仅进行Phong光照模型计算的粗糙材质、进行反射计算的反射材质和进行反射与折射计算的折射材质，分别由定义的宏ROUGH, REFLECTIVE, REFRACTIVE表示；

粗糙类型的材质的参数有用于Phong模型计算的 **物体颜色objColor** 与 **高光指数shininess**；全反射材质的参数有 **反射系数R0** ；折射材质的参数有 **折射率**n ；三种类型的材质分别有对应的构造函数重载。项目中材质结构体Material定义如下：

```cpp
struct Material{
    int type;
    
    //Phong模型参数
    glm::vec3 objColor;
    float shininess;
    
    //折射计算参数：材料折射率
    float n;
    
    Material(int type) : type(type) {}
    // 粗糙材质
    Material(glm::vec3 color, float shininess):type(ROUGH),
        objColor(color), shininess(shininess), R0(glm::vec3(.0f, .0f, .0f)){}
    // 反射材质
    Material(glm::vec3 R0) : type(REFLECTIVE), R0(R0) {}
    // 折射材质
    Material(float n) : type(REFRACTIVE), n(n) {float r0 = pow((n - 1) / (n + 1), 2); R0 = glm::vec3(r0, r0, r0);}
};
```

---

### 1.3 光照 Light

光照数据结构为场景中的光照，在本项目中为平行光，故只需定义光的方向、颜色和光照强度，在本项目中把光照颜色和强度定义在一起了，所以本项目中的光照结构体只有两个参数。光照结构体Light定义如下：

```cpp
struct Light{
    glm::vec3 dir;     //光照方向
    glm::vec3 color;   //光照强度*颜色
    
    Light(glm::vec3 dir, glm::vec3 color) : dir(glm::normalize(dir)), color(color) {}
};
```

---

### 1.4 相机 Camera

本项目中的相机类参考了官方教程中给出的相机类，并针对本项目进行了简化。本项目中相机指向场景的坐标原点，其主要函数为构造函数和观察光线函数getRay，分别实现了通过相机位置构造相机对象、返回观察光线的功能。相机类Camera的定义如下：

```cpp
class Camera{
public:
    // 相机位置、旋转等参数
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    // 相机构造函数
    Camera(glm::vec3 position = glm::vec3(.0f, .0f, 1.0f))
    : front(-glm::normalize(position)), position(position), worldUp(glm::vec3(.0f, 1.0f, .0f)) {updateCameraVectors();}
    // 相机在xOz平面围绕坐标原点进行旋转    
    void rotate(float radian);
    // 返回观察光线
    Ray getRay(float viewx, float viewy, float fov);
    
private:
    // 调整相机状态
    void updateCameraVectors(void);
};
```

---

### 1.5 球体Sphere

球体为本项目中用于光线求交等计算的物体，球体包含 **球心** 、 **半径** 、 **材质** 等基本参数；并且有 **求交函数** ，用于计算光线是否与球体相交，并记录交点的坐标、法向量参数，求交函数将在算法实现部分进行详细说明。球体类Sphere的定义如下：

```cpp
class Sphere{
    // 球体参数：球心、半径、材质
    glm::vec3 center;
    float radius;
    
public:
    Material *material;
    // 球体构造函数
    Sphere(glm::vec3 center, float radius, Material *material)
        : center(center), radius(radius), material(material) {}
    // 友元函数：判断光线与球体碰撞
    friend float Intersect(const Sphere &sphere, const Ray &ray, glm::vec3 *hitNorm, glm::vec3 *hitPos);
};
```

---

### 1.6 场景Scene

场景类存放以上定义的球体、光照、摄像机等类，也是光线追踪算法进行的地方。场景类包含球体可变数组、光照可变数组（场景中可添加多个光照）、相机等成员，以及背景颜色属性；场景类有首次相交、阴影检测、光线追踪等函数与方法，用于场景中进行光线追踪计算（以上函数与方法的具体实现将在算法实现中进行详细说明）。场景类Scene的定义如下：

```cpp
class Scene{
    // 场景类基本成员：球体、光照、相机，以及背景颜色
    std::vector<Sphere*> objects;
    std::vector<Light*> light;
    Camera camera;
    glm::vec3 background_color;
    
public:
    Scene();
    // 设置场景参数
    void setLight(Light *light);
    void addSphere(Sphere *sphere);
    void setBackground(glm::vec3 bgcolor = glm::vec3(.0f, .0f, .0f));
    // 检测光线与场景中物体的碰撞
    bool SceneIntersect(const Ray &ray,
                        int *hitObject, glm::vec3 *hitNorm, glm::vec3 *hitPos);
    // 阴影检测
    bool ShadowRayIntersect(const Ray &shadow_ray);
    // 光线追踪
    glm::vec3 Trace(const Ray& ray, int depth = 0);
    // 对整个场景进行光线追踪算法    
    void renderScene(float viewWidth, float viewHeight, void(*handleFunc) (float, float, glm::vec3));
    // 旋转场景中的相机
    void rotateCamera(float radian);
};
```

---

## 2. 算法实现

### 2.1 光线追踪算法原理与步骤

进行光线追踪算法的实现之前，我们需要了解光线追踪算法的原理。以下将依据教材和相关资料对光线追踪算法的原理及其步骤进行简要的说明：

![图2 光线追踪原理示意图（图源维基百科同名词条）](https://img-blog.csdnimg.cn/8a26422ad9e84e9c8e51d25146c5c4b9.png '图2 光线追踪原理示意图（图源维基百科同名词条）')   
图2 光线追踪原理示意图（图源维基百科同名词条）

*注：上图中相关参数名与教材有一定出入，这里以教材为标准。*

如上图所示，光线追踪算法总体分为以下几步：

1. 计算观察光线；

2. 计算光线与物体相交；

3. 进行阴影计算/Phong光照模型颜色计算/反射、折射光线计算；

4. 对于反射、折射光线递归调用光线追踪算法。

在本项目中，通过定义多个函数、方法，并调用上述定义的类、结构体中的属性值进行计算，以实现上述光线追踪算法的步骤，下面对这些函数与方法的原理与具体实现进行解释说明：

---

### 2.2 计算观察光线

观察光线，依据教材上的解释，即：

>从眼睛 $\vec{e}$ 到屏幕上一点 $\vec{s}$ 的三维参数直线： $\vec{p}(t) = \vec{e} + t(\vec{s} - \vec{e})$   

将屏幕上的待渲染的像素点坐标(u, v)由屏幕坐标系uvw投影到场景坐标系xyz，得到转化后的场景坐标以构造观察光线，教材上对于这一转化的描述如下（其中l, b分别为屏幕坐标原点的u, v坐标投影至场景坐标系中的值，r-l, t-b分别为屏幕坐标水平、垂直正方向投影至场景坐标系中的向量）：

>$u_{s}=l+(r-l)\frac{i+0.5}{n_{x}}$
>
>$v_{s}=b+(t-b)\frac{j+0.5}{n_{y}}$

将上述转化在C++中实现为相机类的成员函数getRay。该函数的参数分别为屏幕坐标上指定像素点的坐标，以及观察平面的边长（即屏幕长宽中较大的一项），函数将返回对应的观察光线。该函数的具体代码如下：

```cpp
Ray Camera::getRay(float viewx, float viewy, float fov){
    glm::vec3 viewDir = front + right * (2 * (viewx + .5f) / fov - 1) +
        up * (2 * (viewy + .5f) / fov - 1) - position;
    Ray viewRay(position, viewDir);
    return viewRay;
}
```

---

### 2.3 光线与物体（球体）相交

依据教材描述，光线 $\vec{p}(t)=\vec{e}+t\vec{d}$ 与球 $f(\vec{p})=0$ 交点满足以下等式：

> $f(\vec{p}(t))=0\: \Leftrightarrow  f(\vec{e}+t\vec{d})=0$

由于球心为、半径为R的球体可表示为：

> $(\vec{p}-\vec{c})\cdot (\vec{p}-\vec{c})-R^{2}=0$

将光线带入上述表达式，可得：

> $(\vec{e}+t\vec{d}-\vec{c})\cdot (\vec{e}+t\vec{d}-\vec{c})-R^{2}=0\: \Leftrightarrow
(\vec{d}\cdot \vec{d})t^{2}+2\vec{d}\cdot(\vec{e}-\vec{c})t+(\vec{e}-\vec{c})\cdot(\vec{e}-\vec{c})-R^{2}=0$

以上方程为一元二次方程，容易进行求解。   
若无实数解，则光线与球没有交点；若两解均小于0，则该球在相机视野的后方与光线相交，剔除该解；   
若两解相同且均大于0，则光线与球相切，仅有一交点；   
若两解不同且均大于0，则 **较小** 的一解为光线 **进入球** 的位置， **较大** 的一解为光线 **离开球** 的位置。

在项目中通过球体Sphere类的友元函数Intersect对光线与球的相交进行判断。   
该函数的4个参数分别为求交的球体、光线，用于存储交点法向量、交点坐标的3维向量指针，函数返回光线与球体交点处的t值，若无交点则返回-1；并存储交点的法向量、坐标数据。该函数的代码实现如下：

```cpp
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
```

---

## 2.4 光线追踪算法的实现

本项目依据教材10.4-10.7章节的光线追踪程序部分内容实现光线追踪算法程序。光线追踪算法由多个函数实现，主要的包含递归调用的光线追踪函数为场景类Scene的成员函数Trace，该函数包含光线求交、反射、折射、Phong光照模型等计算，实现了完整的光线追踪算法。下面将分多个部分对本项目的光线追踪算法及其实现进行解释说明：

### 2.4.1 遮挡剔除

光线追踪程序首先需要找到场景中首个与该观察光线相交的物体，以便进行后续的颜色计算。在场景中，可能有多个物体与一条观察光线相交，我们需要找到场景中与光线交点t值最小(距离光线源点最近)的物体，在该物体进行后续的颜色、反射折射等计算。

![图3 遮挡剔除示意图（图源维基百科光线追踪词条）](https://img-blog.csdnimg.cn/ba61e348c5524ec99ce6968e50109341.png '图3 遮挡剔除示意图（图源维基百科光线追踪词条）')   
图3 遮挡剔除示意图（图源维基百科光线追踪词条）

教材中对于该算法的描述为以下伪代码：


>hit = false   
>**for** each object ***o*** **do**   
>&emsp;&emsp; **if** (object is hit at ray parameter t and  $t\epsilon [t_{0}, t_{1}]$  **then**  
>&emsp;&emsp;&emsp;&emsp;hit = true   
>&emsp;&emsp;&emsp;&emsp;hitobject = ***o***   
>&emsp;&emsp;&emsp;&emsp; $t_{1}$ = t   
>
>**return** hit   


在本项目中，该算法由场景类Scene的成员函数SceneIntersect实现，该函数判断指定观察光线是否与场景中物体相交，若相交，则存储与光线 **首个相交的物体** 的数组 **编号** 、 **法向量** 与 **交点坐标** 。该函数的代码实现如下：

```cpp
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
```

---

### 2.4.2 阴影

根据生活常识，若一个物体在光线方向上被其他物体遮挡，则会落入阴影中。   
即从物体表面的一个点向光源方向发出一条“阴影光线” $\vec{p}+t\vec{l}$ ，若该阴影光线与其他物体相交（即 $t\epsilon [0, \infty )$ ），则该点在阴影中；   
若阴影光线不与任何物体相交，则该点不在阴影中，对该点运用Phong光照模型进行颜色计算。   
由于数值的不确定性，阴影光线可能与其源点所在平面相交，故使t在范围 $t\epsilon [\varepsilon , \infty )$ 内进行检测（ $\varepsilon$ 为一个极小的正值）。

![图4 阴影示意图（图源维基百科）](https://img-blog.csdnimg.cn/a289feb1754e44e48d3043d7ddbbef58.png '图4 阴影示意图（图源维基百科）')   
图4 阴影示意图（图源维基百科）

教材上的阴影部分伪代码如下：

>**function** raycolor(ray $\vec{e}+t\vec{d}$ , real $t_{0}$, real $t_{1}$)   
>hit-record rec, srec   
>**if** (scene->hit($\vec{e}+t\vec{d}$, $t_{0}$, $t_{1}$, rec)) **then**&emsp;&emsp;&emsp;//测试是否与场景中物体相交   
>&emsp;&emsp;$\vec{p}=\vec{e}+rec.t\vec{d}$&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;//构造阴影光线   
>&emsp;&emsp;color c = $rec.c_{r}, \, rec.c_{a}$  &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;//Phong模型环境泛光   
>&emsp;&emsp;**if** (not scene->hit($\vec{p}+s\vec{l}$, $\varepsilon$, $\infin$, srec)) **then** &emsp;//若不在阴影内   
>&emsp;&emsp;&emsp;&emsp;vec3 $\vec{h}$ = normalized(normalized($\vec{l}$) + normalized(- $\vec{d}$))   
>&emsp;&emsp;&emsp;&emsp;c = c + Phong(rec, $\vec{h}$) &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;//通过Phong模型计算颜色   
>&emsp;&emsp;return c   
> else   
>&emsp;&emsp;return background-color &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;//若无相交，则返回背景色   

上述伪代码中的Phong(rec, $\vec{h}$)表示：若点不在阴影内，则调用Phong光照模型计算该点的颜色。Phong模型包含环境泛光、漫反射、镜面高光，是一种较为逼真的着色方法，下面对Phong模型的原理与实现进行简要说明： 

![图5 Phong光照模型](https://img-blog.csdnimg.cn/36f2790093b24b88a1605c81c0ac7410.png '图5 Phong光照模型')   
图5 Phong光照模型

环境泛光为物体处在场景中受周围反射光线影响的颜色，在Phong模型中被表示为环境光颜色与物体颜色的数值乘积，场景中所有物体均受到环境泛光的影响；

漫反射指粗糙表面物体向四处反射入射光的现象，在Phong模型中，漫反射仅与入射光的角度有关，与观察者位置等因素无关，在教材中，漫反射被表示为光照颜色乘以光照方向与交点法向量的点积，即：

>$c = c_{l}\:max(\vec{n}\cdot\vec{l},\:0)$

镜面高光为在光滑物体表面的高光，镜面高光影响因素有观察者的观察光线与反射光的 **夹角** 和 **高光指数**；   
观察者的观察光线与反射光线夹角 **越小** ，高光 **越亮** ，反之亦然，在Phong模型中使用光照方向与观察光线方向间的半角向量与交点法向量的点乘表示观察光线与反射光线的夹角；   
**高光指数p** 为该材料上高光的衰减系数，与不同材料有关，p值越大高光面积越小。故完整的镜面高光可表示为：

>$c = c_{l}(\vec{h}\cdot\vec{n})^{p}$

教材中完整的Phong模型伪代码为：

>$c=c_{r}(c_{a}+c_{l}\: max(0, \vec{n}\cdot \vec{l}))+c_{l}(\vec{h}\cdot \vec{n})^{p}$

以下为本项目中阴影检测函数ShadowRayIntersect，该函数为简化的碰撞函数，仅返回阴影光线是否与物体相交：

```cpp
bool Scene::ShadowRayIntersect(const Ray &shadow_ray){
    if (objects.size() < 1) return false;
    for (int i = 0; i < objects.size(); ++i) {
        float t = Intersect(*objects[i], shadow_ray, NULL, NULL);
        if (t > 0 && t > TRACE_MIN_T)
            return true;
    }
    return false;
}
```

本项目中对粗糙材质的球体进行阴影检测与Phong光照模型计算，在程序中的实现如下：

```cpp
if(hit_material.type == ROUGH){
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
```

以下为实现阴影检测与Phong光照模型后的场景渲染效果：

![图6 阴影检测与Phong模型](https://img-blog.csdnimg.cn/d5e35cc933e44226bb59c6a557fec846.png '图6 阴影检测与Phong模型')   
图6 阴影检测与Phong模型

---

### 2.4.3 镜面反射

![图7 镜面反射原理](https://img-blog.csdnimg.cn/636118aa0a264a7089de3a94519e2bc0.png 图7 镜面反射原理')   
图7 镜面反射原理

镜面反射原理非常简单，在已知入射光线方向与法向量的情况下，反射光线可被表示为：

>$\vec{r}=\vec{d}+2(\vec{d}\cdot\vec{n})\vec{n}$

对反射光线递归调用光线追踪函数，以检测与反射光线相交的物体，计算反射光线的颜色；   
由于场景中可能存在多个反射物体，导致递归调用不能及时终止、使算法效率低下甚至进入无限递归，在递归调用光线追踪算法时，设置最大递归深度，每次进行递归调用时递归深度+1，若某次递归调用时的递归深度高于最大递归深度，则递归调用终止；  

最后，与阴影检测类似，为了不使反射光线与碰撞点所在平面相交，将反射光线碰撞检测t值的范围设置为$[\varepsilon, \:\infty )$。   

由于在现实中不同材质对光线的反射效果不同，例如水的反射效果不如钢材、金反射黄色光线的效果高于其他颜色的光，故使用球体材质的材质颜色（objColor）属性值以表示当前材质对光线的反射效果。

上述镜面反射有关算法的伪代码如下：

>$color\:c = c+c_{s}raycolor(\vec{p}+s\vec{r},\varepsilon, \infty,d+1)$

在本项目中，镜面反射相关功能实现如下（最大递归深度取值为5）：

```cpp
// 计算反射光线方向（入射光线方向与法线方向点乘结果为负，故使用减法）
glm::vec3 reflectDir = ray.dir - 2 * glm::dot(ray.dir, hit_norm) * hit_norm;
Ray reflectRay = Ray(hit_pos + EPSILON * reflectDir, glm::normalize(reflectDir));
glm::vec3 cs = hit_material.objColor;
// 递归调用光线追踪函数
c = cs * Trace(reflectRay, depth + 1);
```

实现阴影、Phong模型与镜面反射后的渲染效果如下：

![图8 镜面反射效果](https://img-blog.csdnimg.cn/1e3a1d5651514efeb02b58c3180cd012.png '图8 镜面反射效果')   
图8 镜面反射效果

上图场景中将底部与后方物体材质设置为反射材质，可以在这两个物体表面看到其他物体的镜像。

---

### 2.4.4 折射

![图9 折射定律（图源教材）](https://img-blog.csdnimg.cn/24595ab0d7994f92addc59f676ce7fa2.png '图9 折射定律（图源教材）')   
图9 折射定律（图源教材）

折射光线的方向可以使用折射定律（Snell法则）求得。取空气的折射率$n_{0}=1$，折射定律在本项目中可表示为：$sin\theta=n_{1}sin\phi$，经过一系列化简可以得出折射光线的方向可表示为：

>$\vec{t}=\frac{n(\vec{d}+\vec{n}cos\theta)}{n_{1}}-\vec{n}cos\phi$

根据 **菲涅尔公式** 的Shlick近似可以计算出折射材质折射光线和反射光线的颜色（原理很复杂，反正会用就行），该公式表示如下：

>$R(\theta)=R_{0}+(1-R_{0})(1-cos\:\theta)^{5}$

其中的为法线上的反射系数，在折射材质中可表示为：

>$R_{0}=(\frac{n_{1}-1}{n_{1}+1})^{2}$

根据以上公式，折射材质的颜色可表示为：

>$R\:color(\vec{p}+\varepsilon\vec{r})+(1-R)\:color(\vec{p}+\varepsilon\vec{t})$

在反射计算中添加折射材质的反射计算，其cs值即R0通过其折射率n值计算得到；   
添加了折射材质的计算后，本项目中光线追踪算法的反射与折射部分如下：

```cpp
// 计算反射光线方向（入射光线方向与法线方向点乘结果为负，故使用减法）
glm::vec3 reflectDir = ray.dir - 2 * glm::dot(ray.dir, hit_norm) * hit_norm;
Ray reflectRay = Ray(hit_pos + EPSILON * reflectDir, glm::normalize(reflectDir));
// 使用菲涅尔方程的Shlick近似方程计算反射光线颜色
glm::vec3 cs;
float cosTheta = - glm::dot(glm::normalize(ray.dir), hit_norm);
cs = hit_material.R0 + (glm::vec3(1.0f, 1.0f, 1.0f) - hit_material.R0) * pow(1.0f - cosTheta, 5.0f);
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
```

实现阴影、Phong模型、反射与折射后的渲染效果如下：

![图10 折射](https://img-blog.csdnimg.cn/5d315d1023d444cbbd55da659f86d545.png '图10 折射')   
图10 折射

---

### 2.4.5 包含递归调用的光线追踪函数

综合上述Phong光照模型、阴影、反射、折射等部分，可以得到完整的包含递归调用的光线追踪函数。该函数在本项目中的完整代码如下：

```cpp
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

    // 计算反射光线方向（入射光线方向与法线方向点乘结果为负，故使用减法）
    glm::vec3 reflectDir = ray.dir - 2 * glm::dot(ray.dir, hit_norm) * hit_norm;
    Ray reflectRay = Ray(hit_pos + EPSILON * reflectDir, glm::normalize(reflectDir));
    // 使用菲涅尔方程的Shlick近似方程计算反射光线颜色
    glm::vec3 cs;
    float cosTheta = - glm::dot(glm::normalize(ray.dir), hit_norm);
    cs = hit_material.R0 + (glm::vec3(1.0f, 1.0f, 1.0f) - hit_material.R0) * pow(1.0f - cosTheta, 5.0f);
    // 递归调用光线追踪函数
    c = cs * Trace(reflectRay, depth + 1);
    
    // 折射计算
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
```

---

### 2.5 其他

定义函数renderScene对屏幕中每一个像素进行光线追踪计算，该函数具有3个参数，分别为屏幕的宽度、高度以及程序主函数中对于计算所得的像素颜色的处理函数的句柄handleFunc。该处理函数在主程序中定义并实现，用于存储经过光线追踪算法计算所得的像素元素数据，以便后续使用OpenGL进行绘制。

```cpp
void Scene::renderScene(float viewWidth, float viewHeight, void(*handleFunc) (float, float, glm::vec3)) {
    float fov = fmax(viewWidth, viewHeight);
    for (int viewx = 0; viewx < fov; viewx++) {
        for (int viewy = 0; viewy < fov; viewy++) {
            Ray viewRay = camera.getRay(viewx, viewy, fov);
            glm::vec3 pixelColor = Trace(viewRay);
            glm::vec2 cameraCoord = glm::vec2(1, 0) * (2 * (viewx + .5f) / fov - 1) +
                            glm::vec2(0, 1) * (2 * (viewy + .5f) / fov - 1);
            float renderX = fov == viewWidth ? cameraCoord.x : cameraCoord.x / viewWidth * viewHeight;
            float renderY = fov == viewHeight ? cameraCoord.y : cameraCoord.y / viewHeight * viewWidth;
            (*handleFunc) (renderX, renderY, pixelColor);
        }
    }
}
```

我使用的绘制方法是以点绘制，由于显示器的原因，如果使用1:1的比例进行绘制的话绘制结果会出现大量空隙影响观感，所以在renderScene函数中把屏幕尺寸设置为了真实窗口尺寸的2倍，推测使用Texture绘制能够避免这一问题。

```cpp
//于函数renderScene中调用，用于将计算所得的像素点颜色与坐标加入vertices向量中
void loadPixel(float fx, float fy, glm::vec3 fragColor){
    vertices.push_back(fx);
    vertices.push_back(fy);
    vertices.push_back(.0f);
    vertices.push_back(fragColor.x);
    vertices.push_back(fragColor.y);
    vertices.push_back(fragColor.z);
}
```

在main函数中进行场景初始化并添加球体，成功运行程序后能够得到文章一开始时的画面：

```cpp
Scene mainScene;
mainScene.setBackground(glm::vec3(.461f, .141f, .062f));
mainScene.setLight(new Light(glm::vec3(.5f, 1.0f, .5f), glm::vec3(1.5f, 1.5f, 1.5f)));
mainScene.addSphere(new Sphere(glm::vec3(.2f, .0f, .0f), .1f, roughMaterial(blue)));
mainScene.addSphere(new Sphere(glm::vec3(-.2f, .0f, .0f), .1f, roughMaterial(red)));
mainScene.addSphere(new Sphere(glm::vec3(.0f, .0f, -.2f), .1f, roughMaterial(brown)));
mainScene.addSphere(new Sphere(glm::vec3(.0f, .0f, .2f), .1f, defaultRefractive));
mainScene.addSphere(new Sphere(glm::vec3(.0f, .2f, .0f), .1f, metalReflective));
mainScene.addSphere(new Sphere(glm::vec3(.0f, -200.15f, .0f), 200.0f, defaultReflective));
mainScene.rotateCamera(.125 * PI);
vertices.clear();
mainScene.renderScene(2 * WINDOW_WIDTH, 2 * WINDOW_HEIGHT, loadPixel);
```

# 三、总结

总体实现了课程设计中的要求，由于没有做反走样，存在一定量的噪点，也是本项目中的不足。

文章中的参考资料均在参考时写出。

[项目地址](https://github.com/0xCCCCCCCCC/Ray_Tracing_CPU)
