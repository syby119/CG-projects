#pragma once

#include <string>
#include <vector>
#include <glad/glad.h>

#include "model.h"

class InstancedModel: public Model {
public:
	InstancedModel(const std::string& filepath, 
                   const std::vector<glm::mat4>& modelMatrices);

	InstancedModel(InstancedModel&& rhs) noexcept;

	~InstancedModel();

	int getInstanceCount() const;

	glm::mat4 getModelMatrix(int index) const;

	const std::vector<glm::mat4>& getModelMatrices() const;

	void draw() const override;

	void draw(int amount) const;

	void drawBoundingBox() const override;

	void drawBoundingBox(int amount) const;

	GLuint getInstacenVbo() const;
private:
	std::vector<glm::mat4> _modelMatrices;
	GLuint _instanceVbo = {};
};