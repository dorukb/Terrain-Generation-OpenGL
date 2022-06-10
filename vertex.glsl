#version 460 core

uniform mat4 modelMat;
uniform mat4 viewingMatrix;
uniform mat4 projectionMatrix;
uniform int widthParam;
uniform int sampleCount;
uniform float height;

layout(location=0) in vec3 inVertex;

out VS_OUT {
    vec4 fragWorldPos;
	vec3 fragWorldNor;
	mat4 projection;
	mat4 viewing;
	float stepSize;
	float heightFactor;

} vs_out;

void main(void)
{
	// Compute the world coordinates of the vertex and its normal.
	// These coordinates will be interpolated during the rasterization
	// stage and the fragment shader will receive the interpolated
	// coordinates.

	float stepSize = (widthParam * 2.0f) / sampleCount;
	int startOffset = -widthParam;

	float xOffset = startOffset + (gl_InstanceID % sampleCount) * stepSize;
	float zOffset = startOffset + (gl_InstanceID / sampleCount) * stepSize;

	vec3 offset = vec3(xOffset, 0.0, -zOffset);
	vs_out.fragWorldPos = modelMat * vec4(inVertex + offset, 1);
	vs_out.fragWorldNor = inverse(transpose(mat3x3(modelMat))) * vec3(0.0, 1.0, 0.0);
	vs_out.stepSize = stepSize;

	vs_out.projection = projectionMatrix;
	vs_out.viewing = viewingMatrix;
	vs_out.heightFactor = height;

	gl_Position = projectionMatrix * viewingMatrix * modelMat * vec4(inVertex+offset, 1);
}

