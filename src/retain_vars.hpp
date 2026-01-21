#pragma once
#include "gui/OverlayFrame.h"
#include "gui/SchriftGX2.h"
#include <gx2/context.h>
#include <mutex>

extern GX2SurfaceFormat gTVSurfaceFormat;
extern GX2SurfaceFormat gDRCSurfaceFormat;
extern GX2ContextState *gContextState;
extern GX2ContextState *gOriginalContextState;
extern std::recursive_mutex gOverlayFrameMutex;
extern std::vector<std::shared_ptr<Notification>> gOverlayQueueDuringStartup;
extern OverlayFrame *gOverlayFrame;
extern SchriftGX2 *gFontSystem;
extern bool gOverlayInitDone;
extern bool gDrawReady;