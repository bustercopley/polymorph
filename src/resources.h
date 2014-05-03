// -*- C++ -*-

#ifndef resource_h
#define resource_h

#include "mswin.h"

#define ID_ICON 1
#define ID_DATA 2
#define ID_VERTEX_SHADER 3
#define ID_SHARED_GEOMETRY_SHADER 4
#define ID_GEOMETRY_SHADER 5
#define ID_SNUB_GEOMETRY_SHADER 6
#define ID_FRAGMENT_SHADER 7

#ifndef RC_INVOKED
namespace
{
  template <typename Size>
  inline void get_resource_data (int id, const char * & data, Size & size)
  {
    HRSRC res = ::FindResource (0, MAKEINTRESOURCE (id), RT_RCDATA);
    size = ::SizeofResource (0, res);
    data = (const char *) ::LockResource (::LoadResource (0, res));
  }
}
#endif

#endif
