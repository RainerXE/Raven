#version 300 es
precision mediump float;
precision highp int;

uniform highp vec4 PushConstants[1];
uniform highp sampler2D u_source;

in highp vec2 v_uv;
layout(location = 0) out highp vec4 out_color;

void main()
{
    if (int(PushConstants[0].z + 0.5) == 0)
    {
        highp vec4 sum = textureLod(u_source, v_uv, 0.0) * 4.0;
        sum += textureLod(u_source, v_uv + vec2(-PushConstants[0].x, -PushConstants[0].y), 0.0);
        sum += textureLod(u_source, v_uv + vec2(PushConstants[0].x, -PushConstants[0].y), 0.0);
        sum += textureLod(u_source, v_uv + vec2(-PushConstants[0].x, PushConstants[0].y), 0.0);
        sum += textureLod(u_source, v_uv + vec2(PushConstants[0].x, PushConstants[0].y), 0.0);
        out_color = sum / vec4(8.0);
    }
    else
    {
        if (int(PushConstants[0].z + 0.5) == 1)
        {
            highp vec4 sum_1 = textureLod(u_source, v_uv + vec2((-PushConstants[0].x) * 2.0, 0.0), 0.0);
            sum_1 += textureLod(u_source, v_uv + vec2(PushConstants[0].x * 2.0, 0.0), 0.0);
            sum_1 += textureLod(u_source, v_uv + vec2(0.0, (-PushConstants[0].y) * 2.0), 0.0);
            sum_1 += textureLod(u_source, v_uv + vec2(0.0, PushConstants[0].y * 2.0), 0.0);
            sum_1 += (textureLod(u_source, v_uv + vec2(-PushConstants[0].x, PushConstants[0].y), 0.0) * 2.0);
            sum_1 += (textureLod(u_source, v_uv + vec2(PushConstants[0].x, PushConstants[0].y), 0.0) * 2.0);
            sum_1 += (textureLod(u_source, v_uv + vec2(-PushConstants[0].x, -PushConstants[0].y), 0.0) * 2.0);
            sum_1 += (textureLod(u_source, v_uv + vec2(PushConstants[0].x, -PushConstants[0].y), 0.0) * 2.0);
            out_color = sum_1 / vec4(12.0);
        }
        else
        {
            out_color = textureLod(u_source, v_uv, 0.0);
        }
    }
}

