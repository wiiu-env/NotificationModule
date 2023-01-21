#include "gui/Notification.h"
#include "retain_vars.hpp"
#include "utils/utils.h"
#include <memory>
#include <notifications/notifications.h>
#include <wums.h>

void ExportCleanUp() {
    std::lock_guard<std::mutex> lock(gNotificationListMutex);
    gNotificationList.clear();
}

NotificationModuleStatus NMAddStaticNotification(const char *text,
                                                 NotificationModuleNotificationType type,
                                                 float durationBeforeFadeOutInSeconds,
                                                 float shakeDurationInSeconds,
                                                 NMColor textColor,
                                                 NMColor backgroundColor,
                                                 void (*finishFunc)(NotificationModuleHandle, void *context),
                                                 void *context) {
    if (!gOverlayFrame) {
        return NOTIFICATION_MODULE_RESULT_OVERLAY_NOT_READY;
    }

    NotificationStatus status;
    switch (type) {
        case NOTIFICATION_MODULE_NOTIFICATION_TYPE_INFO:
            status = NOTIFICATION_STATUS_INFO;
            break;
        case NOTIFICATION_MODULE_NOTIFICATION_TYPE_ERROR:
            status = NOTIFICATION_STATUS_ERROR;
            break;
        default:
            return NOTIFICATION_MODULE_RESULT_UNSUPPORTED_TYPE;
    }
    auto notification = make_shared_nothrow<Notification>(
            text,
            status,
            durationBeforeFadeOutInSeconds,
            shakeDurationInSeconds,
            (GX2Color){textColor.r, textColor.g, textColor.b, textColor.a},
            (GX2Color){backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a},
            finishFunc,
            context);
    if (!notification) {
        return NOTIFICATION_MODULE_RESULT_ALLOCATION_FAILED;
    }

    gOverlayFrame->addNotification(notification);

    return NOTIFICATION_MODULE_RESULT_SUCCESS;
}

void NMNotificationRemovedFromOverlay(Notification *notification) {
    if (notification) {
        auto handle = notification->getHandle();
        if (!remove_locked_first_if(gNotificationListMutex, gNotificationList, [handle](auto &cur) { return cur->getHandle() == handle; })) {
            DEBUG_FUNCTION_LINE_ERR("NMNotificationRemovedFromOverlay failed");
        }
    }
}

NotificationModuleStatus NMAddDynamicNotification(const char *text,
                                                  NMColor textColor,
                                                  NMColor backgroundColor,
                                                  void (*finishFunc)(NotificationModuleHandle, void *context),
                                                  void *context,
                                                  NotificationModuleHandle *outHandle) {
    if (outHandle == nullptr) {
        return NOTIFICATION_MODULE_RESULT_INVALID_ARGUMENT;
    }
    *outHandle = 0;
    if (!gOverlayFrame) {
        return NOTIFICATION_MODULE_RESULT_OVERLAY_NOT_READY;
    }

    auto notification = make_shared_nothrow<Notification>(
            text,
            NOTIFICATION_STATUS_IN_PROGRESS,
            0.0f,
            0.0f,
            (GX2Color){textColor.r, textColor.g, textColor.b, textColor.a},
            (GX2Color){backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a},
            finishFunc,
            context,
            NMNotificationRemovedFromOverlay);
    if (!notification) {
        return NOTIFICATION_MODULE_RESULT_ALLOCATION_FAILED;
    }

    {
        std::lock_guard<std::mutex> lock(gNotificationListMutex);
        *outHandle = notification->getHandle();
        gOverlayFrame->addNotification(notification);
        gNotificationList.push_front(notification);
    }

    return NOTIFICATION_MODULE_RESULT_SUCCESS;
}

NotificationModuleStatus NMUpdateDynamicNotificationText(NotificationModuleHandle handle,
                                                         const char *text) {
    NotificationModuleStatus res = NOTIFICATION_MODULE_RESULT_INVALID_HANDLE;
    std::lock_guard<std::mutex> lock(gNotificationListMutex);
    for (auto &cur : gNotificationList) {
        if (cur->getHandle() == handle) {
            cur->updateText(text);
            res = NOTIFICATION_MODULE_RESULT_SUCCESS;
            break;
        }
    }
    return res;
}

