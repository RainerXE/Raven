#version 450

/*
 * Raven - backdrop of the glass primitives.
 *
 * A wide blur is built cheaply with the "dual filtering" scheme: the frame is
 * reduced by half a few times (mode 0), then enlarged back (mode 1). Every
 * pass reads a handful of bilinear samples, so the cost stays small whatever
 * the blur radius. Mode 2 is a plain copy, used to show the finished frame.
 */

layout(set = 0, binding = 0) uniform sampler2D u_source;

layout(push_constant) uniform PushConstants {
	/* size of one texel of the source, times the spread of the filter */
	vec4 p;
} pc;

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

void main() {
	if (int(pc.p.z + 0.5) == 0) {
		/* reduce: the center and the four diagonals */
		vec4 sum = textureLod(u_source, v_uv, 0.0) * 4.0;
		sum += textureLod(u_source, v_uv + vec2(-pc.p.xy.x, -pc.p.xy.y), 0.0);
		sum += textureLod(u_source, v_uv + vec2( pc.p.xy.x, -pc.p.xy.y), 0.0);
		sum += textureLod(u_source, v_uv + vec2(-pc.p.xy.x,  pc.p.xy.y), 0.0);
		sum += textureLod(u_source, v_uv + vec2( pc.p.xy.x,  pc.p.xy.y), 0.0);
		out_color = sum / 8.0;
	} else if (int(pc.p.z + 0.5) == 1) {
		/* enlarge: a ring of eight samples */
		vec4 sum = textureLod(u_source, v_uv + vec2(-pc.p.xy.x * 2.0, 0.0), 0.0);
		sum += textureLod(u_source, v_uv + vec2( pc.p.xy.x * 2.0, 0.0), 0.0);
		sum += textureLod(u_source, v_uv + vec2(0.0, -pc.p.xy.y * 2.0), 0.0);
		sum += textureLod(u_source, v_uv + vec2(0.0,  pc.p.xy.y * 2.0), 0.0);
		sum += textureLod(u_source, v_uv + vec2(-pc.p.xy.x,  pc.p.xy.y), 0.0) * 2.0;
		sum += textureLod(u_source, v_uv + vec2( pc.p.xy.x,  pc.p.xy.y), 0.0) * 2.0;
		sum += textureLod(u_source, v_uv + vec2(-pc.p.xy.x, -pc.p.xy.y), 0.0) * 2.0;
		sum += textureLod(u_source, v_uv + vec2( pc.p.xy.x, -pc.p.xy.y), 0.0) * 2.0;
		out_color = sum / 12.0;
	} else {
		out_color = textureLod(u_source, v_uv, 0.0);
	}
}
