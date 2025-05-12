//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		3d quaternion camera system
//
// $NoKeywords: $cam
//===============================================================================//

#include "Camera.h"

#include "ConVar.h"
#include "Engine.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

ConVar cl_pitchup("cl_pitchup", 89.0f, FCVAR_CHEAT);
ConVar cl_pitchdown("cl_pitchdown", 89.0f, FCVAR_CHEAT);

Matrix4 Camera::buildMatrixOrtho2D(float left, float right, float bottom, float top, float zn, float zf)
{
	return buildMatrixOrtho2DGLLH(left, right, bottom, top, zn, zf);
}

Matrix4 Camera::buildMatrixOrtho2DGLLH(float left, float right, float bottom, float top, float zn, float zf)
{
	Matrix4 result;
	glm::mat4 glmMatrix = glm::ortho(left, right, bottom, top, zn, zf);
	result = Matrix4(&glmMatrix[0][0]);
	return result;
}

Matrix4 Camera::buildMatrixOrtho2DDXLH(float left, float right, float bottom, float top, float zn, float zf)
{
	Matrix4 result;
	glm::mat4 glmMatrix = glm::orthoLH(left, right, bottom, top, zn, zf);
	result = Matrix4(&glmMatrix[0][0]);
	return result;
}

Matrix4 Camera::buildMatrixLookAt(Vector3 eye, Vector3 target, Vector3 up)
{
	Matrix4 result;
	glm::mat4 glmMatrix = glm::lookAt(glm::vec3(eye.x, eye.y, eye.z), glm::vec3(target.x, target.y, target.z), glm::vec3(up.x, up.y, up.z));
	result = Matrix4(&glmMatrix[0][0]);
	return result;
}

Matrix4 Camera::buildMatrixLookAtLH(Vector3 eye, Vector3 target, Vector3 up)
{
	Matrix4 result;
	glm::mat4 glmMatrix = glm::lookAtLH(glm::vec3(eye.x, eye.y, eye.z), glm::vec3(target.x, target.y, target.z), glm::vec3(up.x, up.y, up.z));
	result = Matrix4(&glmMatrix[0][0]);
	return result;
}

Matrix4 Camera::buildMatrixPerspectiveFov(float fovRad, float aspect, float zn, float zf)
{
	return buildMatrixPerspectiveFovVertical(fovRad, aspect, zn, zf);
}

Matrix4 Camera::buildMatrixPerspectiveFovVertical(float fovRad, float aspectRatioWidthToHeight, float zn, float zf)
{
	Matrix4 result;
	glm::mat4 glmMatrix = glm::perspective(fovRad, aspectRatioWidthToHeight, zn, zf);
	result = Matrix4(&glmMatrix[0][0]);
	return result;
}

Matrix4 Camera::buildMatrixPerspectiveFovVerticalDXLH(float fovRad, float aspectRatioWidthToHeight, float zn, float zf)
{
	Matrix4 result;
	glm::mat4 glmMatrix = glm::perspectiveLH(fovRad, aspectRatioWidthToHeight, zn, zf);
	result = Matrix4(&glmMatrix[0][0]);
	return result;
}

Matrix4 Camera::buildMatrixPerspectiveFovHorizontal(float fovRad, float aspectRatioHeightToWidth, float zn, float zf)
{
	float verticalFov = 2.0f * std::atan(std::tan(fovRad * 0.5f) * aspectRatioHeightToWidth);
	return buildMatrixPerspectiveFovVertical(verticalFov, 1.0f / aspectRatioHeightToWidth, zn, zf);
}

Matrix4 Camera::buildMatrixPerspectiveFovHorizontalDXLH(float fovRad, float aspectRatioHeightToWidth, float zn, float zf)
{
	float verticalFov = 2.0f * std::atan(std::tan(fovRad * 0.5f) * aspectRatioHeightToWidth);
	return buildMatrixPerspectiveFovVerticalDXLH(verticalFov, 1.0f / aspectRatioHeightToWidth, zn, zf);
}

