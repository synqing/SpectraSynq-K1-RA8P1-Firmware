#!/usr/bin/env python3
"""Run the production catch-up loop with controlled hop, deadline and TX seams.

This checks scheduling decisions, not peripheral timing or sustainable CPU load.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def production_loop(source):
    start = source.index('for (unsigned catchup = 0; catchup < 3U; ++catchup) {')
    stop = source.index('    live_runtime.setCaptureMeta', start)
    # The loop straddles conditional compilation for the legacy one-hop path.
    return '\n'.join(line for line in source[start:stop].splitlines()
                     if not line.lstrip().startswith('#'))


PREFIX = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
namespace k1 { namespace titan {
enum class LiveHopStatus { Ok, NotReady, Discontinuity, Fault };
}}
using Status = k1::titan::LiveHopStatus;
static unsigned calls, polls, queued, resets, stop_tx_on, due_on;
static bool transmitting;
static Status status;
static bool pair_any_transmitting() { return transmitting; }
static void palette_pair_poll() { ++polls; }
static struct Runtime {
  Status serviceOneHop() {
    ++calls;
    if (status == Status::Ok) { assert(queued); --queued; }
    if (calls == stop_tx_on) transmitting = false;
    return status;
  }
  std::uint64_t nowUs() { return calls; }
} live_runtime;
static struct Palettes {
  void resetAudioState() { ++resets; }
  bool frameDue(std::uint64_t now) { return now >= due_on; }
} palettes;
static void run() {
'''

CASES = r'''
}
static void setup(bool tx, unsigned stop, unsigned due=99, Status result=Status::Ok) {
  calls=polls=resets=0; queued=9; transmitting=tx;
  stop_tx_on=stop; due_on=due; status=result;
}
static void check(unsigned expected, unsigned consumed, unsigned reset=0) {
  assert(calls==expected); assert(polls==expected);
  assert(queued==9-consumed); assert(resets==reset);
}
int main() {
  setup(true,1); run(); check(1,1); // First hop opens idle; queued PCM retained.
  setup(true,2); run(); check(2,2); // Transition on a later hop also yields.
  setup(true,99); run(); check(3,3); // Unchanged TX keeps bounded catch-up.
  setup(false,99); run(); check(3,3); // Initially idle is not a new transition.
  setup(true,99,1); run(); check(1,1); // A due frame still yields immediately.
  setup(false,99,0); run(); check(1,1); // Even already due, service first hop.
  setup(true,99,99,Status::NotReady); run(); check(1,0);
  setup(true,99,99,Status::Discontinuity); run(); check(1,0,1);
  setup(true,99,99,Status::Fault); run(); check(1,0,1);
  std::puts("nine production-loop scheduling cases passed");
}
'''


def main():
    source = (ROOT/'platform/ra8p1/fixture_app.cpp').read_text()
    loop = production_loop(source)
    transition = 'if (transmitting_before_hop && !pair_any_transmitting()) break;'
    mutations = {
        'actual': loop,
        'lost_transition': loop.replace(transition, '(void)transmitting_before_hop;'),
        'break_initial_idle': loop.replace(transition,
            '(void)transmitting_before_hop; if (!pair_any_transmitting()) break;'),
        'lost_deadline': loop.replace(
            'if (palettes.frameDue(live_runtime.nowUs())) break;', ''),
    }
    assert transition in loop
    with tempfile.TemporaryDirectory(prefix='titan-idle-yield-') as tmp:
        for name, body in mutations.items():
            path = Path(tmp)/f'{name}.cpp'
            binary = Path(tmp)/name
            path.write_text(PREFIX+body+CASES)
            subprocess.run(['c++', '-std=c++17', '-O1', '-Wall', '-Wextra',
                            '-Werror', str(path), '-o', str(binary)], check=True)
            result = subprocess.run([str(binary)], capture_output=True, text=True)
            if name == 'actual':
                assert result.returncode == 0, result.stderr
                print(result.stdout.strip())
            else:
                assert result.returncode != 0 and 'Assertion failed' in result.stderr, name
                print(f'{name}: mutation rejected')


if __name__ == '__main__':
    main()
