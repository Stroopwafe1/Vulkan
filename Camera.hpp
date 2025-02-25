#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/vec3.hpp>
#include <glm/ext/matrix_float4x4.hpp>

class Camera {
public:
	glm::vec3 m_Position = glm::vec3(0, 1.38, 1.32);
	glm::vec3 m_Front = glm::vec3(0.06, -0.26, -0.96);
	glm::vec3 m_Up = glm::vec3(0.0f, 1.0f, 0.0f);
	float m_Yaw = -85.32;
	float m_Pitch = -17.5714;
	float m_FOV{};
	float m_Aspect{};
	glm::mat4 m_View{};
	glm::mat4 m_Projection{};

	Camera() = delete;
	Camera(float fov, float aspect);

	void SetFOV(float degrees);
	void SetAspect(float aspect);
};

#endif // CAMERA_HPP
