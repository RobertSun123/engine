#version 320 es

precision highp float;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(abs(0.0), abs(-1.0), 0.0, abs(1.0));
}
