/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#if defined(WIN32)
#include <windows.h>
// winnt.h defines ARRAYSIZE, but we want our own one...
#undef ARRAYSIZE
#endif

#include "engines/myst3/gfx.h"

#include "common/config-manager.h"

#include "graphics/renderer.h"
#include "graphics/surface.h"

#ifdef USE_OPENGL
#include "graphics/opengl/context.h"
#endif

#include "math/glmath.h"
#include "math/vector2d.h"

namespace Myst3 {

const float Renderer::cubeVertices[] = {
	// S     T      X      Y      Z
	0.0f, 1.0f, -320.0f, -320.0f, -320.0f,
	1.0f, 1.0f,  320.0f, -320.0f, -320.0f,
	0.0f, 0.0f, -320.0f,  320.0f, -320.0f,
	1.0f, 0.0f,  320.0f,  320.0f, -320.0f,
	0.0f, 1.0f,  320.0f, -320.0f, -320.0f,
	1.0f, 1.0f, -320.0f, -320.0f, -320.0f,
	0.0f, 0.0f,  320.0f, -320.0f,  320.0f,
	1.0f, 0.0f, -320.0f, -320.0f,  320.0f,
	0.0f, 1.0f,  320.0f, -320.0f,  320.0f,
	1.0f, 1.0f, -320.0f, -320.0f,  320.0f,
	0.0f, 0.0f,  320.0f,  320.0f,  320.0f,
	1.0f, 0.0f, -320.0f,  320.0f,  320.0f,
	0.0f, 1.0f,  320.0f, -320.0f, -320.0f,
	1.0f, 1.0f,  320.0f, -320.0f,  320.0f,
	0.0f, 0.0f,  320.0f,  320.0f, -320.0f,
	1.0f, 0.0f,  320.0f,  320.0f,  320.0f,
	0.0f, 1.0f, -320.0f, -320.0f,  320.0f,
	1.0f, 1.0f, -320.0f, -320.0f, -320.0f,
	0.0f, 0.0f, -320.0f,  320.0f,  320.0f,
	1.0f, 0.0f, -320.0f,  320.0f, -320.0f,
	0.0f, 1.0f,  320.0f,  320.0f,  320.0f,
	1.0f, 1.0f, -320.0f,  320.0f,  320.0f,
	0.0f, 0.0f,  320.0f,  320.0f, -320.0f,
	1.0f, 0.0f, -320.0f,  320.0f, -320.0f
};

Renderer::Renderer(OSystem *system)
		: _system(system), _font(NULL) {

	// Compute the cube faces Axis Aligned Bounding Boxes
	for (uint i = 0; i < ARRAYSIZE(_cubeFacesAABB); i++) {
		for (uint j = 0; j < 4; j++) {
			_cubeFacesAABB[i].expand(Math::Vector3d(cubeVertices[5 * (4 * i + j) + 2], cubeVertices[5 * (4 * i + j) + 3], cubeVertices[5 * (4 * i + j) + 4]));
		}
	}
}

Renderer::~Renderer() {
}

void Renderer::initFont(const Graphics::Surface *surface) {
	_font = createTexture(surface);
}

void Renderer::freeFont() {
	if (_font) {
		freeTexture(_font);
		_font = nullptr;
	}
}

Common::Rect Renderer::getFontCharacterRect(uint8 character) {
	uint index = 0;

	if (character == ' ')
		index = 0;
	else if (character >= '0' && character <= '9')
		index = 1 + character - '0';
	else if (character >= 'A' && character <= 'Z')
		index = 1 + 10 + character - 'A';
	else if (character == '|')
		index = 1 + 10 + 26;
	else if (character == '/')
		index = 2 + 10 + 26;
	else if (character == ':')
		index = 3 + 10 + 26;

	return Common::Rect(16 * index, 0, 16 * (index + 1), 32);
}

Common::Rect Renderer::viewport() const {
	return _screenViewport;
}

Common::Rect Renderer::frameViewport() const {
	Common::Rect screen = viewport();

	Common::Rect frame = Common::Rect(screen.width(), screen.height() * kFrameHeight / kOriginalHeight);
	frame.translate(screen.left, screen.top + screen.height() * kBottomBorderHeight / kOriginalHeight);

	return frame;
}

void Renderer::computeScreenViewport() {
	int32 screenWidth = _system->getWidth();
	int32 screenHeight = _system->getHeight();

	if (_system->getFeatureState(OSystem::kFeatureAspectRatioCorrection)) {
		// Aspect ratio correction
		int32 viewportWidth = MIN<int32>(screenWidth, screenHeight * kOriginalWidth / kOriginalHeight);
		int32 viewportHeight = MIN<int32>(screenHeight, screenWidth * kOriginalHeight / kOriginalWidth);
		_screenViewport = Common::Rect(viewportWidth, viewportHeight);

		// Pillarboxing
		_screenViewport.translate((screenWidth - viewportWidth) / 2,
			(screenHeight - viewportHeight) / 2);
	} else {
		// Aspect ratio correction disabled, just stretch
		_screenViewport = Common::Rect(screenWidth, screenHeight);
	}
}

Common::Point Renderer::frameCenter() const {
	Common::Rect screen = viewport();
	Common::Rect frame = frameViewport();
	return Common::Point((frame.left + frame.right) / 2, screen.top + screen.bottom - (frame.top + frame.bottom) / 2);
}

Math::Matrix4 Renderer::makeProjectionMatrix(float fov) const {
	static const float nearClipPlane = 1.0;
	static const float farClipPlane = 10000.0;

	float aspectRatio = kOriginalWidth / (float) kFrameHeight;

	float xmaxValue = nearClipPlane * tan(fov * M_PI / 360.0);
	float ymaxValue = xmaxValue / aspectRatio;

	return Math::makeFrustumMatrix(-xmaxValue, xmaxValue, -ymaxValue, ymaxValue, nearClipPlane, farClipPlane);
}

void Renderer::setupCameraPerspective(float pitch, float heading, float fov) {
	_projectionMatrix = makeProjectionMatrix(fov);
	_modelViewMatrix = Math::Matrix4(180.0f - heading, pitch, 0.0f, Math::EO_YXZ);

	Math::Matrix4 proj = _projectionMatrix;
	Math::Matrix4 model = _modelViewMatrix;
	proj.transpose();
	model.transpose();

	_mvpMatrix = proj * model;

	_frustum.setup(_mvpMatrix);

	_mvpMatrix.transpose();
}

void Renderer::screenPosToDirection(const Common::Point screen, float &pitch, float &heading) {
	// Screen coords to 3D coords
	Math::Vector3d obj;
	Math::gluMathUnProject(Math::Vector3d(screen.x, _system->getHeight() - screen.y, 0.9f), _mvpMatrix, frameViewport(), obj);

	// 3D coords to polar coords
	obj.normalize();

	Math::Vector2d horizontalProjection = Math::Vector2d(obj.x(), obj.z());
	horizontalProjection.normalize();

	pitch = 90 - Math::Angle::arcCosine(obj.y()).getDegrees();
	heading = Math::Angle::arcCosine(horizontalProjection.getY()).getDegrees();

	if (horizontalProjection.getX() > 0.0)
		heading = 360 - heading;
}

bool Renderer::isCubeFaceVisible(uint face) {
	assert(face < 6);

	return _frustum.isInside(_cubeFacesAABB[face]);
}

void Renderer::flipVertical(Graphics::Surface *s) {
	for (int y = 0; y < s->h / 2; ++y) {
		// Flip the lines
		byte *line1P = (byte *)s->getBasePtr(0, y);
		byte *line2P = (byte *)s->getBasePtr(0, s->h - y - 1);

		for (int x = 0; x < s->pitch; ++x)
			SWAP(line1P[x], line2P[x]);
	}
}

Renderer *createRenderer(OSystem *system) {
	Common::String rendererConfig = ConfMan.get("renderer");
	Graphics::RendererType desiredRendererType = Graphics::parseRendererTypeCode(rendererConfig);
	Graphics::RendererType matchingRendererType = Graphics::getBestMatchingAvailableRendererType(desiredRendererType);

	bool fullscreen = ConfMan.getBool("fullscreen");
	bool isAccelerated = matchingRendererType != Graphics::kRendererTypeTinyGL;
	system->setupScreen(Renderer::kOriginalWidth, Renderer::kOriginalHeight, fullscreen, isAccelerated);

#if defined(USE_OPENGL)
	// Check the OpenGL context actually supports shaders
	if (matchingRendererType == Graphics::kRendererTypeOpenGLShaders && !OpenGLContext.shadersSupported) {
		matchingRendererType = Graphics::kRendererTypeOpenGL;
	}
#endif

	if (matchingRendererType != desiredRendererType && desiredRendererType != Graphics::kRendererTypeDefault) {
		// Display a warning if unable to use the desired renderer
		warning("Unable to create a '%s' renderer", rendererConfig.c_str());
	}

#if defined(USE_GLES2) || defined(USE_OPENGL_SHADERS)
	if (matchingRendererType == Graphics::kRendererTypeOpenGLShaders) {
		return CreateGfxOpenGLShader(system);
	}
#endif
#if defined(USE_OPENGL) && !defined(USE_GLES2)
	if (matchingRendererType == Graphics::kRendererTypeOpenGL) {
		return CreateGfxOpenGL(system);
	}
#endif
	if (matchingRendererType == Graphics::kRendererTypeTinyGL) {
		return CreateGfxTinyGL(system);
	}

	error("Unable to create a '%s' renderer", rendererConfig.c_str());
}

FrameLimiter::FrameLimiter(OSystem *system, const uint framerate) :
	_system(system),
	_speedLimitMs(1000 / framerate),
	_startFrameTime(0) {

}

void FrameLimiter::startFrame() {
	_startFrameTime = _system->getMillis();
}

void FrameLimiter::delayBeforeSwap() {
	uint endFrameTime = _system->getMillis();
	uint frameDuration = endFrameTime - _startFrameTime;

	if (frameDuration < _speedLimitMs) {
		_system->delayMillis(_speedLimitMs - frameDuration);
	}
}

} // End of namespace Myst3
