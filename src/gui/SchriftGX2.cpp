/*
* SchriftGX2 is a wrapper class for libFreeType which renders a compiled
* FreeType parsable font so a GX texture for Wii homebrew development.
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

#include "SchriftGX2.h"
#include "schrift.h"
#include "shaders/Texture2DShader.h"
#include "utils/logger.h"

using namespace std;

/**
* Default constructor for the SchriftGX2 class.
*/
SchriftGX2::SchriftGX2(const uint8_t *fontBuffer, uint32_t bufferSize) {
    GX2InitSampler(&ftSampler, GX2_TEX_CLAMP_MODE_CLAMP_BORDER, GX2_TEX_XY_FILTER_MODE_LINEAR);
    ftPointSize = 0;

    pFont.xScale = 20;
    pFont.yScale = 20,
    pFont.flags  = SFT_DOWNWARD_Y;
    pFont.font   = sft_loadmem(fontBuffer, bufferSize);
    if (!pFont.font) {
        DEBUG_FUNCTION_LINE_ERR("Failed to load font!!");
    } else {
        OSMemoryBarrier();
    }

    uint32_t heapSize = 1024 * 1024;
    glyphHeap         = (uint8_t *) MEMAllocFromMappedMemoryForGX2Ex(heapSize, 0x100);
    if (!glyphHeap) {
        heapSize  = 512 * 1024;
        glyphHeap = (uint8_t *) MEMAllocFromMappedMemoryForGX2Ex(heapSize, 0x100);
    }
    if (glyphHeap) {
        glyphHeapHandle = MEMCreateExpHeapEx((void *) (glyphHeap), heapSize, 1);
        if (!glyphHeapHandle) {
            OSFatal("NotificationModule: Failed to create heap for glyphData");
        }
    } else {
        OSFatal("NotificationModule: Failed to alloc heap for glyphData");
    }

    ftKerningEnabled = false;
}

/**
* Default destructor for the SchriftGX2 class.
*/
SchriftGX2::~SchriftGX2() {
    unloadFont();
    sft_freefont(pFont.font);
    pFont.font = nullptr;

    MEMFreeToMappedMemory(glyphHeap);
    glyphHeap = nullptr;
}

/**
* Convert a short char string to a wide char string.
*
* This routine converts a supplied short character string into a wide character string.
* Note that it is the user's responsibility to clear the returned buffer once it is no longer needed.
*
* @param strChar   Character string to be converted.
* @return Wide character representation of supplied character string.
*/

wchar_t *SchriftGX2::charToWideChar(const char *strChar) {
    if (!strChar) return nullptr;

    auto *strWChar = new (std::nothrow) wchar_t[strlen(strChar) + 1];
    if (!strWChar) return nullptr;

    int32_t bt = mbstowcs(strWChar, strChar, strlen(strChar));
    if (bt > 0) {
        strWChar[bt] = 0;
        return strWChar;
    }

    wchar_t *tempDest = strWChar;
    while ((*tempDest++ = *strChar++))
        ;

    return strWChar;
}

char *SchriftGX2::wideCharToUTF8(const wchar_t *strChar) {
    if (!strChar) {
        return nullptr;
    }

    size_t len = 0;
    wchar_t wc;

    for (size_t i = 0; strChar[i]; ++i) {
        wc = strChar[i];
        if (wc < 0x80)
            ++len;
        else if (wc < 0x800)
            len += 2;
        else if (wc < 0x10000)
            len += 3;
        else
            len += 4;
    }

    char *pOut = new (std::nothrow) char[len];
    if (!pOut)
        return nullptr;

    size_t n = 0;

    for (size_t i = 0; strChar[i]; ++i) {
        wc = strChar[i];
        if (wc < 0x80)
            pOut[n++] = (char) wc;
        else if (wc < 0x800) {
            pOut[n++] = (char) ((wc >> 6) | 0xC0);
            pOut[n++] = (char) ((wc & 0x3F) | 0x80);
        } else if (wc < 0x10000) {
            pOut[n++] = (char) ((wc >> 12) | 0xE0);
            pOut[n++] = (char) (((wc >> 6) & 0x3F) | 0x80);
            pOut[n++] = (char) ((wc & 0x3F) | 0x80);
        } else {
            pOut[n++] = (char) (((wc >> 18) & 0x07) | 0xF0);
            pOut[n++] = (char) (((wc >> 12) & 0x3F) | 0x80);
            pOut[n++] = (char) (((wc >> 6) & 0x3F) | 0x80);
            pOut[n++] = (char) ((wc & 0x3F) | 0x80);
        }
    }
    return pOut;
}

