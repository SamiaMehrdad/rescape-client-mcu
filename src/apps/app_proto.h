#pragma once
#include "app_base.h"

class MusicPlayer;

class AppProto : public AppBase
{
public:
        ~AppProto();
        void setup(const AppContext &context) override;
        void loop() override;
        bool handleInput(InputEvent event) override;
        void handleCommand(const RoomFrame &frame) override;

private:
        MusicPlayer *m_player = nullptr;
};
