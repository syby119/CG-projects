#pragma once

#include <limits>
#include <glm/glm.hpp>

struct BoundingBox {
	glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 max = -glm::vec3(std::numeric_limits<float>::max());

	BoundingBox& operator+=(const BoundingBox& rhs) {
		min = glm::min(min, rhs.min);
		max = glm::max(max, rhs.max);

		return *this;
	}
};