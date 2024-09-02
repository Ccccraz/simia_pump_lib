#ifndef L298N_H
#define L298N_H
#include <Arduino.h>

namespace simia
{

class L298N
{
  public:
    L298N(int postivePin, int negativePin);
    L298N(int postivePin, int negativePin, int pwmPin, float speed);
    ~L298N() = default;
    auto start() -> void;
    auto stop() -> void;
    auto reverse() -> void;
    auto set_speed(float speed) -> void;
    auto getSpeed() -> int;

  private:
    int _positivePin{};
    int _negativePin{};
    bool _rewarding{false};
    bool _is_reversing{false};
    int _PWMPin{};

    bool _enableSpeedCtrl{false};
    float _speedPre{1};
    int _speed{255};
};

inline L298N::L298N(int positivePin, int negativePin) : L298N(positivePin, negativePin, -1, 255)
{
}

inline L298N::L298N(int positivePin, int negativePin, int pwmPin, float speed)
    : _positivePin(positivePin), _negativePin(negativePin), _PWMPin(pwmPin), _speedPre(constrain(speed, 0.0f, 1.0f))
{
    pinMode(_positivePin, OUTPUT);
    pinMode(_negativePin, OUTPUT);

    if (_PWMPin <= -1)
    {
        _enableSpeedCtrl = false;
    }
    else
    {
        _enableSpeedCtrl = true;
        _speed = static_cast<int>(_speedPre * 255);
        pinMode(_PWMPin, OUTPUT);
    }
}

inline auto L298N::start() -> void
{
    this->stop();
    digitalWrite(_positivePin, HIGH);
    digitalWrite(_negativePin, LOW);
    if (_enableSpeedCtrl)
    {
        analogWrite(_PWMPin, _speed);
    }

    _rewarding = true;
}

inline auto L298N::stop() -> void
{
    digitalWrite(_positivePin, LOW);
    digitalWrite(_negativePin, LOW);

    _rewarding = false;
}

inline auto L298N::reverse() -> void
{
    std::swap<int>(_positivePin, _negativePin);
    if (_rewarding)
    {
        this->start();
    }
}

inline auto L298N::set_speed(float speed) -> void
{
    _speedPre = constrain(speed, 0.0f, 1.0f);
    _speed = static_cast<int>(_speedPre * 255);
    analogWrite(_PWMPin, _speed);
}

inline auto L298N::getSpeed() -> int
{
    return _speed;
}
} // namespace simia

#endif