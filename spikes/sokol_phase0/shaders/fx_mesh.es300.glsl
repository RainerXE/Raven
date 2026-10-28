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

bool raven_has_pointer(RavenEffect fx)
{
    return fx.pointer.x > (-100000.0);
}

highp vec4 raven_effect(RavenEffect fx)
{
    highp float t = (fx.time * 0.20000000298023223876953125) * fx.params.x;
    highp vec3 a = fx.color.xyz;
    highp vec3 b = fx.color2.xyz;
    highp vec3 c = clamp(mix(a, b, vec3(0.5)).yzx * 1.10000002384185791015625, vec3(0.0), vec3(1.0));
    highp vec3 d = clamp((mix(a, b, vec3(0.300000011920928955078125)) * 0.64999997615814208984375) + vec3(0.119999997317790985107421875), vec3(0.0), vec3(1.0));
    highp vec2 uv = fx.uv;
    uv += (vec2(sin((uv.y * 3.0) + (t * 2.0)), cos((uv.x * 3.0) - (t * 1.7000000476837158203125))) * 0.119999997317790985107421875);
    highp vec2 param = (uv * 2.0) + vec2(t);
    highp float _496 = raven_fbm(param);
    highp vec2 param_1 = ((uv * 2.0) - vec2(t)) + vec2(5.0);
    highp float _506 = raven_fbm(param_1);
    uv += ((vec2(_496, _506) * 0.0599999986588954925537109375) - vec2(0.02999999932944774627685546875));
    highp float glow = 0.0;
    RavenEffect param_2 = fx;
    if (raven_has_pointer(param_2))
    {
        highp vec2 m = fx.pointer / max(fx.size, vec2(1.0));
        highp vec2 to = uv - m;
        highp float pull = exp((-dot(to, to)) * 7.0) * fx.hover;
        uv -= ((to * pull) * (0.449999988079071044921875 + (0.25 * fx.press)));
        glow = exp((-dot(fx.pixel - fx.pointer, fx.pixel - fx.pointer)) / pow(0.3499999940395355224609375 * min(fx.size.x, fx.size.y), 2.0)) * fx.hover;
    }
    uv = clamp(uv, vec2(0.0), vec2(1.0));
    highp vec2 s = (uv * uv) * (vec2(3.0) - (uv * 2.0));
    highp vec3 color = mix(mix(a, c, vec3(s.x)), mix(d, b, vec3(s.x)), vec3(s.y));
    color = mix(color, min((color * 1.25) + vec3(0.100000001490116119384765625), vec3(1.0)), vec3(glow * 0.60000002384185791015625));
    highp vec2 param_3 = fx.pixel + vec2(t);
    highp float _629 = raven_hash(param_3);
    color += vec3((_629 - 0.5) / 255.0);
    return vec4(color, mix(fx.color.w, fx.color2.w, uv.y));
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

