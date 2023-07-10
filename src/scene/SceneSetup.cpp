#include <algorithm>
#include "SceneSetup.h"
#include "physics/PhysicsComponent.h"
#include "game/PlayerComponent.h"

void createSamplePhysicsScene(const ApplicationVulkanContext& context,
                              Scene&                          scene,
                              GameContactListener&            contactListener) {

    CubeMap& cubemap = scene.getSceneData().cubemap;

    cubemap.paths[0] = "res/assets/textures/cubemap/px.png";
    cubemap.paths[1] = "res/assets/textures/cubemap/nx.png";
    cubemap.paths[2] = "res/assets/textures/cubemap/py.png";
    cubemap.paths[3] = "res/assets/textures/cubemap/ny.png";
    cubemap.paths[4] = "res/assets/textures/cubemap/pz.png";
    cubemap.paths[5] = "res/assets/textures/cubemap/nz.png";
    createCubeMap(context.baseContext, context.commandContext, cubemap, true);

    {
        ModelLoader loader;


        //loader.loadModel("res/assets/models/scene2/playable.gltf",
        loader.loadModel("res/assets/models/sponza/sponza.gltf",
                         scene.getModelLoadingOffsets(), context.baseContext,
                         context.commandContext);

        addToScene(scene, loader, contactListener);
    }

    /*{
        ModelLoader loader;

        loader.loadModel("res/assets/models/sponza/sponza.gltf",
                         scene.getModelLoadingOffsets(), context.baseContext,
                         context.commandContext);

        addToScene(scene, loader, contactListener);
    }*/
}


void addToScene(Scene&                          scene,
                ModelLoader&                    loader,
                GameContactListener&            contactListener) {
    SceneData& sceneData = scene.getSceneData();
    LevelData& levelData = scene.getLevelData();

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
    sceneData.lights.insert(sceneData.lights.end(), loader.lights.begin(),
                            loader.lights.end());

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

        const std::string hazardNamePrefix = "Hazard";
        if(instance.name.rfind(hazardNamePrefix, 0) == 0) {
            auto* fixture = addPhysicsComponent(scene, entityId, instance, false);

            b2Filter filter;
            filter.categoryBits |= HAZARD_CATEGORY_BITS;

            fixture->SetFilterData(filter);
        }

        const std::string winAreaNamePrefix = "Win";
        if(instance.name.rfind(winAreaNamePrefix, 0) == 0) {
            auto* fixture = addPhysicsComponent(scene, entityId, instance, false);

            b2Filter filter;
            filter.categoryBits |= WIN_AREA_CATEGORY_BITS;

            fixture->SetFilterData(filter);
        }

        const std::string playerNamePrefix = "Player";
        if(instance.name.rfind(playerNamePrefix, 0) == 0) {
            auto* fixture = addPhysicsComponent(scene, entityId, instance, true);

            auto* playerComponent = scene.assign<PlayerComponent>(entityId);
            contactListener.setPlayerComponent(playerComponent);
            contactListener.setPlayerFixture(fixture);

            levelData.playerSpawnLocation = glm::vec3(instance.translation.x, instance.translation.y, 0);
        }
    }
}
/*
* NOTE: This function will only set the collision box correctly when the origin of the 3D model is also the
*       center point of its xy-bounds!
*/
b2Fixture* addPhysicsComponent(Scene& scene, EntityId entityId, ModelInstance& instance, bool isDynamic) {
    auto* physicsComponent = scene.assign<PhysicsComponent>(entityId);

    b2BodyDef bodyDef;
    bodyDef.type = isDynamic ? b2BodyType::b2_dynamicBody : b2BodyType::b2_staticBody;
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
