#include "retain_vars.hpp"
#include "shaders/ColorShader.h"
#include "shaders/Texture2DShader.h"
#include <function_patcher/fpatching_defines.h>
#include <gx2/state.h>

bool drawScreenshotSavedTexture(const GX2ColorBuffer *colorBuffer, GX2ScanTarget scan_target);

GX2ColorBuffer lastTVColorBuffer;
GX2ColorBuffer lastDRCColorBuffer;
DECL_FUNCTION(void, GX2GetCurrentScanBuffer, GX2ScanTarget scanTarget, GX2ColorBuffer *cb) {
    real_GX2GetCurrentScanBuffer(scanTarget, cb);
    if (scanTarget == GX2_SCAN_TARGET_TV) {
        memcpy(&lastTVColorBuffer, cb, sizeof(GX2ColorBuffer));
    } else {
        memcpy(&lastDRCColorBuffer, cb, sizeof(GX2ColorBuffer));
    }
}

DECL_FUNCTION(void, GX2SetContextState, GX2ContextState *curContext) {
    real_GX2SetContextState(curContext);

    gOriginalContextState = curContext;
}

DECL_FUNCTION(void, GX2SetupContextStateEx, GX2ContextState *state, BOOL unk1) {
    real_GX2SetupContextStateEx(state, unk1);
    gOriginalContextState = state;
    DEBUG_FUNCTION_LINE_VERBOSE("gOriginalContextState = %08X", state);
}

DECL_FUNCTION(void, GX2SetTVBuffer, void *buffer, uint32_t buffer_size, int32_t tv_render_mode, GX2SurfaceFormat surface_format, GX2BufferingMode buffering_mode) {
    DEBUG_FUNCTION_LINE_VERBOSE("Set TV Buffer format to 0x%08X", surface_format);
    gTVSurfaceFormat = surface_format;

    return real_GX2SetTVBuffer(buffer, buffer_size, tv_render_mode, surface_format, buffering_mode);
}

DECL_FUNCTION(void, GX2SetDRCBuffer, void *buffer, uint32_t buffer_size, uint32_t drc_mode, GX2SurfaceFormat surface_format, GX2BufferingMode buffering_mode) {
    DEBUG_FUNCTION_LINE_VERBOSE("Set DRC Buffer format to 0x%08X", surface_format);
    gDRCSurfaceFormat = surface_format;

    return real_GX2SetDRCBuffer(buffer, buffer_size, drc_mode, surface_format, buffering_mode);
}

void drawIntoColorBuffer(const GX2ColorBuffer *colorBuffer, OverlayFrame *overlayFrame, GX2ScanTarget scan_target) {
    real_GX2SetContextState(gContextState);

    GX2SetDefaultState();

    GX2SetColorBuffer((GX2ColorBuffer *) colorBuffer, GX2_RENDER_TARGET_0);
    GX2SetViewport(0.0f, 0.0f, colorBuffer->surface.width, colorBuffer->surface.height, 0.0f, 1.0f);
    GX2SetScissor(0, 0, colorBuffer->surface.width, colorBuffer->surface.height);

    GX2SetDepthOnlyControl(GX2_FALSE, GX2_FALSE, GX2_COMPARE_FUNC_NEVER);
    GX2SetAlphaTest(GX2_TRUE, GX2_COMPARE_FUNC_GREATER, 0.0f);
    GX2SetColorControl(GX2_LOGIC_OP_COPY, GX2_ENABLE, GX2_DISABLE, GX2_ENABLE);
    auto outputFormat = scan_target ? gTVSurfaceFormat : gDRCSurfaceFormat;
    overlayFrame->draw(outputFormat & 0x400);
    GX2Flush();

    real_GX2SetContextState(gOriginalContextState);
}


void drawScreenshotSavedTexture2(GX2ColorBuffer *colorBuffer, GX2ScanTarget scan_target) {
    if (gOverlayFrame->empty()) {
        return;
    }

    drawIntoColorBuffer(colorBuffer, gOverlayFrame, scan_target);
}

