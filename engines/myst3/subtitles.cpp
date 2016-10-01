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

#include "engines/myst3/subtitles.h"
#include "engines/myst3/myst3.h"
#include "engines/myst3/state.h"

#ifdef USE_ICONV
#include "common/iconv.h"
#endif

#include "graphics/fontman.h"
#include "graphics/font.h"
#include "graphics/fonts/ttf.h"

#include "video/bink_decoder.h"

namespace Myst3 {

class FontSubtitles : public Subtitles {
public:
	FontSubtitles(Myst3Engine *vm);
	virtual ~FontSubtitles();

protected:
	void loadResources() override;
	bool loadSubtitles(int32 id) override;
	void drawToTexture(const Phrase *phrase) override;

private:
	void loadCharset(int32 id);
	void createTexture();

	/** Return a codepage usable by iconv from a GDI Charset as provided to CreateFont */
	const char *getCodePage(uint32 gdiCharset);

	const Graphics::Font *_font;
	Graphics::Surface *_surface;
	float _scale;
	uint8 *_charset;
};

FontSubtitles::FontSubtitles(Myst3Engine *vm) :
	Subtitles(vm),
	_font(0),
	_surface(0),
	_scale(1.0),
	_charset(nullptr) {
}

FontSubtitles::~FontSubtitles() {
	if (_surface) {
		_surface->free();
		delete _surface;
	}

	delete _font;
	delete[] _charset;
}

void FontSubtitles::loadResources() {
#ifdef USE_FREETYPE2
	Common::String ttfFile;
	if (_fontFace == "Arial Narrow") {
		// Use the TTF font provided by the game if TTF support is available
		ttfFile = "arir67w.ttf";
	} else if (_fontFace == "MS Gothic") {
		// The Japanese font has to be supplied by the user
		ttfFile = "msgothic.ttf";
	} else {
		error("Unknown subtitles font face '%s'", _fontFace.c_str());
	}

	Common::SeekableReadStream *s = SearchMan.createReadStreamForMember(ttfFile);
	if (s) {
		_font = Graphics::loadTTFFont(*s, _fontSize * _scale);
		delete s;
	} else {
		warning("Unable to load the subtitles font '%s'", ttfFile.c_str());
	}
#endif

	// We draw the subtitles in the adequate resolution so that they are not
	// scaled up. This is the scale factor of the current resolution
	// compared to the original
	Common::Rect screen = _vm->_gfx->viewport();
	_scale = screen.width() / Renderer::kOriginalWidth;

}

void FontSubtitles::loadCharset(int32 id) {
	const DirectorySubEntry *fontCharset = _vm->getFileDescription("CHAR", id, 0, DirectorySubEntry::kRawData);

	// Load the font charset if any
	if (fontCharset) {
		Common::MemoryReadStream *data = fontCharset->getData();

		_charset = new uint8[data->size()];

		data->read(_charset, data->size());

		delete data;
	}
}

bool FontSubtitles::loadSubtitles(int32 id) {
	// No game-provided charset for the Japanese version
	if (_fontCharsetCode == 0) {
		loadCharset(1100);
	}

	int32 overridenId = checkOverridenId(id);

	const DirectorySubEntry *desc = loadText(overridenId, overridenId != id);

	if (!desc)
		return false;

	Common::MemoryReadStream *crypted = desc->getData();

	// Read the frames and associated text offsets
	while (true) {
		Phrase s;
		s.frame = crypted->readUint32LE();
		s.offset = crypted->readUint32LE();

		if (!s.frame)
			break;

		_phrases.push_back(s);
	}

	// Read and decrypt the frames subtitles
	for (uint i = 0; i < _phrases.size(); i++) {
		crypted->seek(_phrases[i].offset);

		uint8 key = 35;
		while (true) {
			uint8 c = crypted->readByte() ^ key++;

			if (c >= 32 && _charset)
				c = _charset[c - 32];

			if (!c)
				break;

			_phrases[i].string += c;
		}
	}

	delete crypted;

	return true;
}

void FontSubtitles::createTexture() {
	// Create a surface to draw the subtitles on
	// Use RGB 565 to allow use of BDF fonts
	if (!_surface) {
		_surface = new Graphics::Surface();
		_surface->create(Renderer::kOriginalWidth * _scale, _surfaceHeight * _scale, Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0));
	}

	if (!_texture) {
		_texture = _vm->_gfx->createTexture(_surface);
	}
}

