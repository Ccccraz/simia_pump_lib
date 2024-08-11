#ifndef AT8236_BRUSHLESS_H
#define AT8236_BRUSHLESS_H
#include <Arduino.h>

namespace simia
{
class AT8236Brushless
{
  private:
    uint8_t _in1_pin{};
    uint8_t _in2_pin{};
    uint8_t _speed_ctrl_pin{};
    uint8_t _report_pin{};
    uint8_t _direction_ctrl_pin{};
    float _speed{};
    int _speed_real{};

    bool _is_running{};

  public:
    AT8236Brushless(uint8_t in1_pin, uint8_t in2_pin, float speed, uint8_t speed_ctrl_pin, uint8_t report_pin,
                    uint8_t direction_ctrl_pin);
    ~AT8236Brushless() = default;
    auto start() -> void;
    auto start_duration(int duration) -> void;
    auto stop() -> void;
    auto reverse() -> void;
    auto set_speed(float speed) -> void;
    auto get_speed() -> float;
};

inline AT8236Brushless::AT8236Brushless(uint8_t in1_pin, uint8_t in2_pin, float speed, uint8_t speed_ctrl_pin,
                                        uint8_t report_pin, uint8_t direction_ctrl_pin)
    : _in1_pin(in1_pin), _in2_pin(in2_pin), _speed(constrain(speed, 0.0f, 1.0f)), _speed_ctrl_pin(speed_ctrl_pin),
      _report_pin(report_pin), _direction_ctrl_pin(direction_ctrl_pin)
{
    pinMode(_in1_pin, OUTPUT);
    pinMode(_in2_pin, OUTPUT);
    pinMode(_speed_ctrl_pin, OUTPUT);
    pinMode(_report_pin, INPUT_PULLDOWN);
    pinMode(_direction_ctrl_pin, OUTPUT);
    _speed_real = static_cast<int>(speed * 255);
    this->stop();
}

inline auto AT8236Brushless::start() -> void
{
    this->stop();

    digitalWrite(_in1_pin, LOW);
    digitalWrite(_in2_pin, HIGH);
    analogWrite(_speed_ctrl_pin, _speed_real);

    _is_running = true;
}

inline auto AT8236Brushless::start_duration(int duration) -> void
{
    this->start();

    vTaskDelay(duration);

    this->stop();
}

inline auto AT8236Brushless::stop() -> void
{
    digitalWrite(_in1_pin, LOW);
    digitalWrite(_in2_pin, LOW);
    analogWrite(_speed_ctrl_pin, LOW);

    _is_running = false;
}

inline auto AT8236Brushless::reverse() -> void
{
  digitalWrite(_direction_ctrl_pin, HIGH);

  vTaskDelay(pdMS_TO_TICKS(1));

  digitalWrite(_direction_ctrl_pin, LOW);
}

inline auto AT8236Brushless::get_speed() -> float
{
    return _speed;
}

inline auto AT8236Brushless::set_speed(float speed) -> void
{
    _speed = constrain(speed, 0.0f, 1.0f);
    _speed_real = static_cast<int>(speed * 255);

    if (_is_running)
    {
      analogWrite(_speed_ctrl_pin, _speed_real);
    }
    
}

} // namespace simia

#endif