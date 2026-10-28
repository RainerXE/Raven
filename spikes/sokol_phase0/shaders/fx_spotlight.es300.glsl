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
    highp float _72;
    if (p.x < 0.0)
    {
        highp float _78;
        if (p.y < 0.0)
        {
            _78 = radii.x;
        }
        else
        {
            _78 = radii.w;
        }
        _72 = _78;
    }
    else
    {
        highp float _92;
        if (p.y < 0.0)
        {
            _92 = radii.y;
        }
        else
        {
            _92 = radii.z;
        }
        _72 = _92;
    }
    highp float radius = _72;
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

highp vec4 raven_effect(RavenEffect fx)
{
    highp vec2 at = vec2(fx.params.x, fx.params.y) * fx.size;
    highp float radius = max(fx.params.z, 0.00999999977648258209228515625) * max(fx.size.x, fx.size.y);
    highp float boost = 1.0;
    RavenEffect param = fx;
    if (raven_has_pointer(param))
    {
        at = mix(at, fx.pointer, vec2(fx.hover));
        radius *= (mix(1.0, 0.550000011920928955078125, fx.hover) * (1.0 - (0.20000000298023223876953125 * fx.press)));
        boost = (1.0 + (0.5 * fx.hover)) + (0.5 * fx.press);
    }
    highp float d = length(fx.pixel - at) / radius;
    highp float light = exp(((-d) * d) * 2.2000000476837158203125);
    highp vec3 color = fx.color2.xyz + (((fx.color.xyz * light) * fx.color.w) * boost);
    highp vec2 param_1 = fx.pixel;
    highp float _430 = raven_hash(param_1);
    color += vec3((_430 - 0.5) / 255.0);
    return vec4(color, fx.color2.w);
}

void main()
{
    highp vec2 p = gl_FragCoord.xy;
    bool _144 = p.x < v_clip.x;
    bool _153;
    if (!_144)
    {
        _153 = p.y < v_clip.y;
    }
    else
    {
        _153 = _144;
    }
    bool _162;
    if (!_153)
    {
        _162 = p.x >= v_clip.z;
    }
    else
    {
        _162 = _153;
    }
    bool _171;
    if (!_162)
    {
        _171 = p.y >= v_clip.w;
    }
    else
    {
        _171 = _162;
    }
    if (_171)
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
    highp vec4 _260;
    if (v_color.w > 0.0)
    {
        _260 = vec4(v_color.xyz / vec3(v_color.w), v_color.w);
    }
    else
    {
        _260 = vec4(0.0);
    }
    fx.color = _260;
    highp vec4 _285;
    if (v_border_color.w > 0.0)
    {
        _285 = vec4(v_border_color.xyz / vec3(v_border_color.w), v_border_color.w);
    }
    else
    {
        _285 = vec4(0.0);
    }
    fx.color2 = _285;
    fx.params = v_params;
    RavenEffect param_3 = fx;
    highp vec4 color = raven_effect(param_3);
    color = clamp(color, vec4(0.0), vec4(1.0));
    highp float opacity = float((v_flags >> uint(8)) & 255u) / 255.0;
    rvn_out = vec4(color.xyz * color.w, color.w) * (coverage * opacity);
}

