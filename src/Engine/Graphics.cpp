//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		top level graphics interface
//
// $NoKeywords: $graphics
//===============================================================================//

#include "Graphics.h"

#include "Engine.h"
#include "ConVar.h"
#include "Camera.h"
namespace cv {
ConVar r_3dscene_zn("r_3dscene_zn", 5.0f, FCVAR_CHEAT);
ConVar r_3dscene_zf("r_3dscene_zf", 5000.0f, FCVAR_CHEAT);

ConVar r_globaloffset_x("r_globaloffset_x", 0.0f, FCVAR_CHEAT);
ConVar r_globaloffset_y("r_globaloffset_y", 0.0f, FCVAR_CHEAT);
ConVar r_debug_disable_cliprect("r_debug_disable_cliprect", false, FCVAR_CHEAT);
ConVar r_debug_disable_3dscene("r_debug_disable_3dscene", false, FCVAR_CHEAT);
ConVar r_debug_flush_drawstring("r_debug_flush_drawstring", false, FCVAR_NONE);
ConVar r_debug_drawimage("r_debug_drawimage", false, FCVAR_CHEAT);
}

Graphics::Graphics()
{
	// init matrix stacks
	m_bTransformUpToDate = false;
	m_worldTransformStack.emplace();
	m_projectionTransformStack.emplace();

	// init 3d gui scene stack
	m_bIs3dScene = false;
	m_3dSceneStack.push(false);
}

void Graphics::pushTransform()
{
	m_worldTransformStack.emplace(m_worldTransformStack.top());
	m_projectionTransformStack.emplace(m_projectionTransformStack.top());
}

void Graphics::popTransform()
{
	if (m_worldTransformStack.size() < 2)
	{
		engine->showMessageErrorFatal("World Transform Stack Underflow", "Too many pop*()s!");
		engine->shutdown();
		return;
	}

	if (m_projectionTransformStack.size() < 2)
	{
		engine->showMessageErrorFatal("Projection Transform Stack Underflow", "Too many pop*()s!");
		engine->shutdown();
		return;
	}

	m_worldTransformStack.pop();
	m_projectionTransformStack.pop();
	m_bTransformUpToDate = false;
}

void Graphics::translate(float x, float y, float z)
{
	m_worldTransformStack.top().translate(x, y, z);
	m_bTransformUpToDate = false;
}

void Graphics::rotate(float deg, float x, float y, float z)
{
	m_worldTransformStack.top().rotate(deg, x, y, z);
	m_bTransformUpToDate = false;
}

void Graphics::scale(float x, float y, float z)
{
	m_worldTransformStack.top().scale(x, y, z);
	m_bTransformUpToDate = false;
}

void Graphics::translate3D(float x, float y, float z)
{
	Matrix4 translation;
	translation.translate(x, y, z);
	setWorldMatrixMul(translation);
}

void Graphics::rotate3D(float deg, float x, float y, float z)
{
	Matrix4 rotation;
	rotation.rotate(deg, x, y, z);
	setWorldMatrixMul(rotation);
}

void Graphics::setWorldMatrix(Matrix4 &worldMatrix)
{
	m_worldTransformStack.pop();
	m_worldTransformStack.push(worldMatrix);
	m_bTransformUpToDate = false;
}

void Graphics::setWorldMatrixMul(Matrix4 &worldMatrix)
{
	m_worldTransformStack.top() *= worldMatrix;
	m_bTransformUpToDate = false;
}

void Graphics::setProjectionMatrix(Matrix4 &projectionMatrix)
{
	m_projectionTransformStack.pop();
	m_projectionTransformStack.push(projectionMatrix);
	m_bTransformUpToDate = false;
}

Matrix4 Graphics::getWorldMatrix()
{
	return m_worldTransformStack.top();
}

Matrix4 Graphics::getProjectionMatrix()
{
	return m_projectionTransformStack.top();
}

