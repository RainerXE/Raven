#version 300 es
precision mediump float;
precision highp int;

uniform highp sampler2D u_backdrop;
uniform highp sampler2D u_backdrop_blur;
uniform highp sampler2D u_texture;

flat in highp vec4 v_params;
flat in highp float v_border_width;
flat in highp vec2 v_inv_size;
flat in highp vec4 v_color;
flat in highp vec4 v_border_color;
flat in highp vec4 v_rect;
flat in uint v_flags;
flat in highp vec4 v_clip;
in highp vec2 v_uv;
flat in highp vec4 v_radii;
layout(location = 0) out highp vec4 out_color;

highp float sd_round_box(highp vec2 p, highp vec2 half_size, highp vec4 radii)
{
    highp float _60;
    if (p.x < 0.0)
    {
        highp float _67;
        if (p.y < 0.0)
        {
            _67 = radii.x;
        }
        else
        {
            _67 = radii.w;
        }
        _60 = _67;
    }
    else
    {
        highp float _81;
        if (p.y < 0.0)
        {
            _81 = radii.y;
        }
        else
        {
            _81 = radii.z;
        }
        _60 = _81;
    }
    highp float radius = _60;
    radius = min(radius, min(half_size.x, half_size.y));
    highp vec2 q = (abs(p) - half_size) + vec2(radius);
    return (min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0)))) - radius;
}

highp float sd_segment(highp vec2 p, highp vec2 a, highp vec2 b)
{
    highp vec2 pa = p - a;
    highp vec2 ba = b - a;
    highp float h = clamp(dot(pa, ba) / max(dot(ba, ba), 9.9999999747524270787835121154785e-07), 0.0, 1.0);
    return length(pa - (ba * h));
}

highp float sd_triangle(highp vec2 p, highp vec2 p0, highp vec2 p1, highp vec2 p2)
{
    highp vec2 e0 = p1 - p0;
    highp vec2 e1 = p2 - p1;
    highp vec2 e2 = p0 - p2;
    highp vec2 v0 = p - p0;
    highp vec2 v1 = p - p1;
    highp vec2 v2 = p - p2;
    highp vec2 pq0 = v0 - (e0 * clamp(dot(v0, e0) / max(dot(e0, e0), 9.9999999747524270787835121154785e-07), 0.0, 1.0));
    highp vec2 pq1 = v1 - (e1 * clamp(dot(v1, e1) / max(dot(e1, e1), 9.9999999747524270787835121154785e-07), 0.0, 1.0));
    highp vec2 pq2 = v2 - (e2 * clamp(dot(v2, e2) / max(dot(e2, e2), 9.9999999747524270787835121154785e-07), 0.0, 1.0));
    highp float s = sign((e0.x * e2.y) - (e0.y * e2.x));
    highp vec2 d = min(min(vec2(dot(pq0, pq0), s * ((v0.x * e0.y) - (v0.y * e0.x))), vec2(dot(pq1, pq1), s * ((v1.x * e1.y) - (v1.y * e1.x)))), vec2(dot(pq2, pq2), s * ((v2.x * e2.y) - (v2.y * e2.x))));
    return (-sqrt(d.x)) * sign(d.y);
}

highp float sd_round_box_normal(highp vec2 p, highp vec2 half_size, highp vec4 radii, out highp vec2 normal)
{
    highp float _126;
    if (p.x < 0.0)
    {
        highp float _132;
        if (p.y < 0.0)
        {
            _132 = radii.x;
        }
        else
        {
            _132 = radii.w;
        }
        _126 = _132;
    }
    else
    {
        highp float _145;
        if (p.y < 0.0)
        {
            _145 = radii.y;
        }
        else
        {
            _145 = radii.z;
        }
        _126 = _145;
    }
    highp float radius = _126;
    radius = min(radius, min(half_size.x, half_size.y));
    highp vec2 side = vec2((p.x < 0.0) ? (-1.0) : 1.0, (p.y < 0.0) ? (-1.0) : 1.0);
    highp vec2 q = (abs(p) - half_size) + vec2(radius);
    highp float corner = length(max(q, vec2(0.0)));
    bool _189 = q.x > 0.0;
    bool _195;
    if (_189)
    {
        _195 = q.y > 0.0;
    }
    else
    {
        _195 = _189;
    }
    if (_195)
    {
        normal = side * (q / vec2(max(corner, 9.9999997473787516355514526367188e-05)));
    }
    else
    {
        highp vec2 _212;
        if (q.x > q.y)
        {
            _212 = vec2(side.x, 0.0);
        }
        else
        {
            _212 = vec2(0.0, side.y);
        }
        normal = _212;
    }
    return (min(max(q.x, q.y), 0.0) + corner) - radius;
}

