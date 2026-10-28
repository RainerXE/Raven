#version 410
#ifdef GL_ARB_shading_language_420pack
#extension GL_ARB_shading_language_420pack : require
#endif

layout(location = 0) out vec2 v_uv;

void main()
{
    vec2 corner = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    v_uv = corner;
    gl_Position = vec4((corner * 2.0) - vec2(1.0), 0.0, 1.0);
}

