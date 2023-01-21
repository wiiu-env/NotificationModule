#pragma once
#include "GuiFrame.h"
#include "Notification.h"
#include "sigslot.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include <forward_list>

class OverlayFrame : public GuiFrame, public sigslot::has_slots<> {

public:
    OverlayFrame(int w, int h) : GuiFrame(w, h) {
    }
    ~OverlayFrame() override = default;

    void addNotification(std::shared_ptr<Notification> status);

    void OnFadeOutFinished(GuiElement *element);

    void OnShakeFinished(GuiElement *element);

    void process() override;

    void clearElements();

private:
    std::forward_list<std::shared_ptr<Notification>> list;
};
