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

#define IDD_CONFIGURE 100
#define IDC_MESSAGE 101
#define IDC_COUNT_TRACKBAR 102
#define IDC_SPEED_TRACKBAR 103
#define IDC_MIN_COUNT_STATIC 104
#define IDC_MAX_COUNT_STATIC 105
#define IDC_MIN_SPEED_STATIC 106
#define IDC_MAX_SPEED_STATIC 107
#define IDC_PREVIEW_BUTTON 108
#define IDC_BUTTONS_STATIC 109

#ifndef IDC_STATIC
#define IDC_STATIC -1
#endif

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
