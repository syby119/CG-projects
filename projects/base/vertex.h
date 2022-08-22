#pragma once

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;

	bool operator==(const Vertex& v) const {
		return (position == v.position) && 
			   (normal == v.normal) && 
			   (texCoord == v.texCoord);
	}
};

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace std {
	template<>
	struct hash<Vertex> {
		size_t operator()(const Vertex& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}