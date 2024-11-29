#ifndef AT8236_HID
#define AT8236_HID
#include <Arduino.h>
#include <USB.h>
#include <USBHID.h>
#include <cstring>

namespace simia
{
class AT8236HID : USBHIDDevice
{
  private:
    // HID device
    char report_descriptor[35] = {
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
        0x95, 0x05,       //     REPORT_COUNT (5)
        0x91, 0x02,       //     OUTPUT (Data,Var,Abs)
        0xc0,             //   END_COLLECTION
        0xc0              // END_COLLECTION
    };

    // Output report
    struct report_t
    {
        float reward; // Start pump when value = 1, otherwise keep value = 0
        float duration; // Start pump and stop after a certain time, unit is milliseconds
        float stop; // Stop pump when value = 1, otherwise keep value = 0
        float speed; // Set the running speed of the pump, the value range is 0 - 1, representing the percentage of the maximum speed of the pump
        float reverse; // Change the running direction of the pump, if value = 1, reverse the pump, otherwise keep value = 0
    };
    // example command:
    // start pump: [1, 0, 0, 0, 0]
    // start pump for 1s: [1, 1000, 0, 0, 0]
    // stop pump: [0, 0, 1, 0, 0]
    // set speed: [0, 0, 0, 0.5, 0]
    // reverse pump: [0, 0, 0 , 0, 1]

    // HID device
    USBHID _usbhid{};

    // Control pins
    uint8_t _in1_pin{};
    uint8_t _in2_pin{};
    // Speed control
    float _speed{};
    int _speed_to_report{};

    // Direction control
    unsigned long _last_reverse_time;
    int _direction{0};

    // Running control
    bool _rewarding{false};
    bool _running{false};

  public:
    AT8236HID(uint8_t in1_pin, uint8_t in2_pin, float speed);
    ~AT8236HID() = default;

    auto start(int duration = -1) -> void;
    auto stop() -> void;
    auto reverse() -> void;
    auto set_speed(float speed) -> void;
    auto get_speed() -> float;

    auto begin() -> void;
    auto _onGetDescriptor(uint8_t *buffer) -> uint16_t;
    auto _onOutput(uint8_t report_id, const uint8_t *buffer, uint16_t len) -> void;
};

/// @brief Init AT8236Brushless
/// @param in1_pin Positive pin
/// @param in2_pin Negative pin
/// @param speed Initial speed
/// @param speed_ctrl_pin Speed control pin
/// @param report_pin Report pin
/// @param direction_ctrl_pin Running direction control pin
inline AT8236HID::AT8236HID(uint8_t in1_pin, uint8_t in2_pin, float speed)
    : _in1_pin(in1_pin), _in2_pin(in2_pin), _speed(constrain(speed, 0.0f, 1.0f))
{
    // Set up positive pin and negative pin
    pinMode(_in1_pin, OUTPUT);
    pinMode(_in2_pin, OUTPUT);
    _speed_to_report = static_cast<int>(speed * 255);

    // Anti-shake
    _last_reverse_time = millis();

    // Set up HID device
    static bool initialized{false};
    if (!initialized)
    {
        initialized = true;
        _usbhid.addDevice(this, sizeof(report_descriptor));
    }

    this->stop();
}

/// @brief Start the pump with a given duration
/// @param duraiton Duration of the pump
/// @return
inline auto AT8236HID::start(int duraiton) -> void
{
    this->stop();

    analogWrite(_in1_pin, _speed_to_report);
    analogWrite(_in2_pin, LOW);

    _rewarding = true;

    if (duraiton > 0)
    {
        vTaskDelay(duraiton);
        this->stop();
    }
}

inline auto AT8236HID::stop() -> void
{
    analogWrite(_in1_pin, LOW);
    analogWrite(_in2_pin, LOW);

    _rewarding = false;
}

inline auto AT8236HID::reverse() -> void
{
    // Get current time for anti-shake
    auto currecnt_time = millis();

    if (currecnt_time - _last_reverse_time > 1000)
    {
        if (_rewarding)
        {
            this->stop();
            std::swap(_in1_pin, _in2_pin);
            this->start();
        }
        else
        {
            std::swap(_in1_pin, _in2_pin);
        }

        // Update last reverse time
        _last_reverse_time = millis();
    }
}

inline auto AT8236HID::begin() -> void
{
    _usbhid.begin();
}

inline auto AT8236HID::_onGetDescriptor(uint8_t *buffer) -> uint16_t
{
    memcpy(buffer, report_descriptor, sizeof(report_descriptor));
    return sizeof(report_descriptor);
}

inline auto AT8236HID::_onOutput(uint8_t report_id, const uint8_t *buffer, uint16_t len) -> void
{
    report_t report{};
    memcpy(&report, buffer, sizeof(report));
    if (report.reward == 1)
    {
        if (report.duration > 0)
        {
            this->start(report.duration);
        }
        else
        {
            this->start();
        }
    }
    else if (report.stop == 1)
    {
        this->stop();
    }
    else if (report.speed > 0)
    {
        this->set_speed(report.speed);
    }
    else if (report.reverse == 1)
    {
        this->reverse();
    }
}

inline auto AT8236HID::set_speed(float speed) -> void
{
    _speed = constrain(speed, 0.0f, 1.0f);
    _speed_to_report = static_cast<int>(_speed * 255);

    if (_rewarding)
    {
        analogWrite(_in1_pin, _speed_to_report);
    }
}

} // namespace simia

#endif