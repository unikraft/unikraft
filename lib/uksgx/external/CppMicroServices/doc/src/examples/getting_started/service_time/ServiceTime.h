#include <servicetime_export.h>

#include <chrono>

struct SERVICETIME_EXPORT ServiceTime
{
  virtual ~ServiceTime();

  // Return the number of milliseconds since POSIX epoch time
  virtual std::chrono::milliseconds elapsed() const = 0;
};
