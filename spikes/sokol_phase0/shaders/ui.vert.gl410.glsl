#version 410
#ifdef GL_ARB_shading_language_420pack
#extension GL_ARB_shading_language_420pack : require
#endif

uniform vec4 PushConstants[2];
layout(location = 7) in uint in_flags;
layout(location = 0) in vec4 in_rect;
layout(location = 6) in float in_border_width;
layout(location = 3) in vec4 in_radii;
layout(location = 1) in vec4 in_uv;
layout(location = 0) out vec2 v_uv;
layout(location = 1) flat out vec4 v_rect;
layout(location = 2) flat out vec4 v_clip;
layout(location = 2) in vec4 in_clip;
layout(location = 3) flat out vec4 v_radii;
layout(location = 4) flat out vec4 v_color;
layout(location = 4) in vec4 in_color;
layout(location = 5) flat out vec4 v_border_color;
layout(location = 5) in vec4 in_border_color;
layout(location = 6) flat out float v_border_width;
layout(location = 7) flat out uint v_flags;
layout(location = 8) flat out vec4 v_params;
layout(location = 9) flat out vec2 v_inv_size;
layout(location = 10) flat out vec4 v_pointer;

void main()
{
    uint kind = in_flags & 15u;
    vec2 corner = vec2(float(gl_VertexID & 1), float(gl_VertexID >> 1));
    vec2 lo = in_rect.xy;
    vec2 hi = in_rect.zw;
    if ((((kind == 0u) || (kind == 5u)) || (kind == 6u)) || (kind == 8u))
    {
        lo -= vec2(0.5);
        hi += vec2(0.5);
    }
    else
    {
        if (kind == 2u)
        {
            float grow = in_border_width + 1.0;
            lo -= vec2(grow);
            hi += vec2(grow);
        }
        else
        {
            if (kind == 3u)
            {
                float grow_1 = (in_radii.x * 0.5) + 1.0;
                lo = min(in_rect.xy, in_rect.zw) - vec2(grow_1);
                hi = max(in_rect.xy, in_rect.zw) + vec2(grow_1);
            }
            else
            {
                if (kind == 7u)
                {
                    float grow_2 = ((in_border_width * 0.5) + in_uv.x) + 1.0;
                    lo = min(min(in_rect.xy, in_rect.zw), min(in_radii.xy, in_radii.zw)) - vec2(grow_2);
                    hi = max(max(in_rect.xy, in_rect.zw), max(in_radii.xy, in_radii.zw)) + vec2(grow_2);
                }
                else
                {
                    if (kind == 4u)
                    {
                        lo = min(min(in_rect.xy, in_rect.zw), in_radii.xy) - vec2(1.0);
                        hi = max(max(in_rect.xy, in_rect.zw), in_radii.xy) + vec2(1.0);
                    }
                }
            }
        }
    }
    vec2 position = mix(lo, hi, corner);
    vec2 size = max(in_rect.zw - in_rect.xy, vec2(9.9999999747524270787835121154785e-07));
    v_uv = mix(in_uv.xy, in_uv.zw, (position - in_rect.xy) / size);
    v_rect = in_rect;
    v_clip = in_clip;
    v_radii = in_radii;
    v_color = vec4(in_color.xyz * in_color.w, in_color.w);
    v_border_color = vec4(in_border_color.xyz * in_border_color.w, in_border_color.w);
    v_border_width = in_border_width;
    v_flags = in_flags;
    v_params = in_uv;
    v_inv_size = PushConstants[0].xy * 0.5;
    vec4 _265;
    if ((in_flags & 32u) != 0u)
    {
        _265 = vec4(-1000000.0, -1000000.0, 0.0, 0.0);
    }
    else
    {
        _265 = vec4(PushConstants[1].xy, float((in_flags >> uint(16)) & 255u) / 255.0, float(in_flags >> uint(24)) / 255.0);
    }
    v_pointer = _265;
    gl_Position = vec4((position * PushConstants[0].xy) + PushConstants[0].zw, 0.0, 1.0);
}

