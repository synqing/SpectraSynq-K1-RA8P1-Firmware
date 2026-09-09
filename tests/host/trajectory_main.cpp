#include "trajectory.h"
#include <new>
#include <cstdlib>
static bool processing = false;
static unsigned allocations = 0;
void* operator new(std::size_t size) { if (processing) ++allocations; if (auto p = std::malloc(size)) return p; std::abort(); }
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
static fixture::Trajectory trajectory;
static fixture::Trace trace;
int main() {
  std::int16_t hop[180];
  for (;;) {
    const auto count = std::fread(hop, sizeof(std::int16_t), 180, stdin);
    if (count == 0 && std::feof(stdin)) break;
    if (count != 180) return 3;
    processing = true; trajectory.process(hop); processing = false;
    trajectory.trace(trace);
    if (!trace.valid || allocations || !trajectory.output.valid) return 4;
    if (std::fwrite(trace.data, 1, trace.size, stdout) != trace.size || std::fputs("END\n", stdout) < 0) return 5;
  }
  return trajectory.sequence ? 0 : 6;
}
