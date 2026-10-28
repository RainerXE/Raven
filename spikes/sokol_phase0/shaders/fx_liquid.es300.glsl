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

uniform highp sampler2D rvn_backdrop;
uniform highp sampler2D rvn_texture;
uniform highp sampler2D rvn_backdrop_blur;

flat in highp vec2 v_inv_size;
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

highp float raven_sd_round_box(highp vec2 p, highp vec2 half_size, highp vec4 radii)
{
    highp float _167;
    if (p.x < 0.0)
    {
        highp float _173;
        if (p.y < 0.0)
        {
            _173 = radii.x;
        }
        else
        {
            _173 = radii.w;
        }
        _167 = _173;
    }
    else
    {
        highp float _187;
        if (p.y < 0.0)
        {
            _187 = radii.y;
        }
        else
        {
            _187 = radii.z;
        }
        _167 = _187;
    }
    highp float radius = _167;
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
    highp float _97 = raven_hash(param);
    highp vec2 param_1 = i + vec2(1.0, 0.0);
    highp float _103 = raven_hash(param_1);
    highp vec2 param_2 = i + vec2(0.0, 1.0);
    highp float _111 = raven_hash(param_2);
    highp vec2 param_3 = i + vec2(1.0);
    highp float _116 = raven_hash(param_3);
    return mix(mix(_97, _103, u.x), mix(_111, _116, u.x), u.y);
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

highp vec4 raven_backdrop(highp vec2 screen)
{
    return textureLod(rvn_backdrop, screen * v_inv_size, 0.0);
}

highp vec4 raven_effect(RavenEffect fx)
{
    highp float _436;
    if (fx.params.x > 0.0)
    {
        _436 = fx.params.x;
    }
    else
    {
        _436 = 6.0;
    }
    highp float strength = _436;
    highp vec2 param = (fx.uv * 3.0) + vec2(fx.time * 0.25);
    highp float _455 = raven_fbm(param);
    highp vec2 param_1 = ((fx.uv * 3.0) - vec2(fx.time * 0.20000000298023223876953125)) + vec2(7.0);
    highp float _469 = raven_fbm(param_1);
    highp vec2 flow = vec2(_455, _469) - vec2(0.5);
    highp vec2 param_2 = fx.screen + ((flow * strength) * 2.0);
    highp vec3 seen = raven_backdrop(param_2).xyz;
    highp float rim = smoothstep(-6.0, 0.0, fx.edge);
    return vec4(mix(seen, fx.color.xyz, vec3(fx.color.w)) + vec3(rim * 0.119999997317790985107421875), 1.0);
}

void main()
{
    highp vec2 p = gl_FragCoord.xy;
    bool _239 = p.x < v_clip.x;
    bool _248;
    if (!_239)
    {
        _248 = p.y < v_clip.y;
    }
    else
    {
        _248 = _239;
    }
    bool _257;
    if (!_248)
    {
        _257 = p.x >= v_clip.z;
    }
    else
    {
        _257 = _248;
    }
    bool _266;
    if (!_257)
    {
        _266 = p.y >= v_clip.w;
    }
    else
    {
        _266 = _257;
    }
    if (_266)
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
    highp vec4 _350;
    if (v_color.w > 0.0)
    {
        _350 = vec4(v_color.xyz / vec3(v_color.w), v_color.w);
    }
    else
    {
        _350 = vec4(0.0);
    }
    fx.color = _350;
    highp vec4 _375;
    if (v_border_color.w > 0.0)
    {
        _375 = vec4(v_border_color.xyz / vec3(v_border_color.w), v_border_color.w);
    }
    else
    {
        _375 = vec4(0.0);
    }
    fx.color2 = _375;
    fx.params = v_params;
    RavenEffect param_3 = fx;
    highp vec4 color = raven_effect(param_3);
    color = clamp(color, vec4(0.0), vec4(1.0));
    highp float opacity = float((v_flags >> uint(8)) & 255u) / 255.0;
    rvn_out = vec4(color.xyz * color.w, color.w) * (coverage * opacity);
}