static Vector3 Vector3TransformCoord(const Vector3 &in, const Matrix4 &mat)
{
	glm::vec4 homogeneous(in.x, in.y, in.z, 1.0f);
	const glm::mat4 &glmMat = mat.getGLM();
	glm::vec4 result = glmMat * homogeneous;

	// perspective divide
	if (result.w != 0.0f)
	{
		result.x /= result.w;
		result.y /= result.w;
		result.z /= result.w;
	}

	return {result.x, result.y, result.z};
}

//*************************//
//	Camera implementation  //
//*************************//

Camera::Camera(Vector3 pos, Vector3 viewDir, float fovDeg, CAMERA_TYPE camType)
{
	m_vPos = pos;
	m_vViewDir = viewDir;
	m_fFov = deg2rad(fovDeg);
	m_camType = camType;

	m_fOrbitDistance = 5.0f;
	m_bOrbitYAxis = true;

	m_fPitch = 0;
	m_fYaw = 0;
	m_fRoll = 0;

	// base axes
	m_vWorldXAxis = Vector3(1, 0, 0);
	m_vWorldYAxis = Vector3(0, 1, 0);
	m_vWorldZAxis = Vector3(0, 0, 1);

	// derived axes
	m_vXAxis = m_vWorldXAxis;
	m_vYAxis = m_vWorldYAxis;
	m_vZAxis = m_vWorldZAxis;

	m_vViewRight = m_vWorldXAxis;
	m_vViewUp = m_vWorldYAxis;

	lookAt(pos + viewDir);
}

void Camera::updateVectors()
{
	// update rotation
	if (m_camType == CAMERA_TYPE_FIRST_PERSON)
		m_rotation.fromEuler(m_fRoll, m_fYaw, -m_fPitch);
	else if (m_camType == CAMERA_TYPE_ORBIT)
	{
		m_rotation.identity();

		if (m_bOrbitYAxis)
		{
			// yaw
			Quaternion tempQuat;
			tempQuat.fromAxis(m_vYAxis, m_fYaw);

			m_rotation = tempQuat * m_rotation;

			// pitch
			tempQuat.fromAxis(m_vXAxis, -m_fPitch);
			m_rotation = m_rotation * tempQuat;

			m_rotation.normalize();
		}
	}

	// calculate new coordinate system
	m_vViewDir = (m_worldRotation * m_rotation) * m_vZAxis;
	m_vViewRight = (m_worldRotation * m_rotation) * m_vXAxis;
	m_vViewUp = (m_worldRotation * m_rotation) * m_vYAxis;

	// update pos if we are orbiting (with the new coordinate system from above)
	if (m_camType == CAMERA_TYPE_ORBIT)
		setPos(m_vOrbitTarget);

	updateViewFrustum();
}

