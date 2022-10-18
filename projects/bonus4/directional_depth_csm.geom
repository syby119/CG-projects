#version 400 core
#define MATRIX_COUNT 5

layout(triangles, invocations = MATRIX_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

uniform mat4 lightSpaceMatrices[MATRIX_COUNT];

void main() {
	for (int i = 0; i < 3; ++i) {
		gl_Position = lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID;
		EmitVertex();
	}
	EndPrimitive();
}