NotificationModuleStatus NMUpdateDynamicNotificationBackgroundColor(NotificationModuleHandle handle,
                                                                    NMColor backgroundColor) {
    NotificationModuleStatus res = NOTIFICATION_MODULE_RESULT_INVALID_HANDLE;
    std::lock_guard<std::mutex> lock(gNotificationListMutex);
    for (auto &cur : gNotificationList) {
        if (cur->getHandle() == handle) {
            cur->updateBackgroundColor((GX2Color){backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a});
            res = NOTIFICATION_MODULE_RESULT_SUCCESS;
            break;
        }
    }
    return res;
}

NotificationModuleStatus NMUpdateDynamicNotificationTextColor(NotificationModuleHandle handle,
                                                              NMColor textColor) {
    NotificationModuleStatus res = NOTIFICATION_MODULE_RESULT_INVALID_HANDLE;
    std::lock_guard<std::mutex> lock(gNotificationListMutex);
    for (auto &cur : gNotificationList) {
        if (cur->getHandle() == handle) {
            cur->updateTextColor((GX2Color){textColor.r, textColor.g, textColor.b, textColor.a});
            res = NOTIFICATION_MODULE_RESULT_SUCCESS;
            break;
        }
    }
    return res;
}

NotificationModuleStatus NMFinishDynamicNotification(NotificationModuleHandle handle,
                                                     NotificationModuleStatusFinish finishMode,
                                                     float durationBeforeFadeOutInSeconds,
                                                     float shakeDurationInSeconds) {
    NotificationStatus newStatus;
    switch (finishMode) {
        case NOTIFICATION_MODULE_STATUS_FINISH:
            newStatus = NOTIFICATION_STATUS_INFO;
            break;
        case NOTIFICATION_MODULE_STATUS_FINISH_WITH_SHAKE:
            newStatus = NOTIFICATION_STATUS_ERROR;
            break;
        default:
            return NOTIFICATION_MODULE_RESULT_INVALID_ARGUMENT;
    }

    NotificationModuleStatus res = NOTIFICATION_MODULE_RESULT_INVALID_HANDLE;
    std::lock_guard<std::mutex> lock(gNotificationListMutex);
    for (auto &cur : gNotificationList) {
        if (cur->getHandle() == handle) {
            cur->updateStatus(newStatus);
            cur->updateWaitDuration(durationBeforeFadeOutInSeconds);
            cur->updateShakeDuration(shakeDurationInSeconds);
            res = NOTIFICATION_MODULE_RESULT_SUCCESS;
            break;
        }
    }
    return res;
}

NotificationModuleStatus NMIsOverlayReady(bool *outIsReady) {
    if (outIsReady == nullptr) {
        return NOTIFICATION_MODULE_RESULT_INVALID_ARGUMENT;
    }
    if (gOverlayFrame == nullptr) {
        *outIsReady = false;
    } else {
        *outIsReady = true;
    }
    return NOTIFICATION_MODULE_RESULT_SUCCESS;
}

NotificationModuleStatus NMGetVersion(NotificationModuleAPIVersion *outVersion) {
    if (outVersion == nullptr) {
        return NOTIFICATION_MODULE_RESULT_INVALID_ARGUMENT;
    }
    *outVersion = 1;
    return NOTIFICATION_MODULE_RESULT_SUCCESS;
}

WUMS_EXPORT_FUNCTION(NMAddDynamicNotification);
WUMS_EXPORT_FUNCTION(NMAddStaticNotification);
WUMS_EXPORT_FUNCTION(NMUpdateDynamicNotificationText);
WUMS_EXPORT_FUNCTION(NMUpdateDynamicNotificationBackgroundColor);
WUMS_EXPORT_FUNCTION(NMUpdateDynamicNotificationTextColor);
WUMS_EXPORT_FUNCTION(NMFinishDynamicNotification);
WUMS_EXPORT_FUNCTION(NMIsOverlayReady);
WUMS_EXPORT_FUNCTION(NMGetVersion);