DECL_FUNCTION(void, GX2CopyColorBufferToScanBuffer, const GX2ColorBuffer *colorBuffer, GX2ScanTarget scan_target) {
    gDrawReady = true;
    if (drawScreenshotSavedTexture(colorBuffer, scan_target)) {
        // if it returns true we don't need to call GX2CopyColorBufferToScanBuffer
        return;
    }

    real_GX2CopyColorBufferToScanBuffer(colorBuffer, scan_target);
}

bool drawScreenshotSavedTexture(const GX2ColorBuffer *colorBuffer, GX2ScanTarget scan_target) {
    if (gOverlayFrame->empty()) {
        return false;
    }
    GX2ColorBuffer cb;
    GX2InitColorBuffer(&cb,
                       colorBuffer->surface.dim,
                       colorBuffer->surface.width,
                       colorBuffer->surface.height,
                       colorBuffer->surface.depth,
                       colorBuffer->surface.format,
                       colorBuffer->surface.aa,
                       colorBuffer->surface.tileMode,
                       colorBuffer->surface.swizzle,
                       colorBuffer->aaBuffer,
                       colorBuffer->aaSize);

    cb.surface.image = colorBuffer->surface.image;

    drawIntoColorBuffer(&cb, gOverlayFrame, scan_target);

    real_GX2CopyColorBufferToScanBuffer(&cb, scan_target);

    return true;
}

DECL_FUNCTION(void, GX2Init, uint32_t attributes) {
    real_GX2Init(attributes);
    if (!gOverlayInitDone) {
        DEBUG_FUNCTION_LINE_VERBOSE("Init Overlay");
        gOverlayFrame = new (std::nothrow) OverlayFrame(1280.0f, 720.0f);
        if (!gOverlayFrame) {
            OSFatal("Failed to alloc gOverlayFrame");
        }

        // Allocate shader.
        ColorShader::instance();
        Texture2DShader::instance();

        // has been allocated in WUMS INIT
        if (!gContextState) {
            OSFatal("Failed to alloc gContextState");
        }
        real_GX2SetupContextStateEx(gContextState, GX2_TRUE);
        DCInvalidateRange(gContextState, sizeof(GX2ContextState)); // Important!
        gOverlayInitDone = true;
    }
}

DECL_FUNCTION(void, GX2MarkScanBufferCopied, GX2ScanTarget scan_target) {
    gDrawReady = true;
    if (scan_target == GX2_SCAN_TARGET_TV) {
        drawScreenshotSavedTexture2(&lastTVColorBuffer, scan_target);
    } else {
        drawScreenshotSavedTexture2(&lastDRCColorBuffer, scan_target);
    }

    real_GX2MarkScanBufferCopied(scan_target);
}

DECL_FUNCTION(void, GX2SwapScanBuffers, void) {
    if (gDrawReady && !gOverlayFrame->empty()) {
        gOverlayFrame->process();
        gOverlayFrame->updateEffects();
    }
    real_GX2SwapScanBuffers();
}

function_replacement_data_t function_replacements[] = {
        REPLACE_FUNCTION(GX2GetCurrentScanBuffer, LIBRARY_GX2, GX2GetCurrentScanBuffer),
        REPLACE_FUNCTION(GX2SetContextState, LIBRARY_GX2, GX2SetContextState),
        REPLACE_FUNCTION(GX2SetupContextStateEx, LIBRARY_GX2, GX2SetupContextStateEx),
        REPLACE_FUNCTION(GX2SetTVBuffer, LIBRARY_GX2, GX2SetTVBuffer),
        REPLACE_FUNCTION(GX2SetDRCBuffer, LIBRARY_GX2, GX2SetDRCBuffer),
        REPLACE_FUNCTION(GX2CopyColorBufferToScanBuffer, LIBRARY_GX2, GX2CopyColorBufferToScanBuffer),
        REPLACE_FUNCTION(GX2Init, LIBRARY_GX2, GX2Init),
        REPLACE_FUNCTION(GX2MarkScanBufferCopied, LIBRARY_GX2, GX2MarkScanBufferCopied),
        REPLACE_FUNCTION(GX2SwapScanBuffers, LIBRARY_GX2, GX2SwapScanBuffers),
};

uint32_t function_replacements_size = sizeof(function_replacements) / sizeof(function_replacement_data_t);
