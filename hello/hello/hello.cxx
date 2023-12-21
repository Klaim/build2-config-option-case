#include <iostream>
#include <libhello/hello.hxx>

int main (int argc, char* argv[])
{
  using namespace std;

  if (argc < 2)
  {
    cerr << "error: missing name" << endl;
    return 1;
  }

  if(hello::is_debug_mode())
  {
    cout << "\nTHIS IS DEBUG MODE" << endl;
  }

  hello::say_hello(cout, argv[1]);
}
