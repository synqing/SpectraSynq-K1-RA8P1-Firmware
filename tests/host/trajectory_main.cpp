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
int main(int argc, char** argv) {
  if(argc==2 && std::strcmp(argv[1],"--schema")==0) trace.format=fixture::Trace::Format::schema;
  else if(argc==2 && std::strcmp(argv[1],"--binary")==0) trace.format=fixture::Trace::Format::binary;
  else if(argc!=1) return 7;
  std::int16_t hop[180];
  for (;;) {
    const auto count = std::fread(hop, sizeof(std::int16_t), 180, stdin);
    if (count == 0 && std::feof(stdin)) break;
    if (count != 180) return 3;
    processing = true; trajectory.process(hop); processing = false;
    trajectory.trace(trace);
    if (!trace.valid || allocations || !trajectory.output.valid) return 4;
    if(trace.format==fixture::Trace::Format::binary) {
      const std::uint32_t size=static_cast<std::uint32_t>(trace.size);
      if(std::fwrite(&size,4,1,stdout)!=1) return 5; // HOST little-endian fixture ABI.
      if(std::fwrite(trace.data,1,trace.size,stdout)!=trace.size) return 5;
    } else {
      if (std::fwrite(trace.data, 1, trace.size, stdout) != trace.size || std::fputs("END\n", stdout) < 0) return 5;
      if(trace.format==fixture::Trace::Format::schema) return 0;
    }
  }
  return trajectory.sequence ? 0 : 6;
}
