#version 440
#extension GL_GOOGLE_include_directive : enable

#include "uniformbuffer.glsl"

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D plane1Texture;
layout(binding = 2) uniform sampler2D plane2Texture;
layout(binding = 3) uniform sampler2D plane3Texture;

void main()
{
    float Y = texture(plane1Texture, texCoord).r * 64;
    float U = texture(plane2Texture, texCoord).r * 64;
    float V = texture(plane3Texture, texCoord).r * 64;
    vec4 color = vec4(Y, U, V, 1.);
    fragColor = ubuf.colorMatrix * color * ubuf.opacity;
}
