/*
* SchriftGX2 is a wrapper class for libFreeType which renders a compiled
* FreeType parsable font into a GX texture for Wii homebrew development.
* Copyright (C) 2008 Armin Tamzarian
* Modified by Dimok, 2015 for WiiU GX2
*
* This file is part of SchriftGX2.
*
* SchriftGX2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* SchriftGX2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with SchriftGX2.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "schrift.h"
#include "shaders/gx2_ext.h"
#include <cstring>
#include <cwchar>
#include <gx2/sampler.h>
#include <gx2/texture.h>
#include <malloc.h>
#include <map>
#include <mutex>
#include <string>
#pragma GCC diagnostic ignored "-Wvolatile"
#include <coreinit/memexpheap.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/*! \struct ftgxCharData_
*
* Font face character glyph relevant data structure.
*/
typedef struct ftgxCharData_ {
    int16_t renderOffsetX;  /**< Texture X axis bearing offset. */
    uint16_t glyphAdvanceX; /**< Character glyph X coordinate advance in pixels. */
    uint16_t glyphAdvanceY; /**< Character glyph Y coordinate advance in pixels. */
    uint32_t glyphIndex;    /**< Charachter glyph index in the font face. */

    int16_t renderOffsetY;   /**< Texture Y axis bearing offset. */
    int16_t renderOffsetMax; /**< Texture Y axis bearing maximum value. */
    int16_t renderOffsetMin; /**< Texture Y axis bearing minimum value. */

    GX2Texture *texture;
} ftgxCharData;

/*! \struct ftgxDataOffset_
*
* Offset structure which hold both a maximum and minimum value.
*/
typedef struct ftgxDataOffset_ {
    int16_t ascender;  /**< Maximum data offset. */
    int16_t descender; /**< Minimum data offset. */
    int16_t max;       /**< Maximum data offset. */
    int16_t min;       /**< Minimum data offset. */
} ftgxDataOffset;

typedef struct ftgxCharData_ ftgxCharData;
typedef struct ftgxDataOffset_ ftgxDataOffset;
#define _TEXT(t)                L##t /**< Unicode helper macro. */

#define FTGX_NULL               0x0000
#define FTGX_JUSTIFY_LEFT       0x0001
#define FTGX_JUSTIFY_CENTER     0x0002
#define FTGX_JUSTIFY_RIGHT      0x0004
#define FTGX_JUSTIFY_MASK       0x000f

#define FTGX_ALIGN_TOP          0x0010
#define FTGX_ALIGN_MIDDLE       0x0020
#define FTGX_ALIGN_BOTTOM       0x0040
#define FTGX_ALIGN_BASELINE     0x0080
#define FTGX_ALIGN_GLYPH_TOP    0x0100
#define FTGX_ALIGN_GLYPH_MIDDLE 0x0200
#define FTGX_ALIGN_GLYPH_BOTTOM 0x0400
#define FTGX_ALIGN_MASK         0x0ff0

#define FTGX_STYLE_UNDERLINE    0x1000
#define FTGX_STYLE_STRIKE       0x2000
#define FTGX_STYLE_MASK         0xf000

/**< Constant color value used only to sanitize Doxygen documentation. */
static const GX2ColorF32 ftgxWhite = (GX2ColorF32){
        1.0f, 1.0f, 1.0f, 1.0f};


/*! \class SchriftGX2
* \brief Wrapper class for the libFreeType library with GX rendering.
* \author Armin Tamzarian
* \version 0.2.4
*
* SchriftGX2 acts as a wrapper class for the libFreeType library. It supports precaching of transformed glyph data into
* a specified texture format. Rendering of the data to the EFB is accomplished through the application of high performance
* GX texture functions resulting in high throughput of string rendering.
*/
class SchriftGX2 {
private:
    int16_t ftPointSize;   /**< Current set size of the rendered font. */
    bool ftKerningEnabled; /**< Flag indicating the availability of font kerning data. */
    uint8_t vertexIndex;   /**< Vertex format descriptor index. */
    GX2Sampler ftSampler;

    SFT pFont = {};

    uint8_t *glyphHeap            = nullptr;
    MEMHeapHandle glyphHeapHandle = nullptr;

    typedef struct _ftGX2Data {
        ftgxDataOffset ftgxAlign;
        std::map<wchar_t, ftgxCharData> ftgxCharMap;
    } ftGX2Data;

    std::map<int16_t, ftGX2Data> fontData; /**< Map which holds the glyph data structures for the corresponding characters in one size. */

    int16_t getStyleOffsetWidth(uint16_t width, uint16_t format);

    int16_t getStyleOffsetHeight(int16_t format, uint16_t pixelSize);

    ftgxCharData *cacheGlyphData(wchar_t charCode, int16_t pixelSize);

    bool loadGlyphData(SFT_Image *bmp, ftgxCharData *charData, ftGX2Data *ftData);

    void copyTextureToFramebuffer(GX2Texture *tex, int16_t screenX, int16_t screenY, int16_t screenZ, const glm::vec4 &color, const float &textBlur, const float &colorBlurIntensity, const glm::vec4 &blurColor);

    std::mutex fontDataMutex;

public:
    SchriftGX2(const uint8_t *fontBuffer, uint32_t bufferSize);

    ~SchriftGX2();

    void unloadFont();

    uint16_t drawText(int16_t x, int16_t y, int16_t z, const wchar_t *text, int16_t pixelSize, const glm::vec4 &color,
                      uint16_t textStyling, uint16_t textWidth, const float &textBlur, const float &colorBlurIntensity, const glm::vec4 &blurColor);

    uint16_t getWidth(const wchar_t *text, int16_t pixelSize);

    uint16_t getCharWidth(const wchar_t wChar, int16_t pixelSize, const wchar_t prevChar = 0x0000);

    uint16_t getHeight(const wchar_t *text, int16_t pixelSize);
    void getOffset(const wchar_t *text, int16_t pixelSize, uint16_t widthLimit = 0);

    static wchar_t *charToWideChar(const char *p);

    static char *wideCharToUTF8(const wchar_t *strChar);
};
