#pragma once

#include <glm/glm.hpp>
#include "primitive.h"

struct RenderObject {
	glm::mat4 globalMatrix;
	const Primitive* primitive;
};