#ifndef GRAPHICSPRAKTIKUM_INPUTCONTROLLER_H
#define GRAPHICSPRAKTIKUM_INPUTCONTROLLER_H


#include <map>

class InputController {

    std::map<int, bool> m_pressedKeys;

public:
    void handleKeyEvent(int key, int action);

    bool isPressed(int key);
};


#endif //GRAPHICSPRAKTIKUM_INPUTCONTROLLER_H
