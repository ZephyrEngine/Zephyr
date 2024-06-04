#include <cstddef>

void* operator new[](std::size_t size, const char* name, int flags, unsigned debugFlags, const char* file, int line) {
  return ::operator new(size);
}

void* operator new[](std::size_t size, std::size_t alignment, std::size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line) {
  return ::operator new(size);
}