highp vec4 glass(highp vec2 p, highp float d, highp vec2 normal)
{
    highp float frost = v_params.x;
    highp float refraction = v_params.y;
    highp float bevel = max(v_border_width, 1.0);
    highp float t = clamp(1.0 + (d / bevel), 0.0, 1.0);
    highp float bend = (t * t) * t;
    highp vec2 shift = ((-normal) * bend) * refraction;
    highp vec2 uv = (p + shift) * v_inv_size;
    highp vec2 spread = (shift * 0.14000000059604644775390625) * v_inv_size;
    highp vec4 middle = textureLod(u_backdrop, uv, 0.0);
    highp vec3 sharp = vec3(textureLod(u_backdrop, uv + spread, 0.0).x, middle.y, textureLod(u_backdrop, uv - spread, 0.0).z);
    highp vec4 wide = textureLod(u_backdrop_blur, uv, 0.0);
    highp vec3 blurred = wide.xyz;
    highp vec3 color = mix(sharp, blurred, vec3(frost));
    highp float behind = mix(middle.w, wide.w, frost);
    highp float luma = dot(color, vec3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875));
    color = max(mix(vec3(luma), color, vec3(v_params.z)), vec3(0.0));
    color = (color * (1.0 - v_color.w)) + v_color.xyz;
    highp float facing = dot(normal, vec2(-0.55470001697540283203125, -0.832099974155426025390625));
    highp float _363;
    if (facing > 0.0)
    {
        _363 = pow(facing, 1.5);
    }
    else
    {
        _363 = 0.5 * pow(-facing, 1.5);
    }
    highp float lit = 0.2199999988079071044921875 + _363;
    highp float line_width = clamp(bevel * 0.119999997317790985107421875, 1.0, 2.5);
    highp float line = clamp(1.0 + (d / line_width), 0.0, 1.0);
    color += (v_border_color.xyz * (((line * line) * lit) + (bend * 0.100000001490116119384765625)));
    highp vec3 around = textureLod(u_backdrop_blur, (p + (normal * ((bevel * 1.5) + 6.0))) * v_inv_size, 0.0).xyz;
    color += ((((around * (vec3(1.0) - color)) * bend) * 0.449999988079071044921875) * v_border_color.w);
    highp float height = max(v_rect.w - v_rect.y, 1.0);
    color += vec3((v_border_color.w * 0.070000000298023223876953125) * (0.5 - ((p.y - v_rect.y) / height)));
    color += vec3(v_params.w);
    highp float opacity = float((v_flags >> uint(8)) & 255u) / 255.0;
    highp float alpha = clamp((1.0 - ((1.0 - v_color.w) * (1.0 - behind))) + (((v_border_color.w * line) * line) * lit), 0.0, 1.0);
    return vec4(clamp(color, vec3(0.0), vec3(1.0)), alpha) * opacity;
}

highp float grain(highp vec2 p)
{
    return fract(sin(dot(floor(p), vec2(12.98980045318603515625, 78.233001708984375))) * 43758.546875) - 0.5;
}