// using view matrix from camera position
void Camera::updateViewFrustum()
{
	Matrix4 viewMatrix = buildMatrixLookAt(m_vPos, m_vPos + m_vViewDir, m_vViewUp);
	Matrix4 projectionMatrix = buildMatrixPerspectiveFov(m_fFov, (float)engine->getScreenWidth() / (float)engine->getScreenHeight(), 0.1f, 100.0f);
	Matrix4 viewProj = projectionMatrix * viewMatrix;

	// extract frustum planes from view-projection matrix
	// left plane
	m_viewFrustum[0].a = viewProj[3] + viewProj[0];
	m_viewFrustum[0].b = viewProj[7] + viewProj[4];
	m_viewFrustum[0].c = viewProj[11] + viewProj[8];
	m_viewFrustum[0].d = viewProj[15] + viewProj[12];

	// right plane
	m_viewFrustum[1].a = viewProj[3] - viewProj[0];
	m_viewFrustum[1].b = viewProj[7] - viewProj[4];
	m_viewFrustum[1].c = viewProj[11] - viewProj[8];
	m_viewFrustum[1].d = viewProj[15] - viewProj[12];

	// top plane
	m_viewFrustum[2].a = viewProj[3] - viewProj[1];
	m_viewFrustum[2].b = viewProj[7] - viewProj[5];
	m_viewFrustum[2].c = viewProj[11] - viewProj[9];
	m_viewFrustum[2].d = viewProj[15] - viewProj[13];

	// bottom plane
	m_viewFrustum[3].a = viewProj[3] + viewProj[1];
	m_viewFrustum[3].b = viewProj[7] + viewProj[5];
	m_viewFrustum[3].c = viewProj[11] + viewProj[9];
	m_viewFrustum[3].d = viewProj[15] + viewProj[13];

	// normalize planes
	for (auto &i : m_viewFrustum)
	{
		glm::vec3 normal(i.a, i.b, i.c);
		float length = glm::length(normal);

		if (length > 0.0f)
		{
			normal = glm::normalize(normal);
			i.a = normal.x;
			i.b = normal.y;
			i.c = normal.z;
			i.d = i.d / length;
		}
		else
		{
			i.a = 0.0f;
			i.b = 0.0f;
			i.c = 0.0f;
			i.d = 0.0f;
		}
	}
}

void Camera::rotateX(float pitchDeg)
{
	m_fPitch += pitchDeg;

	if (m_fPitch > cl_pitchup.getFloat())
		m_fPitch = cl_pitchup.getFloat();
	else if (m_fPitch < -cl_pitchdown.getFloat())
		m_fPitch = -cl_pitchdown.getFloat();

	updateVectors();
}

void Camera::rotateY(float yawDeg)
{
	m_fYaw += yawDeg;

	if (m_fYaw > 360.0f)
		m_fYaw = m_fYaw - 360.0f;
	else if (m_fYaw < 0.0f)
		m_fYaw = 360.0f + m_fYaw;

	updateVectors();
}

void Camera::lookAt(Vector3 target)
{
	lookAt(m_vPos, target);
}

void Camera::lookAt(Vector3 eye, Vector3 target)
{
	if ((eye - target).length() < 0.001f)
		return;

	m_vPos = eye;

	// https://stackoverflow.com/questions/1996957/conversion-euler-to-matrix-and-matrix-to-euler
	// https://gamedev.stackexchange.com/questions/50963/how-to-extract-euler-angles-from-transformation-matrix

	Matrix4 lookAtMatrix = buildMatrixLookAt(eye, target, m_vYAxis);

	// extract Euler angles from the matrix (NOTE: glm::extractEulerAngleYXZ works differently for some reason?)
	const float yaw = glm::atan2(-lookAtMatrix[8], lookAtMatrix[0]);
	const float pitch = glm::asin(-lookAtMatrix[6]);
	///float roll = glm::atan2(lookAtMatrix[4], lookAtMatrix[5]);

	m_fYaw = 180.0f + glm::degrees(yaw);
	m_fPitch = glm::degrees(pitch);

	updateVectors();
}

void Camera::setType(CAMERA_TYPE camType)
{
	if (camType == m_camType)
		return;

	m_camType = camType;

	if (m_camType == CAMERA_TYPE_ORBIT)
		setPos(m_vOrbitTarget);
	else
		m_vPos = m_vOrbitTarget;
}

void Camera::setPos(Vector3 pos)
{
	m_vOrbitTarget = pos;

	if (m_camType == CAMERA_TYPE_ORBIT)
		m_vPos = m_vOrbitTarget + m_vViewDir * -m_fOrbitDistance;
	else if (m_camType == CAMERA_TYPE_FIRST_PERSON)
		m_vPos = pos;
}

void Camera::setOrbitDistance(float orbitDistance)
{
	m_fOrbitDistance = orbitDistance;
	if (m_fOrbitDistance < 0)
		m_fOrbitDistance = 0;
}

