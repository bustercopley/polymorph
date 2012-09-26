#include "system.h"
#include "make_system.h"
#include "show_system.h"

#include <ostream>
#include <fstream>
#include <iostream>
namespace
{
  template <typename Stream, typename T>
  inline Stream & write_system (Stream & stream, const T & x)
  {
    stream.write (reinterpret_cast <const char *> (& x), sizeof x);
    return stream;
  }
}

int main (int argc, char * argv [])
{
  // Lay out data fields for the descriptions of three kinds of
  // spherical tiling (see "system.h")

  system_t <3, 3> tetrahedral;
  system_t <3, 4> octahedral;
  system_t <3, 5> icosahedral;

  // Fill in the data fields (see "make_system.tcc").

  make_system (tetrahedral);
  make_system (octahedral);
  make_system (icosahedral);

  // Output a human-readable summary (see "show_system.h").

  std::cout << tetrahedral << '\n'
            << octahedral << '\n'
            << icosahedral << '\n';

  // Dump the descriptions to a file.

  if (argc == 2) {
    std::ofstream stream (argv [1], std::ios_base::out | std::ios_base::binary);
    stream
      && write_system (stream, tetrahedral)
      && write_system (stream, octahedral)
      && write_system (stream, icosahedral);
  }
}
