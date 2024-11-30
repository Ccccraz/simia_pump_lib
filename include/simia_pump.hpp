#ifndef SIMIA_PUMP
#define SIMIA_PUMP

namespace simia
{
class simia_pump
{
public:
  virtual void start () const = 0;
  virtual void stop () const = 0;
};
} // namespace simia

#endif