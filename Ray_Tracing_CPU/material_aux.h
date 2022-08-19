//
//  material_aux.h
//  Ray_Tracing_CPU
//
//  Created by YNK on 2022/8/1.
//

#ifndef material_aux_h
#define material_aux_h
#include "frameworks.hpp"

#define red glm::vec3(1.0f, .0f, .0f)
#define green glm::vec3(.0f, 1.0f, .0f)
#define blue glm::vec3(.0f, .0f, 1.0f)
#define yellow glm::vec3(1.0f, 1.0f, .0f)
#define cyan glm::vec3(.0f, 1.0f, 1.0f)
#define violet glm::vec3(.53f, .0f, .48f)
#define orange glm::vec3(1.0f, 1.0f, .0f)
#define white glm::vec3(1.0f, 1.0f, 1.0f)
#define black glm::vec3(.0f, .0f, .0f)
#define brown glm::vec3(.83f, .69f, .21f)


#define roughMaterial(color) (new Material(color, 32.0f))
#define defaultReflective (new Material(glm::vec3(.48f, .48f, .48f)))
#define metalReflective (new Material(glm::vec3(.86f, .87f, .88f)))
#define defaultRefractive (new Material(1.5f))

#endif /* material_aux_h */
