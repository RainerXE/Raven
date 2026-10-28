#version 300 es
precision mediump float;
precision highp int;

struct RavenEffect
{
    highp vec2 uv;
    highp vec2 pixel;
    highp vec2 size;
    highp vec2 screen;
    highp float edge;
    highp float time;
    highp vec2 pointer;
    highp float hover;
    highp float press;
    highp vec4 color;
    highp vec4 color2;
    highp vec4 params;
};

uniform highp sampler2D rvn_texture;
uniform highp sampler2D rvn_backdrop;
uniform highp sampler2D rvn_backdrop_blur;

flat in highp vec4 v_clip;
flat in highp vec4 v_rect;
flat in highp vec4 v_radii;
flat in highp float v_border_width;
flat in highp vec4 v_pointer;
flat in highp vec4 v_color;
flat in highp vec4 v_border_color;
flat in highp vec4 v_params;
flat in uint v_flags;
layout(location = 0) out highp vec4 rvn_out;
in highp vec2 v_uv;
flat in highp vec2 v_inv_size;

highp float raven_sd_round_box(highp vec2 p, highp vec2 half_size, highp vec4 radii)
{
    highp float _161;
    if (p.x < 0.0)
    {
        highp float _167;
        if (p.y < 0.0)
        {
            _167 = radii.x;
        }
        else
        {
            _167 = radii.w;
        }
        _161 = _167;
    }
    else
    {
        highp float _181;
        if (p.y < 0.0)
        {
            _181 = radii.y;
        }
        else
        {
            _181 = radii.z;
        }
        _161 = _181;
    }
    highp float radius = _161;
    radius = min(radius, min(half_size.x, half_size.y));
    highp vec2 q = (abs(p) - half_size) + vec2(radius);
    return (min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0)))) - radius;
}

bool raven_has_pointer(RavenEffect fx)
{
    return fx.pointer.x > (-100000.0);
}

highp float raven_hash(inout highp vec2 p)
{
    p = fract(p * vec2(123.339996337890625, 456.209991455078125));
    p += vec2(dot(p, p + vec2(45.31999969482421875)));
    return fract(p.x * p.y);
}

highp float raven_noise(highp vec2 p)
{
    highp vec2 i = floor(p);
    highp vec2 f = fract(p);
    highp vec2 u = (f * f) * (vec2(3.0) - (f * 2.0));
    highp vec2 param = i;
    highp float _84 = raven_hash(param);
    highp vec2 param_1 = i + vec2(1.0, 0.0);
    highp float _91 = raven_hash(param_1);
    highp vec2 param_2 = i + vec2(0.0, 1.0);
    highp float _99 = raven_hash(param_2);
    highp vec2 param_3 = i + vec2(1.0);
    highp float _104 = raven_hash(param_3);
    return mix(mix(_84, _91, u.x), mix(_99, _104, u.x), u.y);
}

highp float raven_fbm(inout highp vec2 p)
{
    highp float sum = 0.0;
    highp float amplitude = 0.5;
    for (int i = 0; i < 5; i++)
    {
        highp vec2 param = p;
        sum += (amplitude * raven_noise(param));
        p = (p * 2.0299999713897705078125) + vec2(17.1000003814697265625, 9.19999980926513671875);
        amplitude *= 0.5;
    }
    return sum;
}

