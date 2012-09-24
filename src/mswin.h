// -*- C++ -*-

#ifndef mswin_h
#define mswin_h

//#define WINVER 0x500
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace
{
  const char * get_resource_data (int id, int * size_var)
  {
    const char * data = nullptr;
    unsigned size = 0;
    if (id) {
      if (HRSRC data_found = ::FindResource (0, MAKEINTRESOURCE (id), MAKEINTRESOURCE (id))) {
        size = ::SizeofResource (0, data_found);
        if (HGLOBAL data_loaded = ::LoadResource (0, data_found)) {
          data = reinterpret_cast <const char *> (::LockResource (data_loaded));
        }
      }
    }
    if (! data) {
      data = "";
    }
    if (size_var) {
      * size_var = size;
    }
    return data;
  }
}

#endif
