#include <iostream>
#include <cassert>
#include <libhello/hello.hxx>

int main (int argc, char* argv[])
{
  using namespace std;

  if (argc < 2)
  {
    cerr << "error: missing name" << endl;
    return 1;
  }

  assert(hello::is_debug_mode() && "config.libhello.debug must be set to `true`");

  hello::say_hello(cout, argv[1]);
}