/**
* Clears all loaded font glyph data.
*
* This routine clears all members of the font map structure and frees all allocated memory back to the system.
*/
void SchriftGX2::unloadFont() {
    std::lock_guard<std::mutex> lock(fontDataMutex);
    for (auto &dataForSize : fontData) {
        for (auto &cur : dataForSize.second.ftgxCharMap) {
            if (cur.second.texture) {
                if (cur.second.texture->surface.image) {
                    MEMFreeToExpHeap(glyphHeapHandle, cur.second.texture->surface.image);
                }
                delete cur.second.texture;
                cur.second.texture = nullptr;
            }
        }
    }

    fontData.clear();
}

/**
* Caches the given font glyph in the instance font texture buffer.
*
* This routine renders and stores the requested glyph's bitmap and relevant information into its own quickly addressible
* structure within an instance-specific map.
*
* @param charCode  The requested glyph's character code.
* @return A pointer to the allocated font structure.
*/
ftgxCharData *SchriftGX2::cacheGlyphData(wchar_t charCode, int16_t pixelSize) {
    std::lock_guard<std::mutex> lock(fontDataMutex);
    auto itr = fontData.find(pixelSize);
    if (itr != fontData.end()) {
        auto itr2 = itr->second.ftgxCharMap.find(charCode);
        if (itr2 != itr->second.ftgxCharMap.end()) {
            return &itr2->second;
        }
    }
    //!Cache ascender and decender as well
    ftGX2Data *ftData = &fontData[pixelSize];

    uint16_t textureWidth = 0, textureHeight = 0;
    if (ftPointSize != pixelSize) {
        ftPointSize  = pixelSize;
        pFont.xScale = ftPointSize;
        pFont.yScale = ftPointSize;
        SFT_LMetrics metrics;
        sft_lmetrics(&pFont, &metrics);

        ftData->ftgxAlign.ascender  = (int16_t) metrics.ascender;
        ftData->ftgxAlign.descender = (int16_t) metrics.descender;
        ftData->ftgxAlign.max       = 0;
        ftData->ftgxAlign.min       = 0;
    }

    SFT_Glyph gid; //  unsigned long gid;
    if (sft_lookup(&pFont, charCode, &gid) >= 0) {
        SFT_GMetrics mtx;
        if (sft_gmetrics(&pFont, gid, &mtx) < 0) {
            DEBUG_FUNCTION_LINE_ERR("bad glyph metrics");
        }
        textureWidth  = (mtx.minWidth + 3) & ~3;
        textureHeight = mtx.minHeight;

        SFT_Image img = {
                .width  = textureWidth,
                .height = textureHeight,
        };

        if (textureWidth == 0) {
            textureWidth = 4;
        }
        if (textureHeight == 0) {
            textureHeight = 4;
        }

        img.pixels = malloc(img.width * img.height);
        if (!img.pixels) {
            return nullptr;
        }
        if (sft_render(&pFont, gid, img) < 0) {
            free(img.pixels);
            DEBUG_FUNCTION_LINE_ERR("sft_render failed.");
        } else {
            ftgxCharData charData;
            charData.renderOffsetX   = (int16_t) mtx.leftSideBearing;
            charData.renderOffsetY   = (int16_t) -mtx.yOffset;
            charData.glyphAdvanceX   = (uint16_t) mtx.advanceWidth;
            charData.glyphAdvanceY   = (uint16_t) 0;
            charData.glyphIndex      = (uint32_t) gid;
            charData.renderOffsetMax = (int16_t) (short) -mtx.yOffset;
            charData.renderOffsetMin = (int16_t) (short) (img.height - (-mtx.yOffset));

            //! Initialize texture
            charData.texture = new (std::nothrow) GX2Texture;
            if (!charData.texture) {
                free(img.pixels);
                return nullptr;
            }
            GX2InitTexture(charData.texture, textureWidth, textureHeight, 1, 0, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_SURFACE_DIM_TEXTURE_2D, GX2_TILE_MODE_LINEAR_ALIGNED);
            if (!loadGlyphData(&img, &charData, ftData)) {
                DEBUG_FUNCTION_LINE_ERR("Failed to load glyph %d", charCode);
                delete charData.texture;
                charData.texture = nullptr;
                free(img.pixels);
                return nullptr;
            } else {
                free(img.pixels);
                ftData->ftgxCharMap[charCode] = charData;
            }

            return &ftData->ftgxCharMap[charCode];
        }
    }
    return nullptr;
}

