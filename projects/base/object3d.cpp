#include "object3d.h"

glm::vec3 Object3D::getFront() const {
	constexpr glm::vec3 defaultFront{ 0.0f, 0.0f, -1.0f };
	return rotation * defaultFront;
}

glm::vec3 Object3D::getUp() const{
	constexpr glm::vec3 defaultUp{ 0.0f, 1.0f, 0.0f };
	return rotation * defaultUp;
}

glm::vec3 Object3D::getRight() const{
	constexpr glm::vec3 defaultRight{ 1.0f, 0.0f, 0.0f };
	return rotation * defaultRight;
}

glm::mat4 Object3D::getModelMatrix() const {
	return glm::translate(glm::mat4(1.0f), position) *
		glm::mat4_cast(rotation) *
		glm::scale(glm::mat4(1.0f), scale);
}