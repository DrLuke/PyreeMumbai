#version 450 core
layout (location = 0) in vec3 posIn;
layout (location = 1) in vec2 uvIn;
layout (location = 2) in vec3 normIn;

layout(binding=0) uniform sampler2D prev;
layout(binding=1) uniform sampler2D printer_page;

layout (location = 0) out vec4 colorOut;

uniform float time;
uniform vec2 res;

void main() {
    colorOut = vec4(vec3(0.3), 1);
}