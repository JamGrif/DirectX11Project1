#include "Camera.h"

Camera::Camera(float x, float y, float z, float rotation)
{
	m_x = x;
	m_y = y;
	m_z = z;
	m_camera_rotation = rotation;

	m_dx = sin(m_camera_rotation * (XM_PI / 180.0));
	m_dz = cos(m_camera_rotation * (XM_PI / 180.0));
}

void Camera::Rotate(float NumOfDegrees)
{
	m_camera_rotation += NumOfDegrees;

	m_dx = sin(m_camera_rotation * (XM_PI / 180.0));
	m_dz = cos(m_camera_rotation * (XM_PI / 180.0));
}

void Camera::Forward(float Distance)
{
	m_x = m_dx * Distance;
	m_z = m_dz * Distance;
}

void Camera::Up()
{

}

XMMATRIX Camera::GetViewMatrix()
{
	m_position = XMVectorSet(m_x, m_y, m_z, 0.0);
	m_lookat = XMVectorSet(m_x + m_dx, m_y, m_z + m_dz, 0.0);
	m_up = XMVectorSet(0.0, 1.0, 0.0, 0.0);

	return XMMatrixLookAtLH(m_position, m_lookat, m_up);
}
