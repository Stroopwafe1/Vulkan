#include <glm/ext/matrix_clip_space.hpp>
#include <iostream>
#include "Camera.hpp"

namespace Township {
	Camera::Camera(float fov, float aspect) {
		m_FOV = fov;
		m_Aspect = aspect;
		m_View = glm::mat4(1.0f);
		m_Projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f,190.0f);
	}

	void Camera::SetFOV(float fov) {
		m_Projection = glm::perspective(glm::radians(fov), m_Aspect, 0.1f, 190.0f);
		m_FOV = fov;
	}

	void Camera::SetAspect(float aspect) {
		m_Projection = glm::perspective(glm::radians(m_FOV), aspect, 0.1f, 190.0f);
		m_Aspect = aspect;
	}
} // Township