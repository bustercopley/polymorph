#include "memory.h"
#include "mswin.h"
#include "print.h"

void * allocate_internal (std::size_t n)
{
  if (! n) return nullptr;
  return ::HeapAlloc (::GetProcessHeap (), 0, n);
}

void deallocate (void * p)
{
  if (p) ::HeapFree (::GetProcessHeap (), 0, p);
}
