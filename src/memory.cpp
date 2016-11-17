#include "mswin.h"
#include "memory.h"

void * allocate (std::size_t n)
{
  if (! n) return nullptr;
  return ::HeapAlloc (::GetProcessHeap (), 0, n);
}

void deallocate (void * p)
{
  if (p) ::HeapFree (::GetProcessHeap (), 0, p);
}