highp vec4 blur(highp vec2 p)
{
    highp float radius = max(v_params.x, 0.0);
    highp vec2 uv = p * v_inv_size;
    highp vec4 color;
    if (radius < 0.75)
    {
        color = textureLod(u_backdrop, uv, 0.0);
    }
    else
    {
        highp vec4 near = vec4(0.0);
        highp float reach = min(radius, 22.0);
        for (int i = 0; i < 16; i++)
        {
            highp float t = (float(i) + 0.5) / 16.0;
            highp float angle = float(i) * 2.3999631404876708984375;
            highp vec2 offset = (vec2(cos(angle), sin(angle)) * sqrt(t)) * reach;
            near += textureLod(u_backdrop, uv + (offset * v_inv_size), 0.0);
        }
        near /= vec4(16.0);
        highp vec4 far = textureLod(u_backdrop_blur, uv, 0.0);
        if (radius > 22.0)
        {
            highp float extra = radius - 22.0;
            highp vec4 ring = vec4(0.0);
            for (int i_1 = 0; i_1 < 8; i_1++)
            {
                highp float angle_1 = float(i_1) * 0.785398185253143310546875;
                ring += textureLod(u_backdrop_blur, uv + ((vec2(cos(angle_1), sin(angle_1)) * extra) * v_inv_size), 0.0);
            }
            far = mix(far, ring / vec4(8.0), vec4(0.75));
        }
        color = mix(near, far, vec4(smoothstep(9.8999996185302734375, 22.0, radius)));
    }
    highp float luma = dot(color.xyz, vec3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875));
    highp vec4 _651 = color;
    highp vec3 _658 = max(mix(vec3(luma), _651.xyz, vec3(v_params.z)), vec3(0.0));
    color.x = _658.x;
    color.y = _658.y;
    color.z = _658.z;
    highp vec2 param = p;
    highp float _674 = color.w;
    highp vec4 _676 = color;
    highp vec3 _679 = _676.xyz + vec3(((grain(param) * v_params.y) * 0.039999999105930328369140625) * _674);
    color.x = _679.x;
    color.y = _679.y;
    color.z = _679.z;
    color = (color * (1.0 - v_color.w)) + v_color;
    highp float _696 = color.w;
    highp vec4 _698 = color;
    highp vec3 _701 = _698.xyz + vec3(v_params.w * _696);
    color.x = _701.x;
    color.y = _701.y;
    color.z = _701.z;
    highp float opacity = float((v_flags >> uint(8)) & 255u) / 255.0;
    return clamp(color, vec4(0.0), vec4(1.0)) * opacity;
}

