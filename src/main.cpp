#include "export.h"
#include "function_patches.h"
#include "retain_vars.hpp"
#include "shaders/ColorShader.h"
#include "shaders/Texture2DShader.h"
#include "utils/logger.h"
#include "version.h"
#include <coreinit/memory.h>
#include <function_patcher/function_patching.h>
#include <wums.h>

WUMS_MODULE_EXPORT_NAME("homebrew_notifications");

#define VERSION "v0.1.2"

WUMS_DEPENDS_ON(homebrew_memorymapping);
WUMS_DEPENDS_ON(homebrew_functionpatcher);

WUMS_INITIALIZE() {
    initLogging();

    if (FunctionPatcher_InitLibrary() != FUNCTION_PATCHER_RESULT_SUCCESS) {
        OSFatal("NotificationModule: FunctionPatcher_InitLibrary failed");
    }
    DEBUG_FUNCTION_LINE("Patch NotificationModule functions");
    for (uint32_t i = 0; i < function_replacements_size; i++) {
        if (FunctionPatcher_AddFunctionPatch(&function_replacements[i], nullptr, nullptr) != FUNCTION_PATCHER_RESULT_SUCCESS) {
            OSFatal("NotificationModule: Failed to patch NotificationModule function");
        }
    }
    DEBUG_FUNCTION_LINE("Patch NotificationModule functions finished");

    gOverlayInitDone = false;

    DEBUG_FUNCTION_LINE_VERBOSE("Init context state");
    // cannot be in unknown regions for GX2 like 0xBCAE1000
    gContextState = (GX2ContextState *) MEMAllocFromMappedMemoryForGX2Ex(
            sizeof(GX2ContextState),
            GX2_CONTEXT_STATE_ALIGNMENT);
    if (gContextState == nullptr) {
        OSFatal("NotificationModule: Failed to allocate gContextState");
    } else {
        DEBUG_FUNCTION_LINE_VERBOSE("Allocated %d bytes for gContextState", sizeof(GX2ContextState));
    }

    void *font    = nullptr;
    uint32_t size = 0;
    if (OSGetSharedData(OS_SHAREDDATATYPE_FONT_STANDARD, 0, &font, &size) && font && size > 0) {
        gFontSystem = new (std::nothrow) SchriftGX2((uint8_t *) font, (int32_t) size);
    }
    if (gFontSystem != nullptr) {
        GuiText::setPresetFont(gFontSystem);
    } else {
        OSFatal("NotificationModule: Failed to init font system");
    }

    OSMemoryBarrier();
    deinitLogging();
}

WUMS_APPLICATION_STARTS() {
    initLogging();
    OSReport("Running NotificationModule " VERSION VERSION_EXTRA "\n");
    gDrawReady = false;
}

WUMS_APPLICATION_ENDS() {
    gDrawReady = false;
    if (gOverlayFrame) {
        gOverlayFrame->clearElements();
    }
    ExportCleanUp();
    if (gFontSystem) {
        gFontSystem->unloadFont();
    }
    ColorShader::destroyInstance();
    Texture2DShader::destroyInstance();
    deinitLogging();
}

WUMS_DEINITIALIZE() {
    delete gOverlayFrame;
    delete gFontSystem;
    MEMFreeToMappedMemory(gContextState);
}