/**
* Loads the rendered bitmap into the relevant structure's data buffer.
*
* This routine does a simple byte-wise copy of the glyph's rendered 8-bit grayscale bitmap into the structure's buffer.
* Each byte is converted from the bitmap's intensity value into the a uint32_t RGBA value.
*
* @param bmp   A pointer to the most recently rendered glyph's bitmap.
* @param charData  A pointer to an allocated ftgxCharData structure whose data represent that of the last rendered glyph.
*/

bool SchriftGX2::loadGlyphData(SFT_Image *bmp, ftgxCharData *charData, ftGX2Data *ftData) {
    if (charData == nullptr || charData->texture->surface.imageSize == 0 || charData->texture->surface.alignment == 0 || ftData == nullptr) {
        DEBUG_FUNCTION_LINE_ERR("Input data was NULL");
        return false;
    }

    charData->texture->surface.image = (uint8_t *) MEMAllocFromExpHeapEx(glyphHeapHandle, charData->texture->surface.imageSize, charData->texture->surface.alignment);
    if (!charData->texture->surface.image) {
        DEBUG_FUNCTION_LINE_INFO("Cache is full, let's clear it");
        for (auto &cur : ftData->ftgxCharMap) {
            if (cur.second.texture) {
                if (cur.second.texture->surface.image) {
                    MEMFreeToExpHeap(glyphHeapHandle, cur.second.texture->surface.image);
                }
                delete cur.second.texture;
            }
        }
        ftData->ftgxCharMap.clear();
        charData->texture->surface.image = (uint8_t *) MEMAllocFromExpHeapEx(glyphHeapHandle, charData->texture->surface.imageSize, charData->texture->surface.alignment);
        if (!charData->texture->surface.image) {
            return false;
        }
    }

    memset(charData->texture->surface.image, 0x00, charData->texture->surface.imageSize);

    auto *src = (uint8_t *) bmp->pixels;
    auto *dst = (uint32_t *) charData->texture->surface.image;
    uint32_t x, y;

    for (y = 0; y < bmp->height; y++) {
        for (x = 0; x < bmp->width; x++) {
            uint8_t val                                   = src[y * bmp->width + x];
            dst[y * charData->texture->surface.pitch + x] = val << 24 | val << 16 | val << 8 | val;
        }
    }
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, charData->texture->surface.image, charData->texture->surface.imageSize);
    return true;
}

