#version 460 core
#define GRADIENT_COUNT 8
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

vec2 gradients[GRADIENT_COUNT] = {
	vec2(1, 1),
	vec2(-1, 1),
	vec2(1, -1),
	vec2(-1, -1),
	vec2(1, 1),
	vec2(-1, 1),
	vec2(-1, -1),
	vec2(1, 1),
};

int table[GRADIENT_COUNT] = {
7, 1, 3,2, 4,6,0,5
};

// i, j and k are integers representing the corners of the current lattice cell
float f(float t)
{
	t = abs(t);
	if(t > 1) return 0;

	float tcubed = t * t * t;
	return -6.0 * t * t * tcubed + 15 *t * tcubed - 10.0f * tcubed + 1;
}

float getContribution(int i, int j, vec2 p)
{
	int idx = table[abs(i) % GRADIENT_COUNT];
	idx = table[abs(j + idx) % GRADIENT_COUNT];
	
	// Compute dx, dy, dx
	float dx = p.x - i;
	float dz = p.y - j;
	vec2 d = vec2(dx, dz);

	float c = f(dx) * f(dz) * dot(gradients[idx], d);
	return c;
}
float perlin2d(vec2 p)
{
	int i = int(floor(p.x));
	int j = int(floor(p.y));

	float cLeftBottom = getContribution(i,j, p);
	float cLeftTop = getContribution(i, j+1, p);
	float cRightTop = getContribution(i+1, j+1, p);
	float cRightBottom = getContribution(i+1, j, p);

	return (cLeftBottom + cLeftTop+ cRightBottom + cRightTop + 1) / 2.0f;
}

float fBmPerlin2d(vec2 p, int octaveCount, float gain, float persistance)
{
	float sum = 0.0f;
	float amp = 1.0f;
	float freq = 1.0f;

	for(int i=0; i < octaveCount; i++)
	{
		sum += perlin2d(p * freq) * amp;
		freq *= gain;
		amp *= persistance;
	}
	return sum/float(octaveCount);
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
	// 4oct 2gain 0.5pers -> nice mountains
	float noise = fBmPerlin2d(wpos.xz * 0.25f, 5, 1.4f, 0.35f);
	//float noise = perlin2d(wpos.xz * 0.25f);
	wpos.y = noise * gs_in[0].heightFactor;

	gl_Position = gs_in[0].projection * gs_in[0].viewing * wpos;
	fragWorldPosG = wpos;
	fragWorldNorG = gs_in[0].fragWorldNor;	
	maxHeight = gs_in[0].heightFactor;
}

