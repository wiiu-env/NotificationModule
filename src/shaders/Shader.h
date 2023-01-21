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

#include "gx2_ext.h"
#pragma GCC diagnostic ignored "-Wvolatile"
#include <glm/glm.hpp>
#include <gx2/draw.h>
#include <gx2/enum.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/shaders.h>
#include <gx2/texture.h>
#include <memory/mappedmemory.h>

class Shader {
protected:
    Shader()          = default;
    virtual ~Shader() = default;

public:
    static const uint16_t cuVertexAttrSize   = sizeof(float) * 3;
    static const uint16_t cuTexCoordAttrSize = sizeof(float) * 2;
    static const uint16_t cuColorAttrSize    = sizeof(uint8_t) * 4;

    static void setLineWidth(const float &width) {
        GX2SetLineWidth(width);
    }

    static void draw(int32_t primitive = GX2_PRIMITIVE_MODE_QUADS, uint32_t vtxCount = 4) {
        switch (primitive) {
            default:
            case GX2_PRIMITIVE_MODE_QUADS: {
                GX2DrawEx(GX2_PRIMITIVE_MODE_QUADS, vtxCount, 0, 1);
                break;
            }
            case GX2_PRIMITIVE_MODE_TRIANGLES: {
                GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLES, vtxCount, 0, 1);
                break;
            }
            case GX2_PRIMITIVE_MODE_TRIANGLE_FAN: {
                GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLE_FAN, vtxCount, 0, 1);
                break;
            }
            case GX2_PRIMITIVE_MODE_LINES: {
                GX2DrawEx(GX2_PRIMITIVE_MODE_LINES, vtxCount, 0, 1);
                break;
            }
            case GX2_PRIMITIVE_MODE_LINE_STRIP: {
                GX2DrawEx(GX2_PRIMITIVE_MODE_LINE_STRIP, vtxCount, 0, 1);
                break;
            }
                //! TODO: add other primitives later
        };
    }
};