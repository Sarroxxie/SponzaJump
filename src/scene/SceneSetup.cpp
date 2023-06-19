#include <algorithm>
#include "SceneSetup.h"
#include "physics/PhysicsComponent.h"
#include "game/PlayerComponent.h"

void
createSamplePhysicsScene(const ApplicationVulkanContext &context, Scene &scene, GameContactListener &contactListener) {
    {
        ModelLoader loader;
        loader.loadModel("res/assets/models/debug_model4/debug_model4.gltf",
                         scene.getModelLoadingOffsets(), context.baseContext,
                         context.commandContext);
        scene.addObject(loader);
        // move a bit to the foreground so object can be seen

        /*
        scene.getSceneData().instances.back().transformation *=
            glm::translate(glm::mat4(1), glm::vec3(0, 0, 4));
            */
    }

    {
        ModelLoader loader;
        loader.loadModel("res/assets/models/debug_cube/debug_cube.gltf",
                         scene.getModelLoadingOffsets(), context.baseContext,
                         context.commandContext);
        scene.addObject(loader);
    }

    /* TODO:
     * 1. load gltf objects as part of the ECS
     * 2. Model Component will be Model (I think ?)
     * 3. Don't forget Transformation Component
     * 4. Find heuristic for when to add physics component, what size
     *      -> Make sure implementation can be changed to name based deciding ?
     * 5. remove instances
     * 6. Per Name Tasks:
     *      - Player Component
     *      - Collision Boxes ?
     *      -
     */

    /*
     * Get Transformation from Node
     * Get Min/Max from Accessors[Node->Mesh->Primitve->Attributes]
     */



    // remove the cube from the instances, because it will not be rendered directly, but added per Entity
    /*
    scene.getSceneData().instances.pop_back();
     */


    /* TODO:
     * 1. rewrite this function
     * 2. hook up new function to gltf files
     * 3. test with exisiting gltf files
     * 4. create simple sample level and test loading
     *
     */

    /*
    int   cubeModelID = scene.getSceneData().models.size() - 1;



    glm::vec3 groundHalfDims = glm::vec3(30, 1, 1);
    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext,
                     cubeModelID,
                     Transformation(),
                     groundHalfDims,
                     false);

    glm::vec3 sideHalfDims = glm::vec3(1, 10, 1);
    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext, cubeModelID,
                     {glm::vec3(-29, 9, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     sideHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext, cubeModelID,
                     {glm::vec3(29, 9, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     sideHalfDims,
                     false);

    glm::vec3 slopeHalfDims = glm::vec3(1, 20, 1);
    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext, cubeModelID,
                     {glm::vec3(45, 10, 0), glm::vec3(0, 0, glm::radians<float>(45)), glm::vec3(1)},
                     slopeHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext, cubeModelID,
                     {glm::vec3(65, 0, 0), glm::vec3(0, 0, glm::radians<float>(60)), glm::vec3(1)},
                     slopeHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext, cubeModelID,
                     {glm::vec3(85, -10, 0), glm::vec3(0, 0, glm::radians<float>(75)), glm::vec3(1)},
                     slopeHalfDims,
                     false);

    glm::vec3 floatingBoxHalfDims(1, 1, 1);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext, cubeModelID,
                     {glm::vec3(-24, 4, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext, cubeModelID,
                     {glm::vec3(-15, 8, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext, cubeModelID,
                     {glm::vec3(0, 7, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext, cubeModelID,
                     {glm::vec3(13, 9, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext, cubeModelID,
                     {glm::vec3(15, 15, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext, cubeModelID,
                     {glm::vec3(5, 19, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    glm::vec3 floatingBoxHalfDims2(1, 0.5, 1);
    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext, cubeModelID,
                     {glm::vec3(-11, 18.5, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims2,
                     false);

    addPhysicsEntity(scene,
                     context.baseContext,
                     context.commandContext, cubeModelID,
                     {glm::vec3(-27, 18, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                     floatingBoxHalfDims,
                     false);

    EntityId playerEntity = addPlayerEntity(scene,
                                            context.baseContext,
                                            context.commandContext, cubeModelID,
                                            {glm::vec3(30, 20, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                                            floatingBoxHalfDims,
                                            contactListener,
                                            true,
                                            true);

    auto *playerComponent = scene.assign<PlayerComponent>(playerEntity);

    contactListener.setPlayerComponent(playerComponent);

    /*
    int numDynamicObjects = 30;
    for (int i = 0; i < numDynamicObjects; i++) {
        glm::vec3 halfSize = glm::vec3(1, 1, 1);
        addPhysicsEntity(scene,
                         context.baseContext,
                         context.commandContext,
                         getCuboid(halfSize, glm::vec3(
                                 static_cast<float>(i + 1) / (static_cast<float>(numDynamicObjects) + 1.0f))),
                         {glm::vec3(-15 + ((static_cast<float>(i) / static_cast<float>(numDynamicObjects)) * 30),
                                    20 + 5 * i, 0), glm::vec3(0, 0, 0), glm::vec3(1)},
                         halfSize,
                         true,
                         false);
    }
     */
}


