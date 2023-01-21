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
#include "GuiImage.h"
#include "shaders/ColorShader.h"
#include "utils/logger.h"
#include "utils/utils.h"

GuiImage::GuiImage(int32_t w, int32_t h, const GX2Color &c, int32_t type) {
    internalInit(w, h);
    imgType    = type;
    colorCount = ColorShader::cuColorVtxsSize / ColorShader::cuColorAttrSize;

    colorVtxs          = (uint8_t *) MEMAllocFromMappedMemoryForGX2Ex(colorCount * ColorShader::cuColorAttrSize, GX2_VERTEX_BUFFER_ALIGNMENT);
    colorVtxsCorrected = (uint8_t *) MEMAllocFromMappedMemoryForGX2Ex(colorCount * ColorShader::cuColorAttrSize, GX2_VERTEX_BUFFER_ALIGNMENT);
    if (colorVtxs && colorVtxsCorrected) {
        for (uint32_t i = 0; i < colorCount; i++) {
            setImageColor(c, i);
        }
    } else {
        DEBUG_FUNCTION_LINE_ERR("Failed to alloc colorVtxs");
    }
}

GuiImage::GuiImage(int32_t w, int32_t h, const GX2Color *c, uint32_t color_count, int32_t type) {
    internalInit(w, h);
    imgType    = type;
    colorCount = ColorShader::cuColorVtxsSize / ColorShader::cuColorAttrSize;
    if (colorCount < color_count) {
        colorCount = color_count;
    }

    colorVtxs          = (uint8_t *) MEMAllocFromMappedMemoryForGX2Ex(colorCount * ColorShader::cuColorAttrSize, GX2_VERTEX_BUFFER_ALIGNMENT);
    colorVtxsCorrected = (uint8_t *) MEMAllocFromMappedMemoryForGX2Ex(colorCount * ColorShader::cuColorAttrSize, GX2_VERTEX_BUFFER_ALIGNMENT);
    if (colorVtxs && colorVtxsCorrected) {
        for (uint32_t i = 0; i < colorCount; i++) {
            // take the last as reference if not enough colors defined
            int32_t idx = (i < color_count) ? i : (color_count - 1);
            setImageColor(c[idx], i);
        }
    } else {
        DEBUG_FUNCTION_LINE_ERR("Failed to alloc colorVtxs");
    }
}

/**
 * Destructor for the GuiImage class.
 */
GuiImage::~GuiImage() {
    if (colorVtxs) {
        MEMFreeToMappedMemory(colorVtxs);
        colorVtxs = nullptr;
    }
    if (colorVtxsCorrected) {
        MEMFreeToMappedMemory(colorVtxsCorrected);
        colorVtxsCorrected = nullptr;
    }
}

void GuiImage::internalInit(int32_t w, int32_t h) {
    width          = w;
    height         = h;
    tileHorizontal = -1;
    tileVertical   = -1;
    imgType        = IMAGE_COLOR;
    colorVtxsDirty = false;
    colorVtxs      = nullptr;
    colorCount     = 0;
    posVtxs        = nullptr;
    texCoords      = nullptr;
    vtxCount       = 4;
    primitive      = GX2_PRIMITIVE_MODE_QUADS;

    imageAngle      = 0.0f;
    blurDirection   = glm::vec3(0.0f);
    positionOffsets = glm::vec3(0.0f);
    scaleFactor     = glm::vec3(1.0f);
    colorIntensity  = glm::vec4(1.0f);
}

void GuiImage::setImageColor(const GX2Color &c, int32_t idx) {
    if (!colorVtxs || !colorVtxsCorrected) {
        return;
    }

    if (idx >= 0 && idx < (int32_t) colorCount) {
        colorVtxs[(idx << 2) + 0] = c.r;
        colorVtxs[(idx << 2) + 1] = c.g;
        colorVtxs[(idx << 2) + 2] = c.b;
        colorVtxs[(idx << 2) + 3] = c.a;

        colorVtxsCorrected[(idx << 2) + 0] = SRGBComponentToRGB(c.r);
        colorVtxsCorrected[(idx << 2) + 1] = SRGBComponentToRGB(c.g);
        colorVtxsCorrected[(idx << 2) + 2] = SRGBComponentToRGB(c.b);
        colorVtxsCorrected[(idx << 2) + 3] = c.a;
        colorVtxsDirty                     = true;
    } else if (colorVtxs) {
        for (uint32_t i = 0; i < (ColorShader::cuColorVtxsSize / sizeof(uint8_t)); i += 4) {
            colorVtxs[i + 0]          = c.r;
            colorVtxs[i + 1]          = c.g;
            colorVtxs[i + 2]          = c.b;
            colorVtxs[i + 3]          = c.a;
            colorVtxsCorrected[i + 0] = SRGBComponentToRGB(c.r);
            colorVtxsCorrected[i + 1] = SRGBComponentToRGB(c.g);
            colorVtxsCorrected[i + 2] = SRGBComponentToRGB(c.b);
            colorVtxsCorrected[i + 3] = c.a;
        }
        colorVtxsDirty = true;
    }
}

void GuiImage::setSize(float w, float h) {
    width  = w;
    height = h;
}

#define DegToRad(a) ((a) *0.01745329252f)

void GuiImage::draw(bool SRGBConversion) {
    if (!this->isVisible() || tileVertical == 0 || tileHorizontal == 0) {
        return;
    }

    float widthScaleFactor  = 1.0f / (float) 1280;
    float heightScaleFactor = 1.0f / (float) 720;
    float depthScaleFactor  = 1.0f / (float) 720;

    float currScaleX = getScaleX();
    float currScaleY = getScaleY();

    positionOffsets[0] = getCenterX() * widthScaleFactor * 2.0f;
    positionOffsets[1] = getCenterY() * heightScaleFactor * 2.0f;
    positionOffsets[2] = getDepth() * depthScaleFactor * 2.0f;

    scaleFactor[0] = currScaleX * getWidth() * widthScaleFactor;
    scaleFactor[1] = currScaleY * getHeight() * heightScaleFactor;
    scaleFactor[2] = getScaleZ();

    //! add other colors intensities parameters
    colorIntensity[3] = getAlpha();

    //! angle of the object
    imageAngle = DegToRad(getAngle());

    if (colorVtxsDirty && colorVtxs) {
        //! flush color vertex only on main GX2 thread
        GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, colorVtxs, colorCount * ColorShader::cuColorAttrSize);
        GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, colorVtxsCorrected, colorCount * ColorShader::cuColorAttrSize);
        colorVtxsDirty = false;
    }

    if (imgType == IMAGE_COLOR && colorVtxs && colorVtxsCorrected) {
        ColorShader::instance()->setShaders();
        ColorShader::instance()->setAttributeBuffer(SRGBConversion ? colorVtxsCorrected : colorVtxs, posVtxs, vtxCount);
        ColorShader::instance()->setAngle(imageAngle);
        ColorShader::instance()->setOffset(positionOffsets);
        ColorShader::instance()->setScale(scaleFactor);
        ColorShader::instance()->setColorIntensity(colorIntensity);
        ColorShader::instance()->draw(primitive, vtxCount);
    }
}
