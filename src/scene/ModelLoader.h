#include <tiny_gltf.h>
#include "scene/Model.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

struct VertexObj
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec4 tangents;
    glm::vec2 texCoord;
};

class ModelLoader
{
  public:
    bool loadModel(const std::string& filename);

    // stores per node transformation -> will be used to create instances from the models
    std::vector<glm::mat4> transforms;

  private:
    glm::mat4 getTransform(tinygltf::Node node);
    Material  createMaterial(tinygltf::Material material);
    Mesh      createMesh(tinygltf::Mesh mesh, tinygltf::Accessor accessors);
};