void addToScene(Scene& scene, ModelLoader& loader, GameContactListener &contactListener) {
    SceneData& sceneData = scene.getSceneData();

    sceneData.meshes.insert(sceneData.meshes.end(), loader.meshes.begin(), loader.meshes.end());
    sceneData.meshParts.insert(sceneData.meshParts.end(), loader.meshParts.begin(), loader.meshParts.end());
    sceneData.textures.insert(sceneData.textures.end(), loader.textures.begin(), loader.textures.end());
    sceneData.materials.insert(sceneData.materials.end(), loader.materials.begin(), loader.materials.end());

    // sceneData.models.insert(sceneData.models.end(), loader.models.begin(), loader.models.end());
    // sceneData.instances.insert(sceneData.instances.end(), loader.instances.begin(), loader.instances.end());


    for (ModelInstance& instance: loader.instances) {
        EntityId entityId = scene.addEntity();

        auto* modelComponent = scene.assign<Model>(entityId);
        modelComponent->meshPartIndices = loader.models[instance.modelID].meshPartIndices;

        auto* transformationComponent = scene.assign<Transformation>(entityId);
        transformationComponent->translation = instance.translation;
        transformationComponent->rotation = instance.rotation;
        transformationComponent->scaling = instance.scaling;


        const std::string PhysicsNamePrefix = "Static";
        if (instance.name.rfind(PhysicsNamePrefix, 0)) {
            auto* physicsComponent = scene.assign<PhysicsComponent>(entityId);

            b2BodyDef bodyDef;
            bodyDef.fixedRotation = true;
            bodyDef.position.Set(instance.translation.x, instance.translation.y);
            bodyDef.angle = instance.rotation.z;

            physicsComponent->body = scene.getWorld().CreateBody(&bodyDef);
            physicsComponent->dynamic = true;

            glm::vec3 halfSizes = (instance.max - instance.min) / glm::vec3(2.0f);

            b2PolygonShape dynamicBox;
            dynamicBox.SetAsBox(halfSizes.x, halfSizes.y);

            b2FixtureDef fixtureDef;
            fixtureDef.shape = &dynamicBox;
            fixtureDef.density = 1.0f;
            fixtureDef.friction = 0.0f;

            physicsComponent->body->CreateFixture(&fixtureDef);
        }


        const std::string playerNamePrefix = "Player";
        if (instance.name.rfind(playerNamePrefix, 0)) {
            auto* physicsComponent = scene.assign<PhysicsComponent>(entityId);

            b2BodyDef bodyDef;
            bodyDef.type = b2BodyType::b2_dynamicBody;
            bodyDef.fixedRotation = true;
            bodyDef.position.Set(instance.translation.x, instance.translation.y);
            bodyDef.angle = instance.rotation.z;

            physicsComponent->body = scene.getWorld().CreateBody(&bodyDef);
            physicsComponent->dynamic = true;

            glm::vec3 halfSizes = (instance.max - instance.min) / glm::vec3(2.0f);

            b2PolygonShape dynamicBox;
            dynamicBox.SetAsBox(halfSizes.x, halfSizes.y);

            b2FixtureDef fixtureDef;
            fixtureDef.shape = &dynamicBox;
            fixtureDef.density = 1.0f;
            fixtureDef.friction = 0.0f;

            physicsComponent->body->CreateFixture(&fixtureDef);
            physicsComponent->dynamic = true;

            auto *playerComponent = scene.assign<PlayerComponent>(entityId);
            contactListener.setPlayerComponent(playerComponent);
        }
    }




}

