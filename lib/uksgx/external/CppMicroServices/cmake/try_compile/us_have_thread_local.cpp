#include <string>

// We use a non-POD type for this test
static thread_local std::string dummy;
int main()
{
  return 0;
}
