#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(binding = 0) uniform SceneTransform {
    mat4 proj;
    mat4 view;
} sceneTransform;

layout( push_constant ) uniform ObjectTransform {
    mat4 obj;
} objectTransform;

layout(location = 0) out vec2 fragPos;
layout(location = 1) out vec3 fragColor;

void main() {
    gl_Position = sceneTransform.proj * sceneTransform.view * objectTransform.obj * vec4(inPosition, 1);
    fragColor = inColor;
    //fragColor = vec3(0,0,1);
}