EntityId addPhysicsEntity(Scene &scene,
                          const VulkanBaseContext &context,
                          const CommandContext &commandContext,
                          int modelID,
                          Transformation transformation,
                          glm::vec3 halfSize,
                          bool dynamic,
                          bool fixedRotation) {

    EntityId dynamicEntity = scene.addEntity();

    // TODO: only the modelID is used here because the transformation is stored in a different component
    //        -> would be better to have a component that only stores the modelID
    // attaches a given model to the entity
    auto* modelComponent    = scene.assign<ModelInstance>(dynamicEntity);
    modelComponent->modelID = modelID;

    auto *pDynamicTransform = scene.assign<Transformation>(dynamicEntity);
    *pDynamicTransform = transformation;
    pDynamicTransform->scaling *= glm::vec3(halfSize[0], halfSize[1], halfSize[2]);

    auto *pDynamicPhysicsComponent = scene.assign<PhysicsComponent>(dynamicEntity);

    b2BodyDef bodyDef;
    if (dynamic)
        bodyDef.type = b2_dynamicBody;
    if (fixedRotation)
        bodyDef.fixedRotation = true;
    bodyDef.position.Set(transformation.translation.x, transformation.translation.y);
    bodyDef.angle = transformation.rotation.z;
    pDynamicPhysicsComponent->body = scene.getWorld().CreateBody(&bodyDef);
    pDynamicPhysicsComponent->dynamic = true;

    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(halfSize.x, halfSize.y);

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &dynamicBox;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.0f;

    pDynamicPhysicsComponent->body->CreateFixture(&fixtureDef);

    return dynamicEntity;
}
EntityId addPlayerEntity(Scene &scene, const VulkanBaseContext &context, const CommandContext &commandContext,
                         int modelID, Transformation transformation, glm::vec3 halfSize,
                         GameContactListener &contactListener,
                         bool dynamic, bool fixedRotation) {
    EntityId playerEntity = scene.addEntity();

    // TODO: only the modelID is used here because the transformation is stored in a different component
    //        -> would be better to have a component that only stores the modelID
    // attaches a given model to the entity
    auto* modelComponent    = scene.assign<ModelInstance>(playerEntity);
    modelComponent->modelID = modelID;

    auto *pDynamicTransform = scene.assign<Transformation>(playerEntity);
    *pDynamicTransform = transformation;
    pDynamicTransform->scaling *= glm::vec3(halfSize[0], halfSize[1], halfSize[2]);

    auto *pDynamicPhysicsComponent = scene.assign<PhysicsComponent>(playerEntity);

    b2BodyDef bodyDef;
    if (dynamic)
        bodyDef.type = b2_dynamicBody;
    if (fixedRotation)
        bodyDef.fixedRotation = true;
    bodyDef.position.Set(transformation.translation.x, transformation.translation.y);
    bodyDef.angle = transformation.rotation.z;
    pDynamicPhysicsComponent->body = scene.getWorld().CreateBody(&bodyDef);
    pDynamicPhysicsComponent->dynamic = true;

    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(halfSize.x, halfSize.y);

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &dynamicBox;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.0f;

    b2Fixture *fixture = pDynamicPhysicsComponent->body->CreateFixture(&fixtureDef);
    contactListener.setPlayerFixture(fixture);

    return playerEntity;
}
