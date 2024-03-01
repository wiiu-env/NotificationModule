#include "retain_vars.hpp"

GX2SurfaceFormat gTVSurfaceFormat                                     = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
GX2SurfaceFormat gDRCSurfaceFormat                                    = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
GX2ContextState *gContextState                                        = nullptr;
GX2ContextState *gOriginalContextState                                = nullptr;
std::mutex gOverlayFrameMutex                                         = {};
std::vector<std::shared_ptr<Notification>> gOverlayQueueDuringStartup = {};
OverlayFrame *gOverlayFrame                                           = nullptr;
SchriftGX2 *gFontSystem                                               = nullptr;
bool gOverlayInitDone                                                 = false;
bool gDrawReady                                                       = false;