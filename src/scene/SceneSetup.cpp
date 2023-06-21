#include <algorithm>
#include "SceneSetup.h"
#include "physics/PhysicsComponent.h"
#include "game/PlayerComponent.h"

void createSamplePhysicsScene(const ApplicationVulkanContext& context,
                              Scene&                          scene,
                              GameContactListener&            contactListener) {
    ModelLoader loader;

    loader.loadModel("res/assets/models/debug_model4/debug_model4.gltf",
                     scene.getModelLoadingOffsets(), context.baseContext,
                     context.commandContext);

    // trying out the cubemap code
    scene.getCubeMap().paths[0] = "res/assets/textures/cubemap/px.png";
    scene.getCubeMap().paths[1] = "res/assets/textures/cubemap/nx.png";
    scene.getCubeMap().paths[2] = "res/assets/textures/cubemap/py.png";
    scene.getCubeMap().paths[3] = "res/assets/textures/cubemap/ny.png";
    scene.getCubeMap().paths[4] = "res/assets/textures/cubemap/pz.png";
    scene.getCubeMap().paths[5] = "res/assets/textures/cubemap/nz.png";
    createCubeMap(context.baseContext, context.commandContext, scene.getCubeMap(), false);


    addToScene(scene, loader, contactListener);
}


void addToScene(Scene& scene, ModelLoader& loader, GameContactListener& contactListener) {
    SceneData& sceneData = scene.getSceneData();

    sceneData.meshes.insert(sceneData.meshes.end(), loader.meshes.begin(),
                            loader.meshes.end());
    sceneData.meshParts.insert(sceneData.meshParts.end(),
                               loader.meshParts.begin(), loader.meshParts.end());
    sceneData.textures.insert(sceneData.textures.end(), loader.textures.begin(),
                              loader.textures.end());
    sceneData.materials.insert(sceneData.materials.end(),
                               loader.materials.begin(), loader.materials.end());
    sceneData.models.insert(sceneData.models.end(), loader.models.begin(),
                            loader.models.end());

    for(ModelInstance& instance : loader.instances) {
        EntityId entityId = scene.addEntity();

        auto* modelComponent       = scene.assign<ModelComponent>(entityId);
        modelComponent->modelIndex = instance.modelID;

        auto* transformationComponent = scene.assign<Transformation>(entityId);
        transformationComponent->translation = instance.translation;
        transformationComponent->rotation    = instance.rotation;
        transformationComponent->scaling     = instance.scaling;


        const std::string PhysicsNamePrefix = "Static";
        if(instance.name.rfind(PhysicsNamePrefix, 0) == 0) {
            addPhysicsComponent(scene, entityId, instance, false);
        }


        const std::string playerNamePrefix = "Player";
        if(instance.name.rfind(playerNamePrefix, 0) == 0) {
            auto* fixture = addPhysicsComponent(scene, entityId, instance, true);

            auto* playerComponent = scene.assign<PlayerComponent>(entityId);
            contactListener.setPlayerComponent(playerComponent);
            contactListener.setPlayerFixture(fixture);
        }
    }
}
b2Fixture* addPhysicsComponent(Scene& scene, EntityId entityId, ModelInstance& instance, bool isDynamic) {
    auto* physicsComponent = scene.assign<PhysicsComponent>(entityId);

    b2BodyDef bodyDef;
    bodyDef.type          = isDynamic ? b2BodyType::b2_dynamicBody : b2BodyType::b2_staticBody;
    bodyDef.fixedRotation = true;
    bodyDef.position.Set(instance.translation.x, instance.translation.y);
    bodyDef.angle = instance.rotation.z;

    physicsComponent->body    = scene.getWorld().CreateBody(&bodyDef);
    physicsComponent->dynamic = true;

    glm::vec3 halfSizes =
        (instance.max * instance.scaling - instance.min * instance.scaling)
        / glm::vec3(2.0f);

    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(halfSizes.x, halfSizes.y);

    b2FixtureDef fixtureDef;
    fixtureDef.shape    = &dynamicBox;
    fixtureDef.density  = 1.0f;
    fixtureDef.friction = 0.0f;

    b2Fixture* fixture = physicsComponent->body->CreateFixture(&fixtureDef);
    physicsComponent->dynamic = isDynamic;

    return fixture;
}
