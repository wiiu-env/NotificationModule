#include "Notification.h"

Notification::Notification(const std::string &overlayText,
                           NotificationStatus status,
                           float delayBeforeFadeoutInSeconds,
                           float shakeDuration,
                           GX2Color textColor,
                           GX2Color backgroundColor,
                           void (*finishFunc)(NotificationModuleHandle, void *),
                           void *context,
                           void (*removedFromOverlayCallback)(Notification *)) : GuiFrame(0, 0), mBackground(0, 0, backgroundColor) {
    mFinishFunction              = finishFunc;
    mFinishFunctionContext       = context;
    mRemovedFromOverlayCallback  = removedFromOverlayCallback;
    mDelayBeforeFadeoutInSeconds = delayBeforeFadeoutInSeconds;
    mShakeDurationInSeconds      = shakeDuration;
    mBackground.setImageColor(backgroundColor);

    mNotificationText.setColor({textColor.r / 255.0f, textColor.g / 255.0f, textColor.b / 255.0f, textColor.a / 255.0f});
    mNotificationText.setPosition(0, 0);
    mNotificationText.setFontSize(20);
    mNotificationText.setAlignment(ALIGN_CENTERED);

    updateStatus(status);

    if (!overlayText.empty()) {
        updateText(overlayText.c_str());
    }

    mWaitForReset = true;

    append(&mBackground);
    append(&mNotificationText);
}

Notification::~Notification() {
    finishFunction();
    remove(&mNotificationText);
    remove(&mBackground);
}

void Notification::process() {
    GuiFrame::process();

    if (mWaitForReset) {
        mTimer.reset();
        mWaitForReset = false;
        return;
    }

    if (mInternalStatus == NOTIFICATION_STATUS_WAIT) {
        if (mTimer.elapsed() >= mDelayBeforeFadeoutInSeconds) {
            mInternalStatus = NOTIFICATION_STATUS_REQUESTED_FADE_OUT_AND_EXIT;
        }
    }
}

void Notification::finishFunction() {
    if (!mFinishFunctionCalled && mFinishFunction) {
        mFinishFunction(this->getHandle(), mFinishFunctionContext);
        mFinishFunctionCalled = true;
    }
}

void Notification::draw(bool SRGBConversion) {
    if (!mPositionSet) {
        return;
    }
    if (mTextDirty) {
        mNotificationText.updateTextSize();
        mTextDirty = false;
    }
    width  = (float) mNotificationText.getTextWidth() + 25;
    height = (float) mNotificationText.getTextHeight() + 25;

    mBackground.setSize(width, height);
    if (width > 25 || height > 25) {
        GuiFrame::draw(SRGBConversion);
    }
}

void Notification::updateStatus(NotificationStatus newStatus) {
    switch (newStatus) {
        case NOTIFICATION_STATUS_INFO:
            mInternalStatus = NOTIFICATION_STATUS_WAIT;
            break;
        case NOTIFICATION_STATUS_ERROR:
            mInternalStatus = NOTIFICATION_STATUS_REQUESTED_SHAKE;
            break;
        case NOTIFICATION_STATUS_IN_PROGRESS:
            mInternalStatus = NOTIFICATION_STATUS_NOTHING;
            break;
        default:
            return;
    }
    mWaitForReset = true;
    this->mStatus = newStatus;
}