#ifndef SINGLETONONE_H
#define SINGLETONONE_H

#include "cppmicroservices/ServiceInterface.h"
#include "cppmicroservices/ServiceReference.h"
#include "cppmicroservices/ServiceRegistration.h"

//![s1]
class SingletonOne
{
public:
  static SingletonOne& GetInstance();

  // Just some member
  int a;

private:
  SingletonOne();
  ~SingletonOne();

  // Disable copy constructor and assignment operator.
  SingletonOne(const SingletonOne&);
  SingletonOne& operator=(const SingletonOne&);
};
//![s1]

class SingletonTwoService;

//![ss1]
class SingletonOneService
{
public:
  // This will return a SingletonOneService instance with the
  // lowest service id at the time this method was called the first
  // time and returned a non-null value (which is usually the instance
  // which was registered first). An empty object is returned if no
  // instance was registered yet.
  //
  // Note: This is a helper method to migrate traditional singletons to
  // services. Do not create a method like this in real world applications.
  static std::shared_ptr<SingletonOneService> GetInstance();

  int a;

  SingletonOneService();
  ~SingletonOneService();

private:
  // Disable copy constructor and assignment operator.
  SingletonOneService(const SingletonOneService&);
  SingletonOneService& operator=(const SingletonOneService&);
};
//![ss1]

#endif // SINGLETONONE_H