void main()
{
    highp vec2 p = gl_FragCoord.xy;
    bool _901 = p.x < v_clip.x;
    bool _910;
    if (!_901)
    {
        _910 = p.y < v_clip.y;
    }
    else
    {
        _910 = _901;
    }
    bool _919;
    if (!_910)
    {
        _919 = p.x >= v_clip.z;
    }
    else
    {
        _919 = _910;
    }
    bool _928;
    if (!_919)
    {
        _928 = p.y >= v_clip.w;
    }
    else
    {
        _928 = _919;
    }
    if (_928)
    {
        discard;
    }
    uint kind = v_flags & 15u;
    highp vec4 color = v_color;
    if ((v_flags & 16u) != 0u)
    {
        highp vec4 texel = textureLod(u_texture, v_uv, 0.0);
        color *= vec4(texel.xyz * texel.w, texel.w);
    }
    highp float coverage = 1.0;
    if (kind == 0u)
    {
        highp vec2 half_size = (v_rect.zw - v_rect.xy) * 0.5;
        highp vec2 param = p - (v_rect.xy + half_size);
        highp vec2 param_1 = half_size;
        highp vec4 param_2 = v_radii;
        highp float d = sd_round_box(param, param_1, param_2);
        coverage = clamp(0.5 - d, 0.0, 1.0);
        if (v_border_width > 0.0)
        {
            highp float inside = clamp(0.5 - (d + v_border_width), 0.0, 1.0);
            color = mix(v_border_color, color, vec4(inside));
        }
    }
    else
    {
        if (kind == 2u)
        {
            highp vec2 half_size_1 = (v_rect.zw - v_rect.xy) * 0.5;
            highp vec2 param_3 = p - (v_rect.xy + half_size_1);
            highp vec2 param_4 = half_size_1;
            highp vec4 param_5 = v_radii;
            highp float d_1 = sd_round_box(param_3, param_4, param_5);
            highp float blur_1 = max(v_border_width, 0.5);
            coverage = 1.0 - smoothstep(-blur_1, blur_1, d_1);
        }
        else
        {
            if (kind == 3u)
            {
                highp vec2 param_6 = p;
                highp vec2 param_7 = v_rect.xy;
                highp vec2 param_8 = v_rect.zw;
                highp float d_2 = sd_segment(param_6, param_7, param_8) - (v_radii.x * 0.5);
                coverage = clamp(0.5 - d_2, 0.0, 1.0);
            }
            else
            {
                if (kind == 4u)
                {
                    highp vec2 param_9 = p;
                    highp vec2 param_10 = v_rect.xy;
                    highp vec2 param_11 = v_rect.zw;
                    highp vec2 param_12 = v_radii.xy;
                    highp float d_3 = sd_triangle(param_9, param_10, param_11, param_12);
                    coverage = clamp(0.5 - d_3, 0.0, 1.0);
                }
                else
                {
                    if (kind == 7u)
                    {
                        highp vec2 a = v_rect.xy;
                        highp vec2 b = v_radii.xy;
                        highp vec2 c = v_radii.zw;
                        highp vec2 e = v_rect.zw;
                        highp vec2 from = a;
                        highp float nearest = 100000002004087734272.0;
                        highp float along = 0.0;
                        for (int i = 1; i <= 24; i++)
                        {
                            highp float t = float(i) / 24.0;
                            highp float u = 1.0 - t;
                            highp vec2 to = (((a * ((u * u) * u)) + (b * (((3.0 * u) * u) * t))) + (c * (((3.0 * u) * t) * t))) + (e * ((t * t) * t));
                            highp vec2 pa = p - from;
                            highp vec2 ba = to - from;
                            highp float h = clamp(dot(pa, ba) / max(dot(ba, ba), 9.9999999747524270787835121154785e-07), 0.0, 1.0);
                            highp vec2 away = pa - (ba * h);
                            highp float d2 = dot(away, away);
                            if (d2 < nearest)
                            {
                                nearest = d2;
                                along = (float(i - 1) + h) / 24.0;
                            }
                            from = to;
                        }
                        highp float d_4 = sqrt(nearest) - (v_border_width * 0.5);
                        highp float stroke = clamp(0.5 - d_4, 0.0, 1.0);
                        highp float pulse = 0.0;
                        color = mix(v_color, v_border_color, vec4(along));
                        highp float bright = float((v_flags >> uint(8)) & 255u) / 255.0;
                        if ((v_params.w > 0.0) && (bright > 0.0))
                        {
                            highp float f = fract((along * v_params.w) - v_params.z) - 0.5;
                            pulse = exp(((-f) * f) * 80.0) * bright;
                            highp vec4 _1253 = color;
                            highp float _1256 = color.w;
                            highp vec3 _1262 = mix(_1253.xyz, vec3(_1256), vec3(pulse * 0.60000002384185791015625));
                            color.x = _1262.x;
                            color.y = _1262.y;
                            color.z = _1262.z;
                        }
                        highp float halo = 0.0;
                        if (v_params.x > 0.0)
                        {
                            halo = 1.0 - smoothstep(0.0, v_params.x, max(d_4, 0.0));
                            halo = ((halo * halo) * v_params.y) * (1.0 + (pulse * 0.800000011920928955078125));
                        }
                        coverage = stroke + ((1.0 - stroke) * min(halo, 1.0));
                    }
                    else
                    {
                        if (kind == 5u)
                        {
                            highp vec2 half_size_2 = (v_rect.zw - v_rect.xy) * 0.5;
                            highp vec2 param_13 = p - (v_rect.xy + half_size_2);
                            highp vec2 param_14 = half_size_2;
                            highp vec4 param_15 = v_radii;
                            highp vec2 param_16;
                            highp float _1326 = sd_round_box_normal(param_13, param_14, param_15, param_16);
                            highp vec2 normal = param_16;
                            highp float d_5 = _1326;
                            coverage = clamp(0.5 - d_5, 0.0, 1.0);
                            highp vec2 param_17 = p;
                            highp float param_18 = d_5;
                            highp vec2 param_19 = normal;
                            color = glass(param_17, param_18, param_19);
                        }
                        else
                        {
                            if (kind == 6u)
                            {
                                highp vec2 half_size_3 = (v_rect.zw - v_rect.xy) * 0.5;
                                highp vec2 param_20 = p - (v_rect.xy + half_size_3);
                                highp vec2 param_21 = half_size_3;
                                highp vec4 param_22 = v_radii;
                                highp float d_6 = sd_round_box(param_20, param_21, param_22);
                                coverage = clamp(0.5 - d_6, 0.0, 1.0);
                                highp vec2 param_23 = p;
                                color = blur(param_23);
                                if (v_border_width > 0.0)
                                {
                                    highp float inside_1 = clamp(0.5 - (d_6 + v_border_width), 0.0, 1.0);
                                    color = mix(v_border_color + (color * (1.0 - v_border_color.w)), color, vec4(inside_1));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    out_color = color * coverage;
}

