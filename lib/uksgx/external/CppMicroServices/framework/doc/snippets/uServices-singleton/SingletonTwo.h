#ifndef SINGLETONTWO_H
#define SINGLETONTWO_H

#include "cppmicroservices/GlobalConfig.h"
#include "cppmicroservices/ServiceInterface.h"

class SingletonTwo
{
public:
  static SingletonTwo& GetInstance();

  int b;

private:
  SingletonTwo();
  ~SingletonTwo();

  // Disable copy constructor and assignment operator.
  SingletonTwo(const SingletonTwo&);
  SingletonTwo& operator=(const SingletonTwo&);
};

class SingletonTwoService
{
public:
  // Note: This is a helper method to migrate traditional singletons to
  // services. Do not create this method in real world applications.
  static std::shared_ptr<SingletonTwoService> GetInstance();

  int b;

  SingletonTwoService();
  ~SingletonTwoService();

private:
  // Disable copy constructor and assignment operator.
  SingletonTwoService(const SingletonTwoService&);
  SingletonTwoService& operator=(const SingletonTwoService&);
};

#endif // SINGLETONTWO_H
