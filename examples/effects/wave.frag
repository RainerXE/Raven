#version 450
#extension GL_GOOGLE_include_directive : require
#include "raven_effect.glsl"

/* An effect of the application: waves of the two colors, rising.
 * Compiled with: glslangValidator -V -I<raven>/include/raven/shaders wave.frag -o wave.spv */
vec4 raven_effect(RavenEffect fx) {
	float wave = sin(fx.uv.x * 9.0 + fx.time * 2.0) * 0.04;
	float band = fract((fx.uv.y + wave) * 6.0 - fx.time * 0.3);
	float stripe = smoothstep(0.0, 0.5, band) * smoothstep(1.0, 0.5, band);
	return mix(fx.color, fx.color2, stripe);
}
