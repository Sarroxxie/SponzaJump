#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../../../src/rendering/host_device.h"

void main() {
    /*if (gl_FragCoord.x < 250) {
        gl_FragDepth = 0.1;
    } else {
        gl_FragDepth = 1;
    }*/
    gl_FragDepth = gl_FragDepth * 0.4;
}