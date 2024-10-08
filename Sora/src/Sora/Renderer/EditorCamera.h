#pragma once

#include "Camera.h"
#include "Sora/Core/Timestep.h"
#include "Sora/Events/Event.h"
#include "Sora/Events/MouseEvent.h"

#include <glm/glm.hpp>

namespace Sora {

	class EditorCamera : public Camera
	{
	public:
		EditorCamera() = default;
		EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		

		inline void SetViewportSize(float width, float height) { m_ViewportWidth = width; m_ViewportHeight = height; UpdateProjection(); }

		const	glm::mat4&	GetViewMatrix()			const { return m_ViewMatrix; }
				glm::mat4	GetViewProjection()		const { return m_Projection * m_ViewMatrix; }

				glm::vec3	GetUpDirection()		const;
				glm::vec3	GetRightDirection()		const;
				glm::vec3	GetFowardDirection()	const;

		const	glm::vec3&	GetPosition()			const { return m_Position; }
		const	glm::vec3&	GetFocalPoint()			const { return m_FocalPoint; }
				glm::quat	GetOrientation()		const;

		inline	float		GetDistance()			const { return m_Distance; }
				float		GetPitch()				const { return m_Pitch; }
				float		GetYaw()				const { return m_Yaw; }
				float		GetFOV()				const { return m_FOV; }
				float		GetAspectRatio()		const { return m_AspectRatio; }
				float		GetNearClip()			const { return m_NearClip; }
				float		GetFarClip()			const { return m_FarClip; }

				void		SetFocalPoint(const glm::vec3& focalPoint)	{ m_FocalPoint = focalPoint; }

				void		SetDistance(float distance)					{ m_Distance = distance; }
				void		SetPitch(float pitch)						{ m_Pitch = pitch; }
				void		SetYaw(float yaw)							{ m_Yaw = yaw; }
	private:
		void UpdateProjection();
		void UpdateView();

		bool OnMouseScroll(MouseScrolledEvent& e);

		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);

		glm::vec3 CalculatePosition() const;

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;
	private:
		float m_FOV = 45.0f, m_AspectRatio = 1.778f, m_NearClip = 0.1f, m_FarClip = 1000.0f;

		glm::mat4 m_ViewMatrix;
		glm::vec3 m_Position	= { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_FocalPoint	= { 0.0f, 0.0f, 0.0f };

		glm::vec2 m_InitialMousePosition;

		float m_Distance = 10.0f;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;

		float m_ViewportWidth = 1600, m_ViewportHeight = 900;
	};

}