void Camera::setRotation(float yawDeg, float pitchDeg, float rollDeg)
{
	m_fYaw = yawDeg;
	m_fPitch = pitchDeg;
	m_fRoll = rollDeg;
	updateVectors();
}

void Camera::setYaw(float yawDeg)
{
	m_fYaw = yawDeg;
	updateVectors();
}

void Camera::setPitch(float pitchDeg)
{
	m_fPitch = pitchDeg;
	updateVectors();
}

void Camera::setRoll(float rollDeg)
{
	m_fRoll = rollDeg;
	updateVectors();
}

Vector3 Camera::getNextPosition(Vector3 velocity) const
{
	return m_vPos + ((m_worldRotation * m_rotation) * velocity);
}

Vector3 Camera::getProjectedVector(Vector3 point, float screenWidth, float screenHeight, float zn, float zf) const
{
	Matrix4 viewMatrix = buildMatrixLookAt(m_vPos, m_vPos + m_vViewDir, m_vViewUp);
	Matrix4 projectionMatrix = buildMatrixPerspectiveFov(m_fFov, screenWidth / screenHeight, zn, zf);

	// complete matrix
	Matrix4 viewProj = projectionMatrix * viewMatrix;

	// transform 3d point to 2d
	Vector3 result = Vector3TransformCoord(point, viewProj);

	// convert projected screen coordinates to real screen coordinates
	result.x = screenWidth * ((result.x + 1.0f) / 2.0f);
	result.y = screenHeight * ((1.0f - result.y) / 2.0f); // flip Y for screen coordinates
	result.z = zn + result.z * (zf - zn);

	return result;
}

Vector3 Camera::getUnProjectedVector(Vector2 point, float screenWidth, float screenHeight, float zn, float zf) const
{
	Matrix4 projectionMatrix = buildMatrixPerspectiveFov(m_fFov, screenWidth / screenHeight, zn, zf);
	Matrix4 viewMatrix = buildMatrixLookAt(m_vPos, m_vPos + m_vViewDir, m_vViewUp);

	// transform pick position from screen space into 3d space
	glm::vec4 viewport(0, 0, screenWidth, screenHeight);
	glm::mat4 glmModel(1.0f); // identity model matrix
	const glm::mat4 &glmView = viewMatrix.getGLM();
	const glm::mat4 &glmProj = projectionMatrix.getGLM();

	// combine model and view matrices (required by glm::unProject)
	glm::mat4 modelView = glmView * glmModel;

	glm::vec3 unprojected = glm::unProject(glm::vec3(point.x, screenHeight - point.y, 0.0f), // flip Y for OpenGL
	                                       modelView, glmProj, viewport);

	return {unprojected.x, unprojected.y, unprojected.z};
}

bool Camera::isPointVisibleFrustum(Vector3 point) const
{
	const float epsilon = 0.01f;

	// left
	float d11 = planeDotCoord(m_viewFrustum[0], point);

	// right
	float d21 = planeDotCoord(m_viewFrustum[1], point);

	// top
	float d31 = planeDotCoord(m_viewFrustum[2], point);

	// bottom
	float d41 = planeDotCoord(m_viewFrustum[3], point);

	if ((d11 < epsilon) || (d21 < epsilon) || (d31 < epsilon) || (d41 < epsilon))
		return false;

	return true;
}

bool Camera::isPointVisiblePlane(Vector3 point) const
{
	constexpr float epsilon = 0.0f; // ?

	if (!(planeDotCoord(m_vViewDir, m_vPos, point) < epsilon))
		return true;

	return false;
}

float Camera::planeDotCoord(CAM_PLANE plane, Vector3 point)
{
	return ((plane.a * point.x) + (plane.b * point.y) + (plane.c * point.z) + plane.d);
}

float Camera::planeDotCoord(Vector3 planeNormal, Vector3 planePoint, Vector3 &pv)
{
	return planeNormal.dot(pv - planePoint);
}
