#pragma once
#include "GuiFrame.h"
#include "GuiImage.h"
#include "GuiText.h"
#include "Timer.h"
#include "utils/logger.h"
#include <notifications/notification_defines.h>

typedef enum {
    NOTIFICATION_STATUS_INFO,
    NOTIFICATION_STATUS_ERROR,
    NOTIFICATION_STATUS_IN_PROGRESS,
} NotificationStatus;

typedef enum {
    NOTIFICATION_STATUS_NOTHING,
    NOTIFICATION_STATUS_WAIT,
    NOTIFICATION_STATUS_REQUESTED_SHAKE,
    NOTIFICATION_STATUS_EFFECT,
    NOTIFICATION_STATUS_REQUESTED_FADE_OUT_AND_EXIT,
    NOTIFICATION_STATUS_REQUESTED_EXIT,
} NotificationInternalStatus;

class OverlayFrame;

class Notification : public GuiFrame {

public:
    friend class OverlayFrame;
    explicit Notification(const std::string &overlayText,
                          NotificationStatus status                            = NOTIFICATION_STATUS_INFO,
                          float delayBeforeFadeoutInSeconds                    = 2.0f,
                          float shakeDuration                                  = 0.5f,
                          GX2Color textColor                                   = {255, 255, 255, 255},
                          GX2Color backgroundColor                             = {100, 100, 100, 255},
                          void (*finishFunc)(NotificationModuleHandle, void *) = nullptr,
                          void *context                                        = nullptr,
                          void (*removedFromOverlayCallback)(Notification *)   = nullptr,
                          bool keepUntilShown                                  = false);

    ~Notification() override;

    void process() override;
    void draw(bool SRGBConversion) override;

    void finishFunction();

    void updateText(const char *text) {
        mNotificationText.setText(text);
        mTextDirty = true;
        OSMemoryBarrier();
    }

    void updateBackgroundColor(GX2Color color) {
        mBackground.setImageColor(color);
        OSMemoryBarrier();
    }

    void updateTextColor(GX2Color textColor) {
        mNotificationText.setColor({textColor.r / 255.0f, textColor.g / 255.0f, textColor.b / 255.0f, textColor.a / 255.0f});
        OSMemoryBarrier();
    }

    void updateStatus(NotificationStatus newStatus);

    NotificationStatus getStatus() {
        return mStatus;
    }

    uint32_t getHandle() {
        return (uint32_t) this;
    }

    void updateWaitDuration(float duration) {
        this->mDelayBeforeFadeoutInSeconds = duration;
    }

    void updateShakeDuration(float duration) {
        this->mShakeDurationInSeconds = duration;
    }

    void callDeleteCallback() {
        if (mRemovedFromOverlayCallback != nullptr) {
            mRemovedFromOverlayCallback(this);
            mRemovedFromOverlayCallback = nullptr;
        }
    }

    void setPosition(float x, float y) override {
        GuiElement::setPosition(x, y);
        mPositionSet = true;
    }

    [[nodiscard]] bool isKeepUntilShown() const {
        return mKeepUntilShown;
    }

private:
    std::function<void(NotificationModuleHandle, void *)> mFinishFunction;
    std::function<void(Notification *)> mRemovedFromOverlayCallback;

    void *mFinishFunctionContext;
    GuiImage mBackground;
    GuiText mNotificationText;
    Timer mTimer;
    float mDelayBeforeFadeoutInSeconds;
    float mShakeDurationInSeconds;
    bool mFinishFunctionCalled = false;
    bool mWaitForReset         = false;

    bool mTextDirty   = false;
    bool mPositionSet = false;

    bool mKeepUntilShown = false;

    NotificationStatus mStatus                 = NOTIFICATION_STATUS_INFO;
    NotificationInternalStatus mInternalStatus = NOTIFICATION_STATUS_NOTHING;
};
