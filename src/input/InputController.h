#ifndef GRAPHICSPRAKTIKUM_INPUTCONTROLLER_H
#define GRAPHICSPRAKTIKUM_INPUTCONTROLLER_H


#include <map>

class InputController {

    struct KeyPressValues {
        bool pressed = false;
        bool pressedOnce = false;
        bool releasedOnce = false;
    };

    std::map<int, KeyPressValues> m_pressedKeys;

public:
    void handleKeyEvent(int key, int action);

    bool isPressed(int key);

    bool getSinglePress(int key);

    bool getSingleRelease(int key);
};


#endif //GRAPHICSPRAKTIKUM_INPUTCONTROLLER_H