/**
* Determines the x offset of the rendered string.
*
* This routine calculates the x offset of the rendered string based off of a supplied positional format parameter.
*
* @param width Current pixel width of the string.
* @param format	Positional format of the string.
*/
int16_t SchriftGX2::getStyleOffsetWidth(uint16_t width, uint16_t format) {
    if (format & FTGX_JUSTIFY_LEFT)
        return 0;
    else if (format & FTGX_JUSTIFY_CENTER)
        return -(width >> 1);
    else if (format & FTGX_JUSTIFY_RIGHT)
        return -width;
    return 0;
}

/**
* Determines the y offset of the rendered string.
*
* This routine calculates the y offset of the rendered string based off of a supplied positional format parameter.
*
* @param offset	Current pixel offset data of the string.
* @param format	Positional format of the string.
*/
int16_t SchriftGX2::getStyleOffsetHeight(int16_t format, uint16_t pixelSize) {
    std::lock_guard<std::mutex> lock(fontDataMutex);
    std::map<int16_t, ftGX2Data>::iterator itr = fontData.find(pixelSize);
    if (itr == fontData.end()) return 0;

    switch (format & FTGX_ALIGN_MASK) {
        case FTGX_ALIGN_TOP:
            return itr->second.ftgxAlign.descender;

        case FTGX_ALIGN_MIDDLE:
        default:
            return (itr->second.ftgxAlign.ascender + itr->second.ftgxAlign.descender + 1) >> 1;

        case FTGX_ALIGN_BOTTOM:
            return itr->second.ftgxAlign.ascender;

        case FTGX_ALIGN_BASELINE:
            return 0;

        case FTGX_ALIGN_GLYPH_TOP:
            return itr->second.ftgxAlign.max;

        case FTGX_ALIGN_GLYPH_MIDDLE:
            return (itr->second.ftgxAlign.max + itr->second.ftgxAlign.min + 1) >> 1;

        case FTGX_ALIGN_GLYPH_BOTTOM:
            return itr->second.ftgxAlign.min;
    }
    return 0;
}

/**
* Processes the supplied text string and prints the results at the specified coordinates.
*
* This routine processes each character of the supplied text string, loads the relevant preprocessed bitmap buffer,
* a texture from said buffer, and loads the resultant texture into the EFB.
*
* @param x Screen X coordinate at which to output the text.
* @param y Screen Y coordinate at which to output the text. Note that this value corresponds to the text string origin and not the top or bottom of the glyphs.
* @param text  NULL terminated string to output.
* @param color Optional color to apply to the text characters. If not specified default value is ftgxWhite: (GXColor){0xff, 0xff, 0xff, 0xff}
* @param textStyle Flags which specify any styling which should be applied to the rendered string.
* @return The number of characters printed.
*/

uint16_t SchriftGX2::drawText(int16_t x, int16_t y, int16_t z, const wchar_t *text, int16_t pixelSize, const glm::vec4 &color, uint16_t textStyle, uint16_t textWidth, const float &textBlur, const float &colorBlurIntensity, const glm::vec4 &blurColor) {
    if (!text) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(fontDataMutex);

    // uint16_t fullTextWidth = (textWidth > 0) ? textWidth : getWidth(text, pixelSize);
    uint16_t x_pos = x, printed = 0;
    uint16_t x_offset = 0, y_offset = 0;

    if (textStyle & FTGX_JUSTIFY_MASK) {
        //x_offset = getStyleOffsetWidth(fullTextWidth, textStyle);
    }
    if (textStyle & FTGX_ALIGN_MASK) {
        //y_offset = getStyleOffsetHeight(textStyle, pixelSize);
    }

    int32_t i = 0;
    while (text[i]) {
        ftgxCharData *glyphData = cacheGlyphData(text[i], pixelSize);

        if (glyphData != nullptr) {
            if (ftKerningEnabled && i > 0) {
                SFT_Kerning kerning;
                sft_kerning(&pFont, fontData[pixelSize].ftgxCharMap[text[i - 1]].glyphIndex, glyphData->glyphIndex, &kerning);
                x_pos += (kerning.xShift);
            }
            copyTextureToFramebuffer(glyphData->texture, x_pos + glyphData->renderOffsetX + x_offset, y + glyphData->renderOffsetY - y_offset, z, color, textBlur, colorBlurIntensity, blurColor);

            x_pos += glyphData->glyphAdvanceX;
            ++printed;
        }
        ++i;
    }

    return printed;
}


