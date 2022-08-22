#include "camera.h"

glm::mat4 Camera::getViewMatrix() const {
	return glm::lookAt(position, position + getFront(), getUp());
}


PerspectiveCamera::PerspectiveCamera(float fovy, float aspect, float znear, float zfar)
	: fovy(fovy), aspect(aspect), znear(znear), zfar(zfar) { }

glm::mat4 PerspectiveCamera::getProjectionMatrix() const {
	return glm::perspective(fovy, aspect, znear, zfar);
}

Frustum PerspectiveCamera::getFrustum() const {
	Frustum frustum;
	// TODO: construct the frustum with the position and orientation of the camera
	// Note: this is for Bonus project 'Frustum Culling'
	// write your code here
	// ----------------------------------------------------------------------
	// ...
	// ----------------------------------------------------------------------

	return frustum;
}

OrthographicCamera::OrthographicCamera(
	float left, float right, float bottom, float top, float znear, float zfar)
	: left(left), right(right), top(top), bottom(bottom), znear(znear), zfar(zfar) { }

glm::mat4 OrthographicCamera::getProjectionMatrix() const {
	return glm::ortho(left, right, bottom, top, znear, zfar);
}

Frustum OrthographicCamera::getFrustum() const {
	Frustum frustum;
	const glm::vec3 fv = getFront();
	const glm::vec3 rv = getRight();
	const glm::vec3 uv = getUp();

	// all of the plane normal points inside the frustum, maybe it's a convention
	frustum.planes[Frustum::NearFace] = { position + znear * fv, fv };
	frustum.planes[Frustum::FarFace] = { position + zfar * fv, -fv };
	frustum.planes[Frustum::LeftFace] = { position - right * rv , rv };
	frustum.planes[Frustum::RightFace] = { position + right * rv , -rv };
	frustum.planes[Frustum::BottomFace] = { position - bottom * uv , uv };
	frustum.planes[Frustum::TopFace] = { position + top * uv , -uv };

	return frustum;
}