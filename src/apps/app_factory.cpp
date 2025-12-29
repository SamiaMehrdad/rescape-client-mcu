/************************* app_factory.cpp *********************
 * Application Factory Implementation
 * Creates App instances based on DeviceType
 * Created by MSK, November 2025
 ***************************************************************/

#include "app_base.h"
#include "apps/app_default.h"
#include "apps/app_proto.h"
#include "apps/app_purger.h"
#include "apps/app_timer.h"

// Include specific apps here as they are created
// #include "apps/app_glowbutton.h"

/************************* create *****************************************
 * Factory method to create the specific application instance.
 * @param type The detected device type.
 * @return Pointer to the new AppBase instance. Caller owns the pointer.
 ***************************************************************/
AppBase *AppBase::create(DeviceType type)
{
        switch (type)
        {
        case PROTO:
                return new AppProto();

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
