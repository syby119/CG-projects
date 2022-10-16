#pragma once

#include <iostream>
#include <glm/glm.hpp>

// Plane Equation
// P(x, y, z) = x * normal.x + y * normal.y + z * normal.z + signedDistance = 0 
struct Plane {
	glm::vec3 normal;      // plane normal;
	float signedDistance;  // signed distance from the origin to the plane
public:
	Plane() : normal{ 0.0f, 1.0f, 0.0f }, signedDistance{ 0.0f } { }
	Plane(const glm::vec3& normal, float distance)
		: normal{ normal }, signedDistance{ distance } { }
	Plane(const glm::vec3 point, const glm::vec3& normal)
		: normal{ glm::normalize(normal) }, 
		  signedDistance{ -glm::dot(point, this->normal) } { }

	float getSignedDistanceToPoint(glm::vec3 point) const {
		return glm::dot(normal, point) + signedDistance;
	}
};

inline std::ostream& operator<<(std::ostream& os, const Plane& p) {
	os << "(" << p.normal.x << "," << p.normal.y << "," << p.normal.z << "," << p.signedDistance << ")";
	return os;
}
