#version 320 es

precision highp float;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(
        0.0,
        pow(3.14, 0.0),
        0.0,
        pow(3.0, 4.0) - 80.0
    );
}
