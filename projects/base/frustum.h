#pragma once

#include <iostream>
#include "plane.h"
#include "bounding_box.h"

struct Frustum {
public:
	Plane planes[6];
	enum {
		LeftFace = 0,
		RightFace = 1,
		BottomFace = 2,
		TopFace = 3,
		NearFace = 4,
		FarFace = 5
	};

	bool intersect(const BoundingBox& aabb, const glm::mat4& modelMatrix) const {
		// TODO: judge whether the frustum intersects the bounding box
		// write your code here
		// ------------------------------------------------------------
		 return true;
		// ------------------------------------------------------------
	}
};

inline std::ostream& operator<<(std::ostream& os, const Frustum& frustum) {
	os << "frustum: \n";
	os << "planes[Left]:   " << frustum.planes[0] << "\n";
	os << "planes[Right]:  " << frustum.planes[1] << "\n";
	os << "planes[Bottom]: " << frustum.planes[2] << "\n";
	os << "planes[Top]:    " << frustum.planes[3] << "\n";
	os << "planes[Near]:   " << frustum.planes[4] << "\n";
	os << "planes[Far]:    " << frustum.planes[5] << "\n";

	return os;
}