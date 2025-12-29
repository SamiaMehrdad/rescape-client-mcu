#pragma once
#include "app_base.h"

class AppTimer : public AppBase
{
public:
        void setup(const AppContext &context) override;
        void loop() override;
        bool handleInput(InputEvent event) override;
        void handleCommand(const RoomFrame &frame) override;
};
