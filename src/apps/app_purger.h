#pragma once
#include "app_base.h"

class AppPurger : public AppBase
{
public:
        void setup(const AppContext &context) override;
        void loop() override;
        void handleInput(InputEvent event) override;
        void handleCommand(const RoomFrame &frame) override;
};
