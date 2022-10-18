#version 330 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

out vec4 fPosition;

uniform mat4 lightSpaceMatrices[6];

void main() {
	for (int i = 0; i < 6; ++i) {;
		gl_Layer = i;
		for (int j = 0; j < 3; ++j) {
			fPosition = gl_in[j].gl_Position;
			gl_Position = lightSpaceMatrices[i] * fPosition;
			EmitVertex();
		}
		EndPrimitive();
	}
}