/**
* Processes the supplied string and return the width of the string in pixels.
*
* This routine processes each character of the supplied text string and calculates the width of the entire string.
* Note that if precaching of the entire font set is not enabled any uncached glyph will be cached after the call to this function.
*
* @param text  NULL terminated string to calculate.
* @return The width of the text string in pixels.
*/
uint16_t SchriftGX2::getWidth(const wchar_t *text, int16_t pixelSize) {
    if (!text) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(fontDataMutex);

    uint16_t strWidth = 0;
    int32_t i         = 0;

    while (text[i]) {
        ftgxCharData *glyphData = cacheGlyphData(text[i], pixelSize);
        if (glyphData != nullptr) {
            if (ftKerningEnabled && (i > 0)) {
                SFT_Kerning kerning;
                sft_kerning(&pFont, fontData[pixelSize].ftgxCharMap[text[i - 1]].glyphIndex, glyphData->glyphIndex, &kerning);
                strWidth += kerning.xShift;
            }

            strWidth += glyphData->glyphAdvanceX;
        }
        ++i;
    }
    return strWidth;
}

/**
* Single char width
*/
uint16_t SchriftGX2::getCharWidth(const wchar_t wChar, int16_t pixelSize, const wchar_t prevChar) {
    std::lock_guard<std::mutex> lock(fontDataMutex);
    uint16_t strWidth       = 0;
    ftgxCharData *glyphData = cacheGlyphData(wChar, pixelSize);

    if (glyphData != nullptr) {
        if (ftKerningEnabled && prevChar != 0x0000) {
            SFT_Kerning kerning;
            sft_kerning(&pFont, fontData[pixelSize].ftgxCharMap[prevChar].glyphIndex, glyphData->glyphIndex, &kerning);
            strWidth += kerning.xShift;
        }
        strWidth += glyphData->glyphAdvanceX;
    }

    return strWidth;
}

/**
* Processes the supplied string and return the height of the string in pixels.
*
* This routine processes each character of the supplied text string and calculates the height of the entire string.
* Note that if precaching of the entire font set is not enabled any uncached glyph will be cached after the call to this function.
*
* @param text  NULL terminated string to calculate.
* @return The height of the text string in pixels.
*/
uint16_t SchriftGX2::getHeight(const wchar_t *text, int16_t pixelSize) {
    std::lock_guard<std::mutex> lock(fontDataMutex);
    getOffset(text, pixelSize);
    return fontData[pixelSize].ftgxAlign.max - fontData[pixelSize].ftgxAlign.min;
}

/**
* Get the maximum offset above and minimum offset below the font origin line.
*
* This function calculates the maximum pixel height above the font origin line and the minimum
* pixel height below the font origin line and returns the values in an addressible structure.
*
* @param text  NULL terminated string to calculate.
* @param offset returns the max and min values above and below the font origin line
*
*/
void SchriftGX2::getOffset(const wchar_t *text, int16_t pixelSize, uint16_t widthLimit) {
    if (!text) {
        return;
    }
    std::lock_guard<std::mutex> lock(fontDataMutex);
    int16_t strMax = 0, strMin = 9999;
    uint16_t currWidth = 0;

    int32_t i = 0;

    while (text[i]) {
        if (widthLimit > 0 && currWidth >= widthLimit) break;

        ftgxCharData *glyphData = cacheGlyphData(text[i], pixelSize);

        if (glyphData != nullptr) {
            strMax = glyphData->renderOffsetMax > strMax ? glyphData->renderOffsetMax : strMax;
            strMin = glyphData->renderOffsetMin < strMin ? glyphData->renderOffsetMin : strMin;
            currWidth += glyphData->glyphAdvanceX;
        }

        ++i;
    }

    if (ftPointSize != pixelSize) {
        ftPointSize  = pixelSize;
        pFont.xScale = ftPointSize;
        pFont.yScale = ftPointSize;
    }
    SFT_LMetrics metrics;
    sft_lmetrics(&pFont, &metrics);

    fontData[pixelSize].ftgxAlign.ascender  = metrics.ascender;
    fontData[pixelSize].ftgxAlign.descender = metrics.descender;
    fontData[pixelSize].ftgxAlign.max       = strMax;
    fontData[pixelSize].ftgxAlign.min       = strMin;
}

