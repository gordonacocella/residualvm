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

#ifndef GFX_H_
#define GFX_H_

#include "common/rect.h"
#include "common/system.h"

#include "math/frustum.h"
#include "math/matrix4.h"
#include "math/vector3d.h"

namespace Myst3 {

class Drawable {
public:
	virtual void draw() {}
	virtual void drawOverlay() {}
	virtual ~Drawable() {}
};

class Texture {
public:
	uint width;
	uint height;
	Graphics::PixelFormat format;

	virtual void update(const Graphics::Surface *surface) = 0;
	virtual void updatePartial(const Graphics::Surface *surface, const Common::Rect &rect) = 0;
protected:
	Texture() {}
	virtual ~Texture() {}
};

class Renderer {
public:
	Renderer(OSystem *system);
	virtual ~Renderer();

	virtual void init() = 0;
	virtual void clear() = 0;

	/**
	 *  Swap the buffers, making the drawn screen visible
	 */
	virtual void flipBuffer() { }

	virtual void initFont(const Graphics::Surface *surface);
	virtual void freeFont();

	virtual Texture *createTexture(const Graphics::Surface *surface) = 0;
	virtual void freeTexture(Texture *texture) = 0;

	virtual void drawRect2D(const Common::Rect &rect, uint32 color) = 0;
	virtual void drawTexturedRect2D(const Common::Rect &screenRect, const Common::Rect &textureRect, Texture *texture,
									float transparency = -1.0, bool additiveBlending = false) = 0;
	virtual void drawTexturedRect3D(const Math::Vector3d &topLeft, const Math::Vector3d &bottomLeft,
									const Math::Vector3d &topRight, const Math::Vector3d &bottomRight,
									Texture *texture) = 0;

	virtual void drawCube(Texture **textures) = 0;
	virtual void draw2DText(const Common::String &text, const Common::Point &position) = 0;

	virtual Graphics::Surface *getScreenshot() = 0;

	Common::Rect viewport() const;
	Common::Rect frameViewport() const;
	Common::Point frameCenter() const;

	Math::Matrix4 makeProjectionMatrix(float fov) const;

	virtual void setupCameraOrtho2D(bool noScaling) = 0;
	virtual void setupCameraPerspective(float pitch, float heading, float fov);

	void screenPosToDirection(const Common::Point screen, float &pitch, float &heading);

	bool isCubeFaceVisible(uint face);

	void flipVertical(Graphics::Surface *s);

	static const int kOriginalWidth = 640;
	static const int kOriginalHeight = 480;
	static const int kTopBorderHeight = 30;
	static const int kBottomBorderHeight = 90;
	static const int kFrameHeight = 360;

protected:
	OSystem *_system;
	Texture *_font;

	Common::Rect _screenViewport;

	Math::Matrix4 _projectionMatrix;
	Math::Matrix4 _modelViewMatrix;
	Math::Matrix4 _mvpMatrix;

	Math::Frustum _frustum;

	static const float cubeVertices[5 * 6 * 4];
	Math::AABB _cubeFacesAABB[6];

	Common::Rect getFontCharacterRect(uint8 character);
	void computeScreenViewport();
};

/**
 * A framerate limiter
 *
 * Ensures the framerate does not exceed the specified value
 * by delaying until all of the timeslot allocated to the frame
 * is consumed.
 * Allows to curb CPU usage and have a stable framerate.
 */
class FrameLimiter {
public:
	FrameLimiter(OSystem *system, const uint framerate);

	void startFrame();
	void delayBeforeSwap();
private:
	OSystem *_system;

	uint _speedLimitMs;
	uint _startFrameTime;
};

Renderer *CreateGfxOpenGL(OSystem *system);
Renderer *CreateGfxOpenGLShader(OSystem *system);
Renderer *CreateGfxTinyGL(OSystem *system);
Renderer *createRenderer(OSystem *system);

} // End of namespace Myst3

#endif // GFX_H_
