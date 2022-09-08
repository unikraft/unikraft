#include "cppmicroservices/ServiceListenerHook.h"

#include <iostream>
#include <unordered_map>

using namespace cppmicroservices;

//! [1]
class MyServiceListenerHook : public ServiceListenerHook
{
private:
  class Tracked
  {
    // Do some work during construction and destruction
  };

  std::unordered_map<ListenerInfo, Tracked> tracked;

public:
  void Added(const std::vector<ListenerInfo>& listeners)
  {
    for (std::vector<ListenerInfo>::const_iterator iter = listeners.begin(),
                                                   endIter = listeners.end();
         iter != endIter;
         ++iter) {
      // Lock the tracked object for thread-safe access

      if (iter->IsRemoved())
        return;
      tracked.insert(std::make_pair(*iter, Tracked()));
    }
  }

  void Removed(const std::vector<ListenerInfo>& listeners)
  {
    for (std::vector<ListenerInfo>::const_iterator iter = listeners.begin(),
                                                   endIter = listeners.end();
         iter != endIter;
         ++iter) {
      // Lock the tracked object for thread-safe access

      // If we got a corresponding "Added" event before, the Tracked
      // destructor will do some cleanup...
      tracked.erase(*iter);
    }
  }
};
//! [1]

int main(int /*argc*/, char* /*argv*/ [])
{
  std::cout
    << "This snippet is not meant to be executed.\n"
       "It does not provide a complete working example.\n"
       "See "
       "http://docs.cppmicroservices.org/en/stable/doc/src/getting_started.html"
    << std::endl;
  return 0;
}
