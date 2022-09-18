#include "fullscreen_quad.h"

FullscreenQuad::FullscreenQuad() {
	float _vertices[] = {
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};

	glGenVertexArrays(1, &_vao);
	glGenBuffers(1, &_vbo);

	glBindVertexArray(_vao);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(_vertices), &_vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 
						  reinterpret_cast<float*>(2 * sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

FullscreenQuad::FullscreenQuad(FullscreenQuad&& rhs) noexcept
	: _vao(rhs._vao), _vbo(rhs._vbo) {
	rhs._vao = 0;
	rhs._vbo = 0;
}

FullscreenQuad::~FullscreenQuad() {
	if (_vao) {
		glDeleteVertexArrays(1, &_vao);
		_vao = 0;
	}

	if (_vbo) {
		glDeleteBuffers(1, &_vbo);
		_vbo = 0;
	}
}

void FullscreenQuad::draw() const {
	glBindVertexArray(_vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}