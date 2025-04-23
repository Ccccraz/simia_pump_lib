#ifndef AT8236_HID
#define AT8236_HID

#include <Arduino.h>
#include <USB.h>
#include <USBHID.h>

#include <atomic>
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
        0x95, 0x02,       //     REPORT_COUNT (2)
        0x91, 0x02,       //     OUTPUT (Data,Var,Abs)
        0xc0,             //   END_COLLECTION
        0xc0              // END_COLLECTION
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
        uint32_t cmd;
        uint32_t payload;
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
    QueueHandle_t task_queue{};
    AT8236HID(uint8_t in1_pin, uint8_t in2_pin, float speed);
    ~AT8236HID() = default;

    void add_task(uint32_t duration);
    auto start(uint32_t duration = 0) -> void;
    auto stop() -> void;
    auto reverse() -> void;
    auto set_speed(uint32_t speed) -> void;
    auto get_speed() -> float;

    auto begin() -> void;
    auto _onGetDescriptor(uint8_t *buffer) -> uint16_t;
    auto _onOutput(uint8_t report_id, const uint8_t *buffer, uint16_t len) -> void;
};

inline void AT8236HID::cmd_parser(report_t &report)
{
    switch (report.cmd)
    {
    case START:
        add_task(report.payload);
        break;
    case STOP:
        stop();
        break;
    case REVERSE:
        reverse();
        break;
    case SET_SPEED:
        set_speed(report.payload);
        break;
    default:
        break;
    }
}

inline void AT8236HID::work_thread(void *param)
{
    AT8236HID *pump = static_cast<AT8236HID *>(param);
    uint32_t duration{};
    while (true)
    {
        if (xQueueReceive(pump->task_queue, &duration, portMAX_DELAY) == pdPASS)
        {
            pump->start(duration);
        }
    }
}

inline void AT8236HID::add_task(uint32_t duration)
{
    xQueueSend(task_queue, &duration, 0);
}

inline void AT8236HID::start_direct()
{
    stop_direct();

    analogWrite(_in1_pin, _speed_to_report);
    analogWrite(_in2_pin, LOW);

    _rewarding = true;
}

inline void AT8236HID::stop_direct()
{
    analogWrite(_in1_pin, LOW);
    analogWrite(_in2_pin, LOW);

    _rewarding = false;
}

/// @brief Init AT8236Brushless
/// @param in1_pin Positive pin
/// @param in2_pin Negative pin
/// @param speed Initial speed
inline AT8236HID::AT8236HID(uint8_t in1_pin, uint8_t in2_pin, float speed)
    : _in1_pin(in1_pin), _in2_pin(in2_pin), _speed(constrain(speed, 0.0f, 1.0f))
{
    // Set up positive pin and negative pin
    pinMode(_in1_pin, OUTPUT);
    pinMode(_in2_pin, OUTPUT);
    _speed_to_report = static_cast<int>(speed * 255);

    // Set up HID device
    static bool initialized{false};
    if (!initialized)
    {
        initialized = true;
        _usbhid.addDevice(this, sizeof(report_descriptor));
    }

    // Create task queue
    task_queue = xQueueCreate(100, sizeof(uint32_t));

    this->stop_direct();
}

/// @brief Start the pump with a given duration
/// @param duraiton Duration of the pump
/// @return
inline auto AT8236HID::start(uint32_t duration) -> void
{
    analogWrite(_in1_pin, _speed_to_report);
    analogWrite(_in2_pin, LOW);

    _rewarding = true;

    const uint32_t start_time = millis();
    const uint32_t end_time = start_time + duration;

    if (duration == 0)
    {
        while (true)
        {
            constexpr uint32_t check_interval_ms = 100;
            vTaskDelay(pdMS_TO_TICKS(check_interval_ms));

            if (stop_request_.load())
            {
                stop_request_.store(false);
                break;
            }
        }
    }
    else
    {
        while (millis() < end_time)
        {
            constexpr uint32_t check_interval_ms = 100;
            const uint32_t remaining_time = end_time - millis();
            const uint32_t delay_time = std::min(check_interval_ms, remaining_time);

            vTaskDelay(pdMS_TO_TICKS(delay_time));

            if (stop_request_.load())
            {
                stop_request_.store(false);
                break;
            }
        }
    }
    stop_direct();
}

inline auto AT8236HID::stop() -> void
{
    analogWrite(_in1_pin, LOW);
    analogWrite(_in2_pin, LOW);

    uint32_t receivedItem{};
    while (xQueueReceive(task_queue, &receivedItem, 0) == pdTRUE)
    {
    }

    _rewarding = false;
    stop_request_.store(true);
}

inline auto AT8236HID::reverse() -> void
{
    if (_rewarding)
    {
        stop_direct();
        std::swap(_in1_pin, _in2_pin);
        start_direct();
    }
    else
    {
        std::swap(_in1_pin, _in2_pin);
    }
}

inline auto AT8236HID::begin() -> void
{
    _usbhid.begin();

    TaskHandle_t worker_task_handle{};
    xTaskCreate(work_thread, "AT8236HID", 1024 * 8, this, tskIDLE_PRIORITY + 1, &worker_task_handle);
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
    cmd_parser(report);
}

inline auto AT8236HID::set_speed(uint32_t speed) -> void
{
    _speed = constrain(speed, 0, 100) / 100.0f;
    _speed_to_report = static_cast<int>(_speed * 255);

    if (_rewarding)
    {
        analogWrite(_in1_pin, _speed_to_report);
    }
}

inline auto AT8236HID::get_speed() -> float
{
    return _speed;
}

} // namespace simia

#endif