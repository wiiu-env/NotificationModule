/****************************************************************************
 * Copyright (C) 2015 Dimok
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#include "GuiText.h"
#include "SchriftGX2.h"
#include "utils/logger.h"
#include "utils/utils.h"

SchriftGX2 *GuiText::presentFont = nullptr;
int32_t GuiText::presetSize      = 20;
int32_t GuiText::presetAlignment = ALIGN_CENTERED;
GX2ColorF32 GuiText::presetColor = (GX2ColorF32){1.0f, 1.0f, 1.0f, 1.0f};

/**
* Constructor for the GuiText class.
*/

GuiText::GuiText() {
    text              = nullptr;
    size              = presetSize;
    currentSize       = size;
    color             = glm::vec4(presetColor.r, presetColor.g, presetColor.b, presetColor.a);
    colorCorrected    = {SRGBComponentToRGB((uint8_t) (color.r * 255.0f)) / 255.0f,
                      SRGBComponentToRGB((uint8_t) (color.g * 255.0f)) / 255.0f,
                      SRGBComponentToRGB((uint8_t) (color.b * 255.0f)) / 255.0f,
                      color.a};
    alpha             = presetColor.a;
    alignment         = presetAlignment;
    textWidth         = 0;
    textHeight        = 0;
    font              = presentFont;
    defaultBlur       = 4.0f;
    blurGlowIntensity = 0.0f;
    blurAlpha         = 0.0f;
    blurGlowColor     = glm::vec4(0.0f);
    width             = 0;
    height            = 0;
}

GuiText::GuiText(const char *t, int s, const glm::vec4 &c) {
    text              = nullptr;
    size              = s;
    currentSize       = size;
    color             = c;
    alpha             = c[3];
    alignment         = ALIGN_CENTER | ALIGN_MIDDLE;
    font              = presentFont;
    textWidth         = 0;
    textHeight        = 0;
    defaultBlur       = 4.0f;
    blurGlowIntensity = 0.0f;
    blurAlpha         = 0.0f;
    blurGlowColor     = glm::vec4(0.0f);

    if (t) {
        std::lock_guard<std::mutex> textLock(mTextLock);
        text = SchriftGX2::charToWideChar(t);
        if (!text) {
            return;
        }

        textHeightDirty = true;
        textWidthDirty  = true;
    }
}

/**
* Destructor for the GuiText class.
*/
GuiText::~GuiText() {
    std::lock_guard<std::mutex> textLock(mTextLock);
    delete[] text;

    text = nullptr;
}

void GuiText::setText(const char *t) {
    std::lock_guard<std::mutex> textLock(mTextLock);
    delete[] text;

    text = nullptr;

    if (t) {
        text = SchriftGX2::charToWideChar(t);
        if (!text) {
            return;
        }
        textHeightDirty = true;
        textWidthDirty  = true;
    }
}

void GuiText::setPresets(int sz, const glm::vec4 &c, int w, int a) {
    presetSize      = sz;
    presetColor     = (GX2ColorF32){(float) c.r / 255.0f, (float) c.g / 255.0f, (float) c.b / 255.0f, (float) c.a / 255.0f};
    presetMaxWidth  = w;
    presetAlignment = a;
}

void GuiText::setPresetFont(SchriftGX2 *f) {
    presentFont = f;
}

void GuiText::setFontSize(int s) {
    std::lock_guard<std::mutex> textLock(mTextLock);
    size = s;
}

void GuiText::setColor(const glm::vec4 &c) {
    color = c;
    alpha = c[3];

    colorCorrected = {SRGBComponentToRGB((uint8_t) (color.r * 255.0f)) / 255.0f,
                      SRGBComponentToRGB((uint8_t) (color.g * 255.0f)) / 255.0f,
                      SRGBComponentToRGB((uint8_t) (color.b * 255.0f)) / 255.0f,
                      color.a};
}

void GuiText::setBlurGlowColor(float blur, const glm::vec4 &c) {
    blurGlowColor     = c;
    blurGlowIntensity = blur;
    blurAlpha         = c[3];
}

/**
* Change font
*/
bool GuiText::setFont(SchriftGX2 *f) {
    std::lock_guard<std::mutex> textLock(mTextLock);
    if (!f) {
        return false;
    }

    font            = f;
    textHeightDirty = true;
    textWidthDirty  = true;
    return true;
}

/**
* Draw the text on screen
*/
void GuiText::draw(bool SRGBConversion) {
    if (!text || !font) {
        return;
    }

    if (!isVisible()) {
        return;
    }
    std::lock_guard<std::mutex> textLock(mTextLock);

    color[3]         = getAlpha();
    blurGlowColor[3] = blurAlpha * getAlpha();

    float x_pos = getCenterX();
    float y_pos = getCenterY();

    if (alignment & ALIGN_TOP) {
        y_pos -= getTextHeight();
    } else if (alignment & ALIGN_MIDDLE) {
        y_pos -= getTextHeight() / 2.0f;
    }
    if (alignment & ALIGN_RIGHT) {
        x_pos -= getTextWidth();
    } else if (alignment & ALIGN_CENTER) {
        x_pos -= getTextWidth() / 2.0f;
    }

    font->drawText(x_pos, y_pos, getDepth(), text, currentSize, SRGBConversion ? colorCorrected : color, alignment, getTextWidth(), defaultBlur, blurGlowIntensity, blurGlowColor);
}

void GuiText::updateTextSize() {
    int newSize = size * getScale();
    if (newSize != currentSize) {
        currentSize     = newSize;
        textWidthDirty  = true;
        textHeightDirty = true;
    }

    if (textWidthDirty && font) {
        textWidth      = font->getWidth(text, currentSize);
        textWidthDirty = false;
    }
    if (textHeightDirty && font) {
        textHeight      = font->getHeight(text, currentSize);
        textHeightDirty = false;
    }
}

void GuiText::process() {
    GuiElement::process();
}
