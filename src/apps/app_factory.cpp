#include "app_base.h"
#include "apps/app_default.h"
#include "apps/app_purger.h"
#include "apps/app_timer.h"

// Include specific apps here as they are created
// #include "apps/app_glowbutton.h"

AppBase *AppBase::create(DeviceType type)
{
        switch (type)
        {
        case PURGER:
                return new AppPurger();

        case TIMER:
                return new AppTimer();

                // case GLOW_BUTTON:
                //     return new AppGlowButton();

        default:
                return new AppDefault();
        }
}