const char *FontSubtitles::getCodePage(uint32 gdiCharset) {
	static const struct {
		uint32 charset;
		const char *codepage;
	} codepages[] = {
			{ 128, "cp932"  }, // SHIFTJIS_CHARSET
			{ 129, "cp949"  }, // HANGUL_CHARSET
			{ 130, "cp1361" }, // JOHAB_CHARSET
			{ 134, "cp936"  }, // GB2312_CHARSET
			{ 136, "cp950"  }, // CHINESEBIG5_CHARSET
			{ 161, "cp1253" }, // GREEK_CHARSET
			{ 162, "cp1254" }, // TURKISH_CHARSET
			{ 163, "cp1258" }, // VIETNAMESE_CHARSET
			{ 177, "cp1255" }, // HEBREW_CHARSET
			{ 178, "cp1256" }, // ARABIC_CHARSET
			{ 186, "cp1257" }, // BALTIC_CHARSET
			{ 204, "cp1251" }, // RUSSIAN_CHARSET
			{ 222, "cp874"  }, // THAI_CHARSET
			{ 238, "cp1250" }  // EASTEUROPE_CHARSET
	};

	for (uint i = 0; i < ARRAYSIZE(codepages); i++) {
		if (gdiCharset == codepages[i].charset) {
			return codepages[i].codepage;
		}
	}

	error("Unknown font charset code '%d'", gdiCharset);
}

void FontSubtitles::drawToTexture(const Phrase *phrase) {
	const Graphics::Font *font;
	if (_font)
		font = _font;
	else
		font = FontMan.getFontByUsage(Graphics::FontManager::kLocalizedFont);

	if (!font)
		error("No available font");

	if (!_texture || !_surface) {
		createTexture();
	}

	// Draw the new text
	memset(_surface->getPixels(), 0, _surface->pitch * _surface->h);


	if (_fontCharsetCode == 0) {
		font->drawString(_surface, phrase->string, 0, _singleLineTop * _scale, _surface->w, 0xFFFFFFFF, Graphics::kTextAlignCenter);
	} else {
		const char *codepage = getCodePage(_fontCharsetCode);
#ifdef USE_ICONV
		Common::U32String unicode = Common::convertToU32String(codepage, phrase->string);
		font->drawString(_surface, unicode, 0, _singleLineTop * _scale, _surface->w, 0xFFFFFFFF, Graphics::kTextAlignCenter);
#else
		warning("Unable to display codepage '%s' subtitles, iconv support is not compiled in.", codepage);
#endif
	}

	// Update the texture
	_texture->update(_surface);
}

class MovieSubtitles : public Subtitles {
public:
	MovieSubtitles(Myst3Engine *vm);
	virtual ~MovieSubtitles();

protected:
	void loadResources() override;
	bool loadSubtitles(int32 id) override;
	void drawToTexture(const Phrase *phrase) override;

private:
	const DirectorySubEntry *loadMovie(int32 id, bool overriden);
	void readPhrases(const DirectorySubEntry *desc);

	Video::BinkDecoder _bink;
};

MovieSubtitles::MovieSubtitles(Myst3Engine *vm) :
		Subtitles(vm) {
}

MovieSubtitles::~MovieSubtitles() {
}

void MovieSubtitles::readPhrases(const DirectorySubEntry *desc) {
	Common::MemoryReadStream *frames = desc->getData();

	// Read the frames
	uint index = 0;
	while (true) {
		Phrase s;
		s.frame = frames->readUint32LE();
		s.offset = index;

		if (!s.frame)
			break;

		_phrases.push_back(s);
		index++;
	}

	delete frames;
}

const DirectorySubEntry *MovieSubtitles::loadMovie(int32 id, bool overriden) {
	const DirectorySubEntry *desc;
	if (overriden) {
		desc = _vm->getFileDescription("IMGR", 200000 + id, 0, DirectorySubEntry::kMovie);
	} else {
		desc = _vm->getFileDescription("", 200000 + id, 0, DirectorySubEntry::kMovie);
	}
	return desc;
}

