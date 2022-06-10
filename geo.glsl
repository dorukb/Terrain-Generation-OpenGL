#version 460 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

out vec4 fragWorldPosG;
out vec3 fragWorldNorG;
out float maxHeight;

in VS_OUT {
    vec4 fragWorldPos;
	vec3 fragWorldNor;
	mat4 projection;
	mat4 viewing;
	float stepSize;	
	float heightFactor;
} gs_in[];

vec2 gradients[4] = {
	vec2(1, 1),
	vec2(-1, 1),
	vec2(1, -1),
	vec2(-1, -1),
};

int table[4] = {
0, 1, 2, 3,
};

// i, j and k are integers representing the corners of the current lattice cell
vec2 fade(vec2 t) {return t*t*t*(t*(t*6.0-15.0)+10.0);}
vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}

float f(float t)
{
	t = abs(t);
	float tcubed = t * t * t;
	return -6.0 * t * t * tcubed + 15 *t * tcubed - 10 * tcubed + 1;
}

float perlin2d(vec2 p)
{
	int idx,i,j;
	idx = table[abs(i) % 16];
	idx = table[abs(j + idx) % 16];
	vec2 g = gradients[idx];

	return 0;
}

float cnoise(vec2 P){
  vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
  vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);
  Pi = mod(Pi, 289.0); // To avoid truncation effects in permutation
  vec4 ix = Pi.xzxz;
  vec4 iy = Pi.yyww;
  vec4 fx = Pf.xzxz;
  vec4 fy = Pf.yyww;
  vec4 i = permute(permute(ix) + iy);
  vec4 gx = 2.0 * fract(i * 0.0243902439) - 1.0; // 1/41 = 0.024...
  vec4 gy = abs(gx) - 0.5;
  vec4 tx = floor(gx + 0.5);
  gx = gx - tx;
  vec2 g00 = vec2(gx.x,gy.x);
  vec2 g10 = vec2(gx.y,gy.y);
  vec2 g01 = vec2(gx.z,gy.z);
  vec2 g11 = vec2(gx.w,gy.w);
  vec4 norm = 1.79284291400159 - 0.85373472095314 * 
    vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11));
  g00 *= norm.x;
  g01 *= norm.y;
  g10 *= norm.z;
  g11 *= norm.w;
  float n00 = dot(g00, vec2(fx.x, fy.x));
  float n10 = dot(g10, vec2(fx.y, fy.y));
  float n01 = dot(g01, vec2(fx.z, fy.z));
  float n11 = dot(g11, vec2(fx.w, fy.w));
  vec2 fade_xy = fade(Pf.xy);
  vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
  float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
  return 2.3 * n_xy;
}

void computePositionAndSetOutputs(vec4 offset);

void main() {

	float zStepSize = gs_in[0].stepSize;


	vec4 offset = vec4(0.0, 0.0, 0.0, 0.0);
	computePositionAndSetOutputs(offset);
	EmitVertex();
	
	// bottom right corner, 2nd vertex
	offset = vec4(0.0, 0.0, zStepSize, 0.0);
	computePositionAndSetOutputs(offset);
	EmitVertex();

	// upper left vertex, 3rd vertex
	offset = vec4(zStepSize, 0.0, 0.0, 0.0);
	computePositionAndSetOutputs(offset);
	EmitVertex();

	// upper right vertex, 4th vertex
	offset = vec4(zStepSize, 0.0, zStepSize, 0.0);
	computePositionAndSetOutputs(offset);
	EmitVertex();

	EndPrimitive();
}

void computePositionAndSetOutputs(vec4 offset)
{
	vec4 wpos = gs_in[0].fragWorldPos + offset;
	wpos.y = (cnoise(wpos.xz) +1) /2 * gs_in[0].heightFactor;

	gl_Position = gs_in[0].projection * gs_in[0].viewing * wpos;
	fragWorldPosG = wpos;
	fragWorldNorG = gs_in[0].fragWorldNor;	
	maxHeight = gs_in[0].heightFactor;
}

