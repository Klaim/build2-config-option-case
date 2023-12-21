#pragma once

#include <iosfwd>
#include <string>

#include <libhello/config.hxx>
#include <libhello/export.hxx>

namespace hello
{
  // Print a greeting for the specified name into the specified
  // stream. Throw std::invalid_argument if the name is empty.
  //
  LIBHELLO_SYMEXPORT void
  say_hello (std::ostream&, const std::string& name);


  constexpr bool is_debug_mode()
  {
    return
#ifdef LIBHELLO_DEBUG_MODE
      true
#else
      false
#endif
    ;
  }

}
