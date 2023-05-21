#include "InputController.h"
#include "GLFW/glfw3.h"

void InputController::handleKeyEvent(int key, int action) {
    if (action == GLFW_PRESS) {
        m_pressedKeys[key] = true;
    } else if (action == GLFW_RELEASE) {
        m_pressedKeys[key] = false;
    }
}

bool InputController::isPressed(int key) {
    auto it = m_pressedKeys.find(key);
    return it != m_pressedKeys.end() && it->second;
}