bool MovieSubtitles::loadSubtitles(int32 id) {
	int32 overridenId = checkOverridenId(id);

	const DirectorySubEntry *phrases = loadText(overridenId, overridenId != id);
	const DirectorySubEntry *movie = loadMovie(overridenId, overridenId != id);

	if (!phrases || !movie)
		return false;

	readPhrases(phrases);

	// Load the movie
	Common::MemoryReadStream *movieStream = movie->getData();
	_bink.setDefaultHighColorFormat(Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
	_bink.loadStream(movieStream);
	_bink.start();

	return true;
}

void MovieSubtitles::loadResources() {
}

void MovieSubtitles::drawToTexture(const Phrase *phrase) {
	_bink.seekToFrame(phrase->offset);
	const Graphics::Surface *surface = _bink.decodeNextFrame();

	if (!_texture) {
		_texture = _vm->_gfx->createTexture(surface);
	} else {
		_texture->update(surface);
	}
}

Subtitles::Subtitles(Myst3Engine *vm) :
		_vm(vm),
		_texture(0),
		_frame(-1) {
}

Subtitles::~Subtitles() {
	freeTexture();
}

void Subtitles::loadFontSettings(int32 id) {
	// Load font settings
	const DirectorySubEntry *fontNums = _vm->getFileDescription("NUMB", id, 0, DirectorySubEntry::kNumMetadata);

	if (!fontNums)
		error("Unable to load font settings values");

	_fontSize = fontNums->getMiscData(0);
	_fontBold = fontNums->getMiscData(1);
	_surfaceHeight = fontNums->getMiscData(2);
	_singleLineTop = fontNums->getMiscData(3);
	_line1Top = fontNums->getMiscData(4);
	_line2Top = fontNums->getMiscData(5);
	_surfaceTop = fontNums->getMiscData(6) + Renderer::kTopBorderHeight + Renderer::kFrameHeight;
	_fontCharsetCode = fontNums->getMiscData(7);

	if (_fontCharsetCode > 0) {
		_fontCharsetCode = 128; // The Japanese subtitles are encoded in CP 932 / Shift JIS
	}

	if (_fontCharsetCode < 0) {
		_fontCharsetCode = -_fontCharsetCode; // Negative values are GDI charset codes
	}

	const DirectorySubEntry *fontText = _vm->getFileDescription("TEXT", id, 0, DirectorySubEntry::kTextMetadata);

	if (!fontText)
		error("Unable to load font face");

	_fontFace = fontText->getTextData(0);
}

int32 Subtitles::checkOverridenId(int32 id) {
	// Subtitles may be overridden using a variable
	if (_vm->_state->getMovieOverrideSubtitles()) {
		id = _vm->_state->getMovieOverrideSubtitles();
		_vm->_state->setMovieOverrideSubtitles(0);
	}
	return id;
}

const DirectorySubEntry *Subtitles::loadText(int32 id, bool overriden) {
	const DirectorySubEntry *desc;
	if (overriden) {
		desc = _vm->getFileDescription("IMGR", 100000 + id, 0, DirectorySubEntry::kText);
	} else {
		desc = _vm->getFileDescription("", 100000 + id, 0, DirectorySubEntry::kText);
	}
	return desc;
}

void Subtitles::setFrame(int32 frame) {
	const Phrase *phrase = nullptr;

	for (uint i = 0; i < _phrases.size(); i++) {
		if (_phrases[i].frame > frame)
			break;

		phrase = &_phrases[i];
	}

	if (!phrase) {
		freeTexture();
		return;
	}

	if (phrase->frame == _frame) {
		return;
	}

	_frame = phrase->frame;

	drawToTexture(phrase);
}

void Subtitles::drawOverlay() {
	if (!_texture) return;

	Common::Rect textureRect = Common::Rect(_texture->width, _texture->height);
	Common::Rect bottomBorder = Common::Rect(Renderer::kOriginalWidth, _surfaceHeight);
	bottomBorder.translate(0, _surfaceTop);

	_vm->_gfx->drawTexturedRect2D(bottomBorder, textureRect, _texture);
}

Subtitles *Subtitles::create(Myst3Engine *vm, uint32 id) {
	Subtitles *s;

	if (vm->getPlatform() == Common::kPlatformXbox) {
		s = new MovieSubtitles(vm);
	} else {
		s = new FontSubtitles(vm);
	}

	s->loadFontSettings(1100);

	if (!s->loadSubtitles(id)) {
		delete s;
		return 0;
	}

	s->loadResources();

	return s;
}

void Subtitles::freeTexture() {
	if (_texture) {
		_vm->_gfx->freeTexture(_texture);
		_texture = nullptr;
	}
}

} // End of namespace Myst3
