#ifndef AT8236_HID
#define AT8236_HID

#include <Arduino.h>
#include <USB.h>
#include <USBHID.h>

#include <atomic>
#include <cstring>

#include "EEPROM.h"
#include <Preferences.h>

namespace simia
{

class AT8236HID : USBHIDDevice
{
  private:
    // HID device
    char report_descriptor[43] = {
        0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
        0x09, 0x04,       // USAGE (Joystick)
        0xa1, 0x01,       // COLLECTION (Application)
        0xa1, 0x00,       //   COLLECTION (Physical)
        0x05, 0x01,       //     USAGE_PAGE (Generic Desktop)
        0x09, 0x39,       //     USAGE (Hat switch)
        0x15, 0x01,       //     LOGICAL_MINIMUM (0)
        0x25, 0x08,       //     LOGICAL_MAXIMUM (8)
        0x75, 0x08,       //     REPORT_SIZE (8)
        0x95, 0x01,       //     REPORT_COUNT (1)
        0x81, 0x02,       //     INPUT (Data,Var,Abs)
        0x06, 0x00, 0xff, //     USAGE_PAGE (Vendor Defined Page 1)
        0x09, 0x01,       //     USAGE (Vendor Usage 1)
        0x75, 0x20,       //     REPORT_SIZE (32)
        0x95, 0x03,       //     REPORT_COUNT (3)
        0x91, 0x02,       //     OUTPUT (Data,Var,Abs)

        0x09, 0x02, //     USAGE (Vendor Usage 2)
        0x75, 0x20, //     REPORT_SIZE (32)
        0x95, 0x02, //     REPORT_COUNT (2)
        0xb1, 0x02, //     FEATURE (Data,Var,Abs)

        0xc0, //   END_COLLECTION
        0xc0  // END_COLLECTION
    };

    enum cmd_t
    {
        START = 0,
        STOP = 1,
        REVERSE = 2,
        SET_SPEED = 3,
    };

    struct report_t
    {
        uint32_t device_id;
        uint32_t cmd;
        uint32_t payload;
    };

    // TODO: Preset SSID and password
    struct feature_t
    {
        uint32_t device_id;
        uint32_t new_device_id;
    };


    // HID device
    USBHID _usbhid{};

    // Control pins
    uint8_t _in1_pin{};
    uint8_t _in2_pin{};

    // Speed control
    float _speed{};
    int _speed_to_report{};

    // Direction control
    int _direction{0};

    // Running control
    bool _rewarding{false};
    std::atomic<bool> stop_request_{false};

    void cmd_parser(report_t &report);

    static void work_thread(void *param);

    void start_direct();
    void stop_direct();

  public:
    uint32_t device_id{0};

    QueueHandle_t task_queue{};
    AT8236HID(uint8_t in1_pin, uint8_t in2_pin, float speed, uint32_t device_id);
    ~AT8236HID();

    void add_task(uint32_t duration);
    auto start(uint32_t duration = 0) -> void;
    auto stop() -> void;
    auto reverse() -> void;
    auto set_speed(uint32_t speed) -> void;
    auto get_speed() -> float;

    auto begin() -> void;
    auto _onGetDescriptor(uint8_t *buffer) -> uint16_t;
    auto _onOutput(uint8_t report_id, const uint8_t *buffer, uint16_t len) -> void;
    auto _onSetFeature(uint8_t report_id, const uint8_t *buffer, uint16_t len) -> void;
    auto _onGetFeature(uint8_t report_id, uint8_t *buffer, uint16_t len) -> uint16_t;
};


} // namespace simia

#endif