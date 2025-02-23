#ifndef TOWNSHIP_CAMERA_HPP
#define TOWNSHIP_CAMERA_HPP

#include <glm/vec3.hpp>
#include <glm/ext/matrix_float4x4.hpp>

namespace Township {

	class Camera {
	public:
		glm::vec3 m_Position = glm::vec3(-12.4414, 11.3955, 0.64625);
		glm::vec3 m_Front = glm::vec3(0.849233, -0.36502, 0.381528);
		glm::vec3 m_Up = glm::vec3(0.0f, 1.0f, 0.0f);
		float m_Yaw = 24.1926;
		float m_Pitch = -21.4088;
		float m_FOV{};
		float m_Aspect{};
		glm::mat4 m_View{};
		glm::mat4 m_Projection{};

		Camera() = delete;
		Camera(float fov, float aspect);

		void SetFOV(float degrees);
		void SetAspect(float aspect);
	};

} // Township

#endif //TOWNSHIP_CAMERA_HPP
