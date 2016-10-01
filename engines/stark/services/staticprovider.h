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

#ifndef STARK_SERVICES_STATIC_PROVIDER_H
#define STARK_SERVICES_STATIC_PROVIDER_H

#include "common/array.h"
#include "common/scummsys.h"

namespace Stark {

namespace Resources {
class Anim;
class Image;
class Level;
}

class ArchiveLoader;
class Global;
class VisualImageXMG;

/**
 * Static Resource provider.
 *
 * Maintains the static resource trees.
 * Maintained trees are the level and the location ones.
 */
class StaticProvider {
public:
	StaticProvider(ArchiveLoader *archiveLoader);

	enum UIElement {
		kInventoryScrollUpArrow = 1,
		kInventoryScrollDownArrow = 2,
		kImages = 4,
		kActionMenuBg = 5,
		kTextScrollUpArrow = 6,
		kTextScrollDownArrow = 7,
		kQuit = 8,
		kDiaryNormal = 15,
		kInventory = 16,
		kTextBackgroundActive = 20,
		kTextBackgroundPassive = 21
	};

	enum UIImage {
		kInventoryBg = 0,
		kDialogOptionBullet = 4
	};

	/** Load the static level archive */
	void init();

	/** State update loop */
	void onGameLoop();

	/** Release the static resources */
	void shutdown();

	/** Obtain the static level */
	Resources::Level *getLevel() const { return _level; }

	/** Get an image for a static cursor */
	VisualImageXMG *getCursorImage(uint32 cursor);

	/** Get an image for a static UI element */
	VisualImageXMG *getUIElement(UIElement element);

	/** Get an image for a static UI element */
	VisualImageXMG *getUIImage(UIImage image);

private:
	ArchiveLoader *_archiveLoader;

	Resources::Level *_level;
	Common::Array<Resources::Anim *> _stockAnims;
	Common::Array<Resources::Image *> _stockImages;
};

} // End of namespace Stark

#endif // STARK_SERVICES_STATIC_PROVIDER_H
