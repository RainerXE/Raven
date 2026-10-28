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
    highp float _44;
    if (p.x < 0.0)
    {
        highp float _51;
        if (p.y < 0.0)
        {
            _51 = radii.x;
        }
        else
        {
            _51 = radii.w;
        }
        _44 = _51;
    }
    else
    {
        highp float _65;
        if (p.y < 0.0)
        {
            _65 = radii.y;
        }
        else
        {
            _65 = radii.z;
        }
        _44 = _65;
    }
    highp float radius = _44;
    radius = min(radius, min(half_size.x, half_size.y));
    highp vec2 q = (abs(p) - half_size) + vec2(radius);
    return (min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0)))) - radius;
}

bool raven_has_pointer(RavenEffect fx)
{
    return fx.pointer.x > (-100000.0);
}

highp vec4 raven_effect(RavenEffect fx)
{
    highp float speed = fx.params.x;
    highp float scale = max(fx.params.y, 0.0500000007450580596923828125);
    highp float contrast = max(fx.params.z, 0.100000001490116119384765625);
    highp float aspect = fx.size.x / max(fx.size.y, 1.0);
    highp vec2 p = (((fx.uv - vec2(0.5)) * vec2(aspect, 1.0)) * 5.0) * scale;
    highp float t = (fx.time * 0.60000002384185791015625) * speed;
    RavenEffect param = fx;
    if (raven_has_pointer(param))
    {
        highp vec2 m = ((((fx.pointer / max(fx.size, vec2(1.0))) - vec2(0.5)) * vec2(aspect, 1.0)) * 5.0) * scale;
        highp vec2 r = p - m;
        highp float turn = (fx.hover * (1.60000002384185791015625 + (fx.press * 2.0))) * exp((-dot(r, r)) * 0.3499999940395355224609375);
        highp float cs = cos(turn);
        highp float sn = sin(turn);
        p = m + (mat2(vec2(cs, sn), vec2(-sn, cs)) * r);
    }
    highp float v = sin(p.x + t);
    v += sin((p.y + t) * 0.699999988079071044921875);
    v += sin(((p.x + p.y) + t) * 0.60000002384185791015625);
    v += sin((length(p + (vec2(sin(t * 0.3300000131130218505859375), cos(t * 0.5)) * 2.0)) * 1.39999997615814208984375) - t);
    v = 0.5 + (0.5 * sin((v * 0.800000011920928955078125) * contrast));
    highp vec3 middle = clamp((mix(fx.color.xyz, fx.color2.xyz, vec3(0.5)) * 1.35000002384185791015625) + vec3(0.07999999821186065673828125), vec3(0.0), vec3(1.0));
    highp vec3 _489;
    if (v < 0.5)
    {
        _489 = mix(fx.color.xyz, middle, vec3(v * 2.0));
    }
    else
    {
        _489 = mix(middle, fx.color2.xyz, vec3((v * 2.0) - 1.0));
    }
    highp vec3 color = _489;
    return vec4(color, mix(fx.color.w, fx.color2.w, v));
}

void main()
{
    highp vec2 p = gl_FragCoord.xy;
    bool _117 = p.x < v_clip.x;
    bool _126;
    if (!_117)
    {
        _126 = p.y < v_clip.y;
    }
    else
    {
        _126 = _117;
    }
    bool _135;
    if (!_126)
    {
        _135 = p.x >= v_clip.z;
    }
    else
    {
        _135 = _126;
    }
    bool _144;
    if (!_135)
    {
        _144 = p.y >= v_clip.w;
    }
    else
    {
        _144 = _135;
    }
    if (_144)
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
    highp vec4 _233;
    if (v_color.w > 0.0)
    {
        _233 = vec4(v_color.xyz / vec3(v_color.w), v_color.w);
    }
    else
    {
        _233 = vec4(0.0);
    }
    fx.color = _233;
    highp vec4 _258;
    if (v_border_color.w > 0.0)
    {
        _258 = vec4(v_border_color.xyz / vec3(v_border_color.w), v_border_color.w);
    }
    else
    {
        _258 = vec4(0.0);
    }
    fx.color2 = _258;
    fx.params = v_params;
    RavenEffect param_3 = fx;
    highp vec4 color = raven_effect(param_3);
    color = clamp(color, vec4(0.0), vec4(1.0));
    highp float opacity = float((v_flags >> uint(8)) & 255u) / 255.0;
    rvn_out = vec4(color.xyz * color.w, color.w) * (coverage * opacity);
}

