#include "InputController.h"
#include "GLFW/glfw3.h"

void InputController::handleKeyEvent(int key, int action) {
    auto values = m_pressedKeys[key];

    if (values.pressed) {
        if (action == GLFW_PRESS) {
            m_pressedKeys[key] = { true, values.pressedOnce, false };
        } else if (action == GLFW_RELEASE) {
            m_pressedKeys[key] = {false, false, true };
        }
    } else {
        if (action == GLFW_PRESS) {
            m_pressedKeys[key] = { true, true, false };
        } else if (action == GLFW_RELEASE) {
            m_pressedKeys[key] = {false, false, values.releasedOnce };
        }
    }
}

bool InputController::isPressed(int key) {
    auto it = m_pressedKeys.find(key);
    return it != m_pressedKeys.end() && it->second.pressed;
}

bool InputController::getSinglePress(int key) {
    auto it = m_pressedKeys.find(key);
    if (it == m_pressedKeys.end()) return false;

    bool res = it->second.pressedOnce;
    it->second.pressedOnce = false;
    return res;
}

bool InputController::getSingleRelease(int key) {
    auto it = m_pressedKeys.find(key);
    if (it == m_pressedKeys.end()) return false;

    bool res = it->second.releasedOnce;
    it->second.releasedOnce = false;
    return res;
}
