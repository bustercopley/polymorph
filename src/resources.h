// -*- C++ -*-

#ifndef resource_h
#define resource_h

#include "mswin.h"

#define IDI_APPICON 1

#define IDR_DATA 1
#define IDR_VERTEX_SHADER 2
#define IDR_SHARED_GEOMETRY_SHADER 3
#define IDR_GEOMETRY_SHADER 4
#define IDR_SNUB_GEOMETRY_SHADER 5
#define IDR_FRAGMENT_SHADER 6

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
