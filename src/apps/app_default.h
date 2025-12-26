#pragma once
#include "app_base.h"
#include "pixel.h"

class AppDefault : public AppBase
{
public:
        void setup(const AppContext &context) override
        {
                AppBase::setup(context);
                // Default behavior: Clear pixels
                if (m_context.pixels)
                {
                        m_context.pixels->clear();
                        m_context.pixels->show();
                }
        }

        void loop() override
        {
                // Do nothing by default
        }
};
