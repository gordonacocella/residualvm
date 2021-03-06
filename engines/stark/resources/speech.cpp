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

#include "engines/stark/resources/speech.h"

#include "engines/stark/formats/xrc.h"

#include "engines/stark/services/services.h"
#include "engines/stark/services/global.h"

#include "engines/stark/resources/anim.h"
#include "engines/stark/resources/item.h"
#include "engines/stark/resources/level.h"
#include "engines/stark/resources/location.h"
#include "engines/stark/resources/sound.h"

namespace Stark {
namespace Resources {

Speech::~Speech() {
}

Speech::Speech(Object *parent, byte subType, uint16 index, const Common::String &name) :
		Object(parent, subType, index, name),
		_character(0),
		_soundResource(nullptr),
		_playTalkAnim(true),
		_removeTalkAnimWhenComplete(true) {
	_type = TYPE;
}

Common::String Speech::getPhrase() const {
	return _phrase;
}

void Speech::playSound() {
	if (_playTalkAnim) {
		setCharacterTalkAnim();
	}

	stopOtherSpeechesFromSameCharacter();

	_soundResource = findChild<Sound>();
	_soundResource->play();
}

void Speech::setCharacterTalkAnim() const {
	ItemVisual *characterItem = getCharacterItem();
	if (characterItem) {
		characterItem->setAnimKind(Anim::kActorUsageTalk);
	}
}

void Speech::removeCharacterTalkAnim() const {
	ItemVisual *characterItem = getCharacterItem();
	if (characterItem && characterItem->getAnimKind() == Anim::kActorUsageTalk) {
		characterItem->setAnimKind(Anim::kActorUsageIdle);
	}
}

ItemVisual *Speech::getCharacterItem() const {
	Current *current = StarkGlobal->getCurrent();
	if (!current) {
		return nullptr;
	}

	Location *location = current->getLocation();
	if (!location) {
		return nullptr;
	}

	return location->getCharacterItem(_character);
}

bool Speech::isPlaying() {
	return _soundResource && _soundResource->isPlaying();
}

void Speech::stop() {
	if (_soundResource) {
		_soundResource->stop();
		_soundResource = nullptr;
	}

	if (_removeTalkAnimWhenComplete) {
		removeCharacterTalkAnim();
	}

	_removeTalkAnimWhenComplete = true;
	_playTalkAnim = true;
}

bool Speech::characterIsApril() const {
	int32 aprilCharacterIndex = StarkGlobal->getApril()->getCharacterIndex();
	return _character == aprilCharacterIndex;
}

void Speech::readData(Formats::XRCReadStream *stream) {
	Object::readData(stream);

	_phrase = stream->readString();
	_character = stream->readSint32LE();
}

void Speech::onGameLoop() {
	Object::onGameLoop();

	// TODO: Add delay between the end of the sound resource and the end of the speech

	if (_soundResource && !_soundResource->isPlaying()) {
		// The speech just stopped, reset our state
		stop();
	}
}

void Speech::onExitLocation() {
	stop();
}

void Speech::onPreDestroy() {
	stop();
}

void Speech::printData() {
	Object::printData();

	debug("phrase: %s", _phrase.c_str());
	debug("character: %d", _character);
}

void Speech::setPlayTalkAnim(bool playTalkAnim) {
	_playTalkAnim = playTalkAnim;
}

void Speech::stopOtherSpeechesFromSameCharacter() {
	Level *globalLevel = StarkGlobal->getLevel();
	Level *currentLevel = StarkGlobal->getCurrent()->getLevel();
	Location *currentLocation = StarkGlobal->getCurrent()->getLocation();

	Common::Array<Speech *> globalLevelSpeeches = globalLevel->listChildrenRecursive<Speech>();
	Common::Array<Speech *> currentLevelSpeeches = currentLevel->listChildrenRecursive<Speech>();
	Common::Array<Speech *> currentLocationSpeeches = currentLocation->listChildrenRecursive<Speech>();

	Common::Array<Speech *> speeches;
	speeches.push_back(globalLevelSpeeches);
	speeches.push_back(currentLevelSpeeches);
	speeches.push_back(currentLocationSpeeches);

	for (uint i = 0; i < speeches.size(); i++) {
		Speech *speech = speeches[i];
		if (speech->_character == _character && speech->isPlaying()) {
			speech->stop();
		}
	}
}

} // End of namespace Resources
} // End of namespace Stark
