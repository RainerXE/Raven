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
    highp float _61;
    if (p.x < 0.0)
    {
        highp float _68;
        if (p.y < 0.0)
        {
            _68 = radii.x;
        }
        else
        {
            _68 = radii.w;
        }
        _61 = _68;
    }
    else
    {
        highp float _82;
        if (p.y < 0.0)
        {
            _82 = radii.y;
        }
        else
        {
            _82 = radii.z;
        }
        _61 = _82;
    }
    highp float radius = _61;
    radius = min(radius, min(half_size.x, half_size.y));
    highp vec2 q = (abs(p) - half_size) + vec2(radius);
    return (min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0)))) - radius;
}

bool raven_has_pointer(RavenEffect fx)
{
    return fx.pointer.x > (-100000.0);
}

highp vec4 raven_backdrop(highp vec2 screen)
{
    return textureLod(rvn_backdrop, screen * v_inv_size, 0.0);
}

highp vec4 raven_effect(RavenEffect fx)
{
    highp float speed = fx.params.x;
    highp float strength = fx.params.y;
    highp float density = max(fx.params.z, 0.100000001490116119384765625);
    highp vec2 center = vec2(fx.params.w, 0.5) * fx.size;
    RavenEffect param = fx;
    if (raven_has_pointer(param))
    {
        center = mix(center, fx.pointer, vec2(fx.hover));
        strength *= (1.0 + (fx.press * 1.5));
    }
    highp vec2 delta = fx.pixel - center;
    highp float dist = length(delta);
    highp vec2 _380;
    if (dist > 0.0)
    {
        _380 = delta / vec2(dist);
    }
    else
    {
        _380 = vec2(0.0);
    }
    highp vec2 direction = _380;
    highp float scale = min(fx.size.x, fx.size.y);
    highp float phase = (((dist / scale) * density) * 6.0) - ((fx.time * speed) * 4.0);
    highp float fade = exp(((-dist) / scale) * 1.2000000476837158203125) * (0.60000002384185791015625 + (0.4000000059604644775390625 * sin(phase * 0.25)));
    highp float wave = sin(phase) * fade;
    highp vec2 offset = (direction * wave) * strength;
    highp vec2 param_1 = fx.screen + offset;
    highp vec3 seen = raven_backdrop(param_1).xyz;
    highp float crest = max(cos(phase), 0.0) * fade;
    highp float lit = (1.0 + (fx.hover * 2.0)) + (fx.press * 2.0);
    highp vec3 color = (mix(seen, fx.color.xyz, vec3(fx.color.w)) + vec3((crest * 0.07999999821186065673828125) * lit)) + (((fx.color.xyz * fx.hover) * 0.14000000059604644775390625) * exp(((-dist) / scale) * 3.0));
    return vec4(color, 1.0);
}

void main()
{
    highp vec2 p = gl_FragCoord.xy;
    bool _134 = p.x < v_clip.x;
    bool _143;
    if (!_134)
    {
        _143 = p.y < v_clip.y;
    }
    else
    {
        _143 = _134;
    }
    bool _152;
    if (!_143)
    {
        _152 = p.x >= v_clip.z;
    }
    else
    {
        _152 = _143;
    }
    bool _161;
    if (!_152)
    {
        _161 = p.y >= v_clip.w;
    }
    else
    {
        _161 = _152;
    }
    if (_161)
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
    highp vec4 _250;
    if (v_color.w > 0.0)
    {
        _250 = vec4(v_color.xyz / vec3(v_color.w), v_color.w);
    }
    else
    {
        _250 = vec4(0.0);
    }
    fx.color = _250;
    highp vec4 _275;
    if (v_border_color.w > 0.0)
    {
        _275 = vec4(v_border_color.xyz / vec3(v_border_color.w), v_border_color.w);
    }
    else
    {
        _275 = vec4(0.0);
    }
    fx.color2 = _275;
    fx.params = v_params;
    RavenEffect param_3 = fx;
    highp vec4 color = raven_effect(param_3);
    color = clamp(color, vec4(0.0), vec4(1.0));
    highp float opacity = float((v_flags >> uint(8)) & 255u) / 255.0;
    rvn_out = vec4(color.xyz * color.w, color.w) * (coverage * opacity);
}

