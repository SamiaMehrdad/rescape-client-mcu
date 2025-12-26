#pragma once

#include "msk.h"
#include "roombus.h"
#include "inputmanager.h"
#include "roomserial.h" // Include full definition for sendFrame
#include "deviceconfig.h"

// Forward declarations to avoid circular includes
class PixelStrip;
class Synth;
class Animation;
class IOExpander;
class MatrixPanel;

/**
 * @brief Context structure passed to applications
 * Provides access to all hardware drivers and core services.
 */
struct AppContext
{
        PixelStrip *pixels;
        Synth *synth;
        Animation *animation;
        InputManager *inputManager;
        RoomSerial *roomBus;
        IOExpander *ioExpander;
        MatrixPanel *matrixPanel;
        const u8 *deviceAddress;      // Pointer to Core::m_address
        const DeviceType *deviceType; // Pointer to Core::m_type
};

/**
 * @brief Abstract base class for all device applications
 *
 * Each device type (Purger, Timer, etc.) should implement a subclass of this.
 * The Core will instantiate the correct subclass based on the configured DeviceType.
 */
class AppBase
{
public:
        virtual ~AppBase() {}

        /**
         * @brief Factory method to create the specific application instance.
         * @param type The detected device type.
         * @return Pointer to the new AppBase instance. Caller owns the pointer.
         */
        static AppBase *create(DeviceType type);

        /**
         * @brief Called once when the application starts (after device type detection)
         * @param context Access to hardware drivers
         */
        virtual void setup(const AppContext &context)
        {
                m_context = context;
        }

        /**
         * @brief Main loop update. Called frequently.
         * Use this for non-blocking logic, animations, etc.
         */
        virtual void loop() {}

        /**
         * @brief Handle an incoming RoomBus command targeted at this device
         * @param frame The received command frame
         */
        virtual void handleCommand(const RoomFrame &frame) {}

        /**
         * @brief Handle a local input event (button press, keypad, etc.)
         * @param event The input event ID
         */
        virtual void handleInput(InputEvent event) {}

        /**
         * @brief Helper to send an event to the server
         * Automatically injects [Source Address] into p[0].
         * @param event The event ID (RoomDeviceEvent)
         * @param p0 Optional parameter byte 0 (goes to p[1])
         * @param p1 Optional parameter byte 1 (goes to p[2])
         * @param p2 Optional parameter byte 2 (goes to p[3])
         * @param p3 Optional parameter byte 3 (goes to p[4])
         */
        void sendEvent(u8 event, u8 p0 = 0, u8 p1 = 0, u8 p2 = 0, u8 p3 = 0)
        {
                if (m_context.roomBus)
                {
                        RoomFrame frame;
                        room_frame_init_device(&frame, event);

                        // Standard Header for Device->Server Events
                        // p[0] = Source Address
                        if (m_context.deviceAddress)
                                frame.p[0] = *m_context.deviceAddress;

                        // Payload
                        frame.p[1] = p0;
                        frame.p[2] = p1;
                        frame.p[3] = p2;
                        frame.p[4] = p3;

                        m_context.roomBus->sendFrame(&frame);
                }
        }

protected:
        AppContext m_context;
};