/**
* Copies the supplied texture quad to the EFB.
*
* This routine uses the in-built GX quad builder functions to define the texture bounds and location on the EFB target.
*
* @param texObj	A pointer to the glyph's initialized texture object.
* @param texWidth  The pixel width of the texture object.
* @param texHeight The pixel height of the texture object.
* @param screenX   The screen X coordinate at which to output the rendered texture.
* @param screenY   The screen Y coordinate at which to output the rendered texture.
* @param color Color to apply to the texture.
*/
void SchriftGX2::copyTextureToFramebuffer(GX2Texture *texture, int16_t x, int16_t y, int16_t z, const glm::vec4 &color, const float &defaultBlur, const float &blurIntensity, const glm::vec4 &blurColor) {
    static const float imageAngle = 0.0f;
    static const float blurScale  = (2.0f);

    float widthScaleFactor  = 1.0f / (float) 1280;
    float heightScaleFactor = 1.0f / (float) 720;

    float offsetLeft = 2.0f * ((float) x + 0.5f * (float) texture->surface.width) * widthScaleFactor;
    float offsetTop  = 2.0f * ((float) y - 0.5f * (float) texture->surface.height) * heightScaleFactor;

    float widthScale  = blurScale * (float) texture->surface.width * widthScaleFactor;
    float heightScale = blurScale * (float) texture->surface.height * heightScaleFactor;

    glm::vec3 positionOffsets(offsetLeft, offsetTop, (float) z);

    //! blur doubles  due to blur we have to scale the texture
    glm::vec3 scaleFactor(widthScale, heightScale, 1.0f);

    glm::vec3 blurDirection;
    blurDirection[2] = 1.0f;

    Texture2DShader::instance()->setShaders();
    Texture2DShader::instance()->setAttributeBuffer();
    Texture2DShader::instance()->setAngle(imageAngle);
    Texture2DShader::instance()->setOffset(positionOffsets);
    Texture2DShader::instance()->setScale(scaleFactor);
    Texture2DShader::instance()->setTextureAndSampler(texture, &ftSampler);

    if (blurIntensity > 0.0f) {
        //! glow blur color
        Texture2DShader::instance()->setColorIntensity(blurColor);

        //! glow blur horizontal
        blurDirection[0] = blurIntensity;
        blurDirection[1] = 0.0f;
        Texture2DShader::instance()->setBlurring(blurDirection);
        Texture2DShader::instance()->draw();

        //! glow blur vertical
        blurDirection[0] = 0.0f;
        blurDirection[1] = blurIntensity;
        Texture2DShader::instance()->setBlurring(blurDirection);
        Texture2DShader::instance()->draw();
    }

    //! set text color
    Texture2DShader::instance()->setColorIntensity(color);

    //! blur horizontal
    blurDirection[0] = defaultBlur;
    blurDirection[1] = 0.0f;
    Texture2DShader::instance()->setBlurring(blurDirection);
    Texture2DShader::instance()->draw();

    //! blur vertical
    blurDirection[0] = 0.0f;
    blurDirection[1] = defaultBlur;
    Texture2DShader::instance()->setBlurring(blurDirection);
    Texture2DShader::instance()->draw();
}