void Graphics::push3DScene(McRect region)
{
	if (cv::r_debug_disable_3dscene.getBool()) return;

	// you can't yet stack 3d scenes!
	if (m_3dSceneStack.top())
	{
		m_3dSceneStack.push(false);
		return;
	}

	// reset & init
	m_v3dSceneOffset.x = m_v3dSceneOffset.y = m_v3dSceneOffset.z = 0;
	float m_fFov = 60.0f;

	// push true, set region
	m_bIs3dScene = true;
	m_3dSceneStack.push(true);
	m_3dSceneRegion = region;

	// backup transforms
	pushTransform();

	// calculate height to fit viewport angle
	float angle = (180.0f - m_fFov) / 2.0f;
	float b = (engine->getScreenHeight() / std::sin(glm::radians(m_fFov))) * std::sin(glm::radians(angle));
	float hc = std::sqrt(std::pow(b,2.0f) - std::pow((engine->getScreenHeight()/2.0f), 2.0f));

	// set projection matrix
	Matrix4 trans2 = Matrix4().translate(-1 + (region.getWidth()) / (float)engine->getScreenWidth() + (region.getX()*2) / (float)engine->getScreenWidth(), 1 - region.getHeight() / (float)engine->getScreenHeight() - (region.getY()*2) / (float)engine->getScreenHeight(), 0);
	Matrix4 projectionMatrix = trans2 * Camera::buildMatrixPerspectiveFov(glm::radians(m_fFov),((float) engine->getScreenWidth())/((float) engine->getScreenHeight()), cv::r_3dscene_zn.getFloat(), cv::r_3dscene_zf.getFloat());
	m_3dSceneProjectionMatrix = projectionMatrix;

	// set world matrix
	Matrix4 trans = Matrix4().translate(-(float)region.getWidth()/2 - region.getX(), -(float)region.getHeight()/2 - region.getY(), 0);
	m_3dSceneWorldMatrix = Camera::buildMatrixLookAt(Vector3(0, 0, -hc), Vector3(0, 0, 0), Vector3(0, -1, 0)) * trans;

	// force transform update
	updateTransform(true);
}

void Graphics::pop3DScene()
{
	if (!m_3dSceneStack.top())
		return;

	m_3dSceneStack.pop();

	// restore transforms
	popTransform();

	m_bIs3dScene = false;
}

void Graphics::translate3DScene(float x, float y, float z)
{
	if (!m_3dSceneStack.top()) return; // block if we're not in a 3d scene

	// translate directly
	m_3dSceneWorldMatrix.translate(x,y,z);

	// force transform update
	updateTransform(true);
}

void Graphics::rotate3DScene(float rotx, float roty, float rotz)
{
	if (!m_3dSceneStack.top()) return; // block if we're not in a 3d scene

	// first translate to the center of the 3d region, then rotate, then translate back
	Matrix4 rot;
	Vector3 centerVec = Vector3(m_3dSceneRegion.getX()+m_3dSceneRegion.getWidth()/2 + m_v3dSceneOffset.x, m_3dSceneRegion.getY()+m_3dSceneRegion.getHeight()/2 + m_v3dSceneOffset.y, m_v3dSceneOffset.z);
	rot.translate(-centerVec);

	// rotate
	if (rotx != 0)
		rot.rotateX(-rotx);
	if (roty != 0)
		rot.rotateY(-roty);
	if (rotz != 0)
		rot.rotateZ(-rotz);

	rot.translate(centerVec);

	// apply the rotation
	m_3dSceneWorldMatrix = m_3dSceneWorldMatrix * rot;

	// force transform update
	updateTransform(true);
}

void Graphics::offset3DScene(float x, float y, float z)
{
	m_v3dSceneOffset = Vector3(x,y,z);
}

void Graphics::forceUpdateTransform()
{
	updateTransform();
}

void Graphics::updateTransform(bool force)
{
	if (!m_bTransformUpToDate || force)
	{
		m_worldMatrix = m_worldTransformStack.top();
		m_projectionMatrix = m_projectionTransformStack.top();

		// HACKHACK: 3d gui scenes
		if (m_bIs3dScene)
		{
			m_worldMatrix = m_3dSceneWorldMatrix * m_worldTransformStack.top();
			m_projectionMatrix = m_3dSceneProjectionMatrix;
		}

		m_MP = m_projectionMatrix * m_worldMatrix;

		onTransformUpdate(m_projectionMatrix, m_worldMatrix);

		m_bTransformUpToDate = true;
	}
}

void Graphics::checkStackLeaks()
{
	if (m_worldTransformStack.size() > 1)
	{
		engine->showMessageErrorFatal("World Transform Stack Leak", "Make sure all push*() have a pop*()!");
		engine->shutdown();
	}

	if (m_projectionTransformStack.size() > 1)
	{
		engine->showMessageErrorFatal("Projection Transform Stack Leak", "Make sure all push*() have a pop*()!");
		engine->shutdown();
	}

	if (m_3dSceneStack.size() > 1)
	{
		engine->showMessageErrorFatal("3DScene Stack Leak", "Make sure all push*() have a pop*()!");
		engine->shutdown();
	}
}



//************************//
//	Graphics ConCommands  //
//************************//

namespace cv
{
ConVar mat_wireframe("mat_wireframe", false, FCVAR_CHEAT, [](float on) -> void { g ? g->setWireframe(!!static_cast<int>(on)) : (void)0; });

ConVar vsync("vsync", false, FCVAR_NONE, "Usage: 'vsync 1' to turn vsync on, 'vsync 0' to turn vsync off",
             [](float on) -> void { g ? g->setVSync(!!static_cast<int>(on)) : (void)0; });
} // namespace cv
