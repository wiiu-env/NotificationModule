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
#pragma once

#include "GuiElement.h"
#include <mutex>
//!Forward declaration
class SchriftGX2;

//!Display, manage, and manipulate text in the GUI
class GuiText : public GuiElement {
public:
    //!Constructor
    GuiText();
    //!\param t Text
    //!\param s Font size
    //!\param c Font color
    GuiText(const char *t, int32_t s, const glm::vec4 &c);
    //!Destructor
    ~GuiText() override;
    //!Sets the text of the GuiText element
    //!\param t Text
    virtual void setText(const char *t);
    //!Sets up preset values to be used by GuiText(t)
    //!Useful when print32_ting multiple text elements, all with the same attributes set
    //!\param sz Font size
    //!\param c Font color
    //!\param w Maximum width of texture background (for text wrapping)
    //!\param wrap Wrapmode when w>0
    //!\param a Text alignment
    static void setPresets(int32_t sz, const glm::vec4 &c, int32_t w, int32_t a);
    static void setPresetFont(SchriftGX2 *font);
    //!Sets the font size
    //!\param s Font size
    void setFontSize(int32_t s);

    //!Sets the font color
    //!\param c Font color
    void setColor(const glm::vec4 &c);

    void setBlurGlowColor(float blurint32_tensity, const glm::vec4 &c);

    void setTextBlur(float blur) { defaultBlur = blur; }
    //!Get the original text as char
    [[nodiscard]] virtual const wchar_t *getText() const { return text; }

    //!Get fontsize
    [[nodiscard]] int32_t getFontSize() const { return size; };

    //!Change the font
    bool setFont(SchriftGX2 *font);

    //!Constantly called to draw the text
    void draw(bool SRGBConversion) override;
    void process() override;

    [[nodiscard]] int32_t getTextWidth() const {
        return textWidth;
    }
    [[nodiscard]] int32_t getTextHeight() const {
        return textHeight;
    }


    void updateTextSize();

protected:
    static SchriftGX2 *presentFont;
    static int32_t presetSize;
    static int32_t presetMaxWidth;
    static int32_t presetAlignment;
    static GX2ColorF32 presetColor;

    wchar_t *text;
    int32_t size; //!< Font size
    SchriftGX2 *font;
    int32_t textWidth;
    bool textWidthDirty = true;
    int32_t textHeight{};
    bool textHeightDirty = true;
    int32_t currentSize;
    glm::vec4 color{};
    glm::vec4 colorCorrected{};
    float defaultBlur;
    float blurGlowIntensity;
    float blurAlpha;
    glm::vec4 blurGlowColor{};

    std::mutex mTextLock;
};