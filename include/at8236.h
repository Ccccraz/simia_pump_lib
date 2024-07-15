#ifndef AT8236_H
#define AT8236_H
#include <Arduino.h>

namespace simia
{
class AT8236
{
  private:
    uint8_t _positive_pin{};
    uint8_t _negative_pin{};
    float _speed{};
    int _speed_real{};

    bool _isrunning{false};

  public:
    AT8236(uint8_t firstPin, uint8_t lastPin, float speed = 1);
    auto start() -> void;
    auto stop() -> void;
    auto reverse() -> void;
    auto set_speed(float speed) -> void;
    auto get_speed() -> uint8_t;
    ~AT8236() = default;
};

inline AT8236::AT8236(uint8_t firstPin, uint8_t lastPin, float speed)
    : _positive_pin(firstPin), _negative_pin(lastPin), _speed(constrain(speed, 0.0f, 1.0f))
{
    pinMode(_positive_pin, OUTPUT);
    pinMode(_negative_pin, OUTPUT);
    _speed_real = static_cast<int>(speed * 255);
    this->stop();
}

inline auto AT8236::start() -> void
{
    this->stop();

    analogWrite(_positive_pin, _speed_real);
    analogWrite(_negative_pin, LOW);

    _isrunning = true;
}

inline auto AT8236::stop() -> void
{
    analogWrite(_positive_pin, LOW);
    analogWrite(_negative_pin, LOW);

    _isrunning = false;
}

inline auto AT8236::reverse() -> void
{
    if (_isrunning)
    {
        this->stop();
        std::swap(_positive_pin, _negative_pin);
        this->start();
    }
    else
    {
        std::swap(_positive_pin, _negative_pin);
    }
}

inline auto AT8236::set_speed(float speed) -> void
{
    _speed = constrain(speed, 0.0f, 1.0f);
    _speed_real = static_cast<int>(_speed * 255);
    if (_isrunning)
    {
        analogWrite(_positive_pin, _speed_real);
    }
}

inline auto AT8236::get_speed() -> uint8_t
{
    return _speed;
}
} // namespace simia

#endif