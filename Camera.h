#pragma once
#include <d3d11.h>
#include <math.h>

#define _XM_NO_INTRINSICS_
#define XM_NO_ALGINMENT
#include <DirectXMath.h>
using namespace DirectX;


class Camera 
{
public:
	Camera(float x, float y, float z, float rotation);

	void Rotate(float NumOfDegrees);
	void Forward(float Distance);
	void Up();

	XMMATRIX GetViewMatrix();

private:
	float m_x, m_y, m_z, m_dx, m_dz, m_camera_rotation;
	XMVECTOR m_position, m_lookat, m_up;



};