highp vec4 raven_effect(RavenEffect fx)
{
    highp float speed = fx.params.x;
    highp float scale = max(fx.params.y, 0.0500000007450580596923828125);
    highp float intensity = fx.params.z;
    highp float aspect = fx.size.x / max(fx.size.y, 1.0);
    highp float x = (fx.uv.x * aspect) * scale;
    highp float t = (fx.time * 0.07999999821186065673828125) * speed;
    highp vec3 sky = mix(vec3(0.014999999664723873138427734375, 0.0199999995529651641845703125, 0.0500000007450580596923828125), mix(fx.color.xyz, fx.color2.xyz, vec3(0.699999988079071044921875)) * 0.07999999821186065673828125, vec3(1.0 - fx.uv.y));
    highp vec3 light = vec3(0.0);
    highp vec2 m = fx.pointer / max(fx.size, vec2(1.0));
    highp float across = ((fx.uv.x - m.x) * aspect) * 2.2000000476837158203125;
    RavenEffect param = fx;
    highp float _500;
    if (raven_has_pointer(param))
    {
        _500 = fx.hover * exp((-across) * across);
    }
    else
    {
        _500 = 0.0;
    }
    highp float near = _500;
    highp float _591;
    for (int i = 0; i < 3; i++)
    {
        highp float k = float(i);
        highp vec2 param_1 = vec2((x * 1.10000002384185791015625) + (t * (1.2000000476837158203125 + (k * 0.4000000059604644775390625))), k * 4.30000019073486328125);
        highp float _566 = raven_fbm(param_1);
        highp float line = ((0.62000000476837158203125 - (k * 0.12999999523162841796875)) + (0.070000000298023223876953125 * sin(((x * (2.099999904632568359375 + (k * 0.699999988079071044921875))) + (t * (3.0 + k))) + (k * 1.7000000476837158203125)))) + (0.100000001490116119384765625 * (_566 - 0.5));
        line = mix(line, clamp(m.y, 0.1500000059604644775390625, 0.89999997615814208984375), near * (0.3499999940395355224609375 - (k * 0.07999999821186065673828125)));
        highp float d = fx.uv.y - line;
        if (d < 0.0)
        {
            _591 = exp(d * (5.0 + (k * 1.5)));
        }
        else
        {
            _591 = exp((-d) * 40.0);
        }
        highp float curtain = _591;
        highp vec2 param_2 = vec2((x * 9.0) + (k * 13.0), (t * 2.0) + k);
        highp float _625 = raven_fbm(param_2);
        highp float folds = 0.550000011920928955078125 + (0.449999988079071044921875 * _625);
        highp vec3 tone = mix(fx.color.xyz, fx.color2.xyz, vec3(clamp(((-d) * 3.0) + (0.1500000059604644775390625 * k), 0.0, 1.0)));
        light += (((tone * curtain) * folds) * (0.550000011920928955078125 - (k * 0.119999997317790985107421875)));
    }
    highp vec2 cell = floor(fx.pixel / vec2(3.0));
    highp vec2 param_3 = cell;
    highp float _668 = raven_hash(param_3);
    highp vec2 param_4 = cell + vec2(7.0);
    highp float _678 = raven_hash(param_4);
    highp float star = step(0.997500002384185791015625, _668) * (0.5 + (0.5 * sin((fx.time * 2.0) + (_678 * 30.0))));
    light *= (1.0 + (near * (0.89999997615814208984375 + (fx.press * 1.2000000476837158203125))));
    highp vec3 color = (sky + (light * intensity)) + (vec3(star * 0.60000002384185791015625) * (1.0 - clamp(length(light) * 2.0, 0.0, 1.0)));
    highp vec2 param_5 = fx.pixel + vec2(fract(t) * 91.0);
    highp float _721 = raven_hash(param_5);
    color += vec3((_721 - 0.5) / 255.0);
    return vec4(color, 1.0);
}

void main()
{
    highp vec2 p = gl_FragCoord.xy;
    bool _233 = p.x < v_clip.x;
    bool _242;
    if (!_233)
    {
        _242 = p.y < v_clip.y;
    }
    else
    {
        _242 = _233;
    }
    bool _251;
    if (!_242)
    {
        _251 = p.x >= v_clip.z;
    }
    else
    {
        _251 = _242;
    }
    bool _260;
    if (!_251)
    {
        _260 = p.y >= v_clip.w;
    }
    else
    {
        _260 = _251;
    }
    if (_260)
    {
        discard;
    }
    highp vec2 size = max(v_rect.zw - v_rect.xy, vec2(1.0));
    highp vec2 center = (v_rect.xy + v_rect.zw) * 0.5;
    highp vec2 param = p - center;
    highp vec2 param_1 = size * 0.5;
    highp vec4 param_2 = v_radii;
    highp float d = raven_sd_round_box(param, param_1, param_2);
    highp float coverage = clamp(0.5 - d, 0.0, 1.0);
    if (coverage <= 0.0)
    {
        discard;
    }
    RavenEffect fx;
    fx.pixel = p - v_rect.xy;
    fx.uv = fx.pixel / size;
    fx.size = size;
    fx.screen = p;
    fx.edge = d;
    fx.time = v_border_width;
    fx.pointer = v_pointer.xy - v_rect.xy;
    fx.hover = v_pointer.z;
    fx.press = v_pointer.w;
    highp vec4 _343;
    if (v_color.w > 0.0)
    {
        _343 = vec4(v_color.xyz / vec3(v_color.w), v_color.w);
    }
    else
    {
        _343 = vec4(0.0);
    }
    fx.color = _343;
    highp vec4 _368;
    if (v_border_color.w > 0.0)
    {
        _368 = vec4(v_border_color.xyz / vec3(v_border_color.w), v_border_color.w);
    }
    else
    {
        _368 = vec4(0.0);
    }
    fx.color2 = _368;
    fx.params = v_params;
    RavenEffect param_3 = fx;
    highp vec4 color = raven_effect(param_3);
    color = clamp(color, vec4(0.0), vec4(1.0));
    highp float opacity = float((v_flags >> uint(8)) & 255u) / 255.0;
    rvn_out = vec4(color.xyz * color.w, color.w) * (coverage * opacity);
}

