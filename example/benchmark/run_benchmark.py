#!/usr/bin/env python3
#
# Copyright (C) 2026 Open Source Robotics Foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Benchmark Comparison

Compare bench under zenoh and zeromq configurations.
Works on Linux, macOS, and Windows.

Prerequisites:
    Build the bench binary from the example/benchmark/ directory:
        cd example/benchmark
        mkdir build && cd build
        cmake ..
        cmake --build .

Usage (run from anywhere):
    python3 <path-to>/run_benchmark.py -b <path-to>/bench [OPTIONS]

Example from the example/benchmark/build directory:
    python3 ../run_benchmark.py -b ./bench -s -i 100 --plot

Example from the source root:
    python3 example/benchmark/run_benchmark.py -b example/benchmark/build/bench -s --plot

Options:
    -b BENCH_PATH   Path to the bench binary (required unless auto-detected)
    -o OUTPUT_DIR   Output directory (default: ./benchmark_results)
    -i ITERATIONS   Iterations per message size (default: 1000, or 50 with --quick)
    -w WARMUP       Warmup iterations per message size (default: 10, or 3 with --quick)
    -s              Use extended range of message sizes
    -t              Throughput only (skip latency)
    -l              Latency only (skip throughput)
    --svc           Also run service call benchmarks (two-way, one-way, no-input)
    --no-pubsub     Skip pub/sub benchmarks (implies --svc)
    --quick         Quick preset: iterations=50, warmup=3, --svc, --plot
                    (individual -i/-w/-l flags still override)
    --full          Full preset: implies --svc, -s, and --plot; composes
                    with --quick
    --plot          Generate PNG charts after benchmarks (requires matplotlib)
    -h              Show help

Output notes:
    Progress lines are written to stderr; the final summary goes to stdout.
    Redirect stderr to suppress the live spinner: 2>/dev/null
"""

import argparse
import itertools
import os
import re
import signal
import subprocess
import sys
import threading
import time


# Each configuration is a dict with:
#   name: identifier used in output filenames
#   env: dict of env vars to SET (keys not present are removed from env)
#   clear: list of env var names to REMOVE
CONFIGS = [
    {
        'name': 'zenoh',
        'description': 'Zenoh (default settings)',
        'env': {
            'GZ_TRANSPORT_IMPLEMENTATION': 'zenoh',
        },
        'clear': [],
    },
    {
        'name': 'zeromq',
        'description': 'ZeroMQ (default HWM=1000)',
        'env': {
            'GZ_TRANSPORT_IMPLEMENTATION': 'zeromq',
        },
        'clear': [],
    },
]


# ── Helpers ────────────────────────────────────────────────────────────────────

# Fixed label column width keeps the status suffix at a constant offset so
# the spinner / "done" text aligns across all runs.  Must be >= the length of
# the longest possible label produced by _make_label().
_LABEL_WIDTH = 28


def _fmt_time(secs):
    """Format seconds as HH:MM:SS (hours omitted when zero)."""
    secs = max(0, int(secs))
    m, s = divmod(secs, 60)
    h, m = divmod(m, 60)
    if h:
        return f'{h:d}:{m:02d}:{s:02d}'
    return f'{m:02d}:{s:02d}'


def _make_label(file_prefix, mode_flag, config_name):
    """Return a compact, human-readable benchmark label.

    Examples
    --------
    throughput/zenoh     (pub/sub)
    svc-twoway-tput/zenoh
    svc-noinput-lat/zeromq
    """
    t = 'tput' if mode_flag == '-t' else 'lat'
    if file_prefix == '':
        mode_str = 'throughput' if mode_flag == '-t' else 'latency'
    elif file_prefix == 'svc_twoway_':
        mode_str = f'svc-twoway-{t}'
    elif file_prefix == 'svc_oneway_':
        mode_str = f'svc-oneway-{t}'
    elif file_prefix == 'svc_noinput_':
        mode_str = f'svc-noinput-{t}'
    else:
        mode_str = f'{file_prefix.rstrip("_")}-{t}'
    return f'{mode_str}/{config_name}'



# Matches bench's native per-size progress line: "[n/N] ..."
_BENCH_PROGRESS_RE = re.compile(r'^\[(\d+)/(\d+)\]')


def _relay_bench_stderr(proc, progress) -> None:
    """Read bench's stderr and update the per-size progress bar.

    Recognises two formats:

    * ``[n/N] ...`` — bench's native per-size output line.
    * ``PROGRESS n/N`` — explicit progress token added to bench for runners
      that need a machine-readable format.

    All other lines are silently discarded.  Forwarding unknown bench output
    to our stderr would corrupt the ``\\r``-based spinner display; failures
    are reported via the non-zero return-code check in ``run_config``.
    """
    for raw in proc.stderr:
        line = raw.decode(errors='replace').rstrip()
        m = _BENCH_PROGRESS_RE.match(line)
        if m:
            progress.update(int(m.group(1)), int(m.group(2)))
            continue
        if line.startswith('PROGRESS '):
            try:
                n, total = line[9:].split('/')
                progress.update(int(n), int(total))
            except (ValueError, IndexError):
                pass  # Malformed PROGRESS line; skip silently.


class _RunProgress:
    """Live single-line progress display for one benchmark run.

    The spinner line shows a per-size progress bar driven by ``PROGRESS``
    lines emitted by the bench binary (one per completed message size).
    Call ``done()`` to stop the thread and emit the final newline-terminated
    line, which includes the suite elapsed time and a global suite bar.

    Progress output goes to stderr so it can be suppressed independently of
    the summary (stdout).
    """

    def __init__(self, idx, total, label, suite_t0):
        """Initialise and immediately start the background spinner thread.

        Args:
            idx: 1-based index of this run in the overall plan.
            total: Total number of runs in the plan.
            label: Compact benchmark label (e.g. 'throughput/zenoh').
            suite_t0: ``time.monotonic()`` timestamp when the suite started.
        """
        self._idx = idx
        self._total = total
        self._label = label
        self._suite_t0 = suite_t0
        self._t0 = time.monotonic()
        self._stop = threading.Event()
        self._spin = itertools.cycle('|/-\\')
        self._width = len(str(total))       # digits needed for zero-padding
        self._size_done = 0
        self._size_total = 0
        self._size_lock = threading.Lock()
        self._thread = threading.Thread(target=self._spin_loop, daemon=True)
        self._thread.start()

    # ── private ──────────────────────────────────────────────────────────────

    def _prefix(self):
        """Return the fixed-width prefix shared by spinner and done lines.

        Format: ``  [<idx>/<total>] <label padded to _LABEL_WIDTH>``.
        Because every part has a fixed width, ``done`` always appears at the
        same terminal column regardless of label length.
        """
        return (f'  [{self._idx:>{self._width}}/{self._total}] '
                f'{self._label:<{_LABEL_WIDTH}}')

    def _size_bar(self) -> str:
        """Return a compact per-size bar, e.g. ``[████░░░░░░░░░░]``.

        Returns an empty string before the first ``PROGRESS`` line arrives
        so the spinner starts cleanly without a placeholder bar.
        """
        with self._size_lock:
            done, total = self._size_done, self._size_total
        if total == 0:
            return ''
        width = 14
        filled = round(done / total * width)
        return f' [{"█" * filled}{"░" * (width - filled)}]'

    def _spin_loop(self):
        """Background thread: rewrite the current line with a spinner tick.

        Uses ``Event.wait(timeout)`` instead of ``sleep`` so the thread wakes
        immediately when ``done()`` sets the stop event.
        """
        while not self._stop.wait(0.2):
            elapsed = time.monotonic() - self._t0
            bar = self._size_bar()
            sys.stderr.write(
                f'\r{self._prefix()}'
                f'  {next(self._spin)}{bar}  {elapsed:6.1f}s   ')
            sys.stderr.flush()

    # ── public ───────────────────────────────────────────────────────────────

    def update(self, done: int, total: int) -> None:
        """Update the per-size progress bar.

        Called from the stderr-reader thread each time bench emits a
        ``PROGRESS n/N`` line.
        """
        with self._size_lock:
            self._size_done = done
            self._size_total = total

    def done(self, ok=True):
        """Stop the spinner and print the final, newline-terminated status line.

        Args:
            ok: False if the run was interrupted or returned an error.
        """
        self._stop.set()
        self._thread.join()
        elapsed = time.monotonic() - self._t0

        suite_elapsed = time.monotonic() - self._suite_t0

        tag = 'done' if ok else 'FAILED'
        # Use a fixed-width elapsed field (:6.1f) so the bracketed suite
        # timestamp always appears at the same terminal column across all runs,
        # regardless of how fast or slow the run was.
        sys.stderr.write(
            f'\r{self._prefix()}'
            f'  {tag} {elapsed:6.1f}s [{_fmt_time(suite_elapsed)}]\n')
        sys.stderr.flush()


def _build_run_list(configs, run_pubsub, run_svc, run_throughput, run_latency):
    """Return an ordered list of all benchmark runs as dicts.

    Each dict has the keys expected by ``run_config``:
        config      – entry from CONFIGS
        mode        – '-t' (throughput) or '-l' (latency)
        svc_flag    – bench flag for service mode, or None for pub/sub
        file_prefix – prefix prepended to the output filename

    One-way service calls only produce meaningful throughput numbers (there is
    no reply to time), so they are always run with mode='-t' regardless of the
    ``run_throughput``/``run_latency`` settings.
    """
    runs = []
    for config in configs:
        if run_pubsub:
            if run_throughput:
                runs.append(dict(config=config, mode='-t',
                                 svc_flag=None, file_prefix=''))
            if run_latency:
                runs.append(dict(config=config, mode='-l',
                                 svc_flag=None, file_prefix=''))
        if run_svc:
            if run_throughput:
                runs.append(dict(config=config, mode='-t',
                                 svc_flag='-c', file_prefix='svc_twoway_'))
            if run_latency:
                runs.append(dict(config=config, mode='-l',
                                 svc_flag='-c', file_prefix='svc_twoway_'))
            # One-way: always throughput — no reply means no RTT to measure.
            runs.append(dict(config=config, mode='-t',
                             svc_flag='--oneway', file_prefix='svc_oneway_'))
            if run_throughput:
                runs.append(dict(config=config, mode='-t',
                                 svc_flag='--noinput',
                                 file_prefix='svc_noinput_'))
            if run_latency:
                runs.append(dict(config=config, mode='-l',
                                 svc_flag='--noinput',
                                 file_prefix='svc_noinput_'))
    return runs


# ── Core functions ─────────────────────────────────────────────────────────────

def find_bench(bench_arg):
    """Resolve the bench binary path."""
    if bench_arg:
        path = os.path.abspath(bench_arg)
        if os.path.isfile(path):
            return path
        # On Windows, try adding .exe
        if sys.platform == 'win32' and os.path.isfile(path + '.exe'):
            return path + '.exe'
        print(f"Error: bench binary not found at '{bench_arg}'")
        sys.exit(1)

    # Auto-detect: look in common build locations relative to this script
    # (example/run_benchmark.py) and the current working directory.
    script_dir = os.path.dirname(os.path.abspath(__file__))
    cwd = os.getcwd()
    candidates = [
        # From example/build/: ../run_benchmark.py → ./bench
        os.path.join(cwd, 'bench'),
        # From example/: ./run_benchmark.py → build/bench
        os.path.join(script_dir, 'build', 'bench'),
        # From source root: example/run_benchmark.py → example/build/bench
        os.path.join(script_dir, '..', 'example', 'build', 'bench'),
    ]
    if sys.platform == 'win32':
        candidates = ([c + '.exe' for c in candidates] + candidates)

    for candidate in candidates:
        if os.path.isfile(candidate):
            return os.path.abspath(candidate)

    print("Error: bench binary not found.")
    print()
    print("Build it first:")
    print("  cd example && mkdir build && cd build && cmake .. && cmake --build .")
    print()
    print("Then either run from example/build/:")
    print("  python3 ../run_benchmark.py")
    print()
    print("Or specify the path explicitly:")
    print("  python3 run_benchmark.py -b example/build/bench")
    sys.exit(1)


def build_env(config):
    """Return a copy of the current environment modified for *config*.

    Variables listed in ``config['clear']`` are removed first (so that any
    user-set values from the calling shell do not bleed in), then the vars in
    ``config['env']`` are set.  This order ensures deterministic behaviour
    independent of the caller's environment.
    """
    env = os.environ.copy()
    for key in config['clear']:
        env.pop(key, None)
    env.update(config['env'])
    return env


def terminate_process(proc):
    """Gracefully stop *proc*, escalating to SIGKILL if necessary.

    On POSIX, sends SIGINT first (bench handles it gracefully) and waits up
    to 5 s before escalating to SIGTERM/SIGKILL.  On Windows, calls
    ``terminate()`` directly.  All exceptions are swallowed — the caller
    should not fail just because cleanup was imperfect.
    """
    if proc.poll() is not None:
        return  # already exited
    try:
        if sys.platform == 'win32':
            proc.terminate()
        else:
            proc.send_signal(signal.SIGINT)
            try:
                proc.wait(timeout=5)
                return
            except subprocess.TimeoutExpired:
                proc.terminate()
        proc.wait(timeout=5)
    except (ProcessLookupError, subprocess.TimeoutExpired):
        pass
    finally:
        # Last-resort SIGKILL if the process is still alive.
        if proc.poll() is None:
            try:
                proc.kill()
                proc.wait(timeout=2)
            except (ProcessLookupError, subprocess.TimeoutExpired):
                pass


def run_config(bench, run, output_dir, iterations, extended_sizes, warmup,
               run_idx=1, total=1, suite_t0=None):
    """Run a single benchmark configuration and write results to a .dat file.

    Launches two processes: a background reply/server node and a foreground
    publisher/client node that writes results.  Both have their output
    suppressed so they do not interfere with the Python progress line.

    Args:
        bench: Absolute path to the bench binary.
        run: Dict from ``_build_run_list`` with keys 'config', 'mode',
             'svc_flag', and 'file_prefix'.
        output_dir: Directory for output .dat files.
        iterations: Number of measurement iterations per message size.
        extended_sizes: True to pass -s (extended sizes) to bench.
        warmup: Number of warmup iterations per message size.
        run_idx: 1-based index of this run in the overall plan.
        total: Total number of runs in the plan.
        suite_t0: ``time.monotonic()`` timestamp for the whole suite.
    Raises:
        KeyboardInterrupt: Re-raised after cleanup so the caller can exit.
    """
    if suite_t0 is None:
        suite_t0 = time.monotonic()

    config = run['config']
    mode_flag = run['mode']
    svc_flag = run['svc_flag']
    file_prefix = run['file_prefix']

    mode_name = 'throughput' if mode_flag == '-t' else 'latency'
    outfile = os.path.join(
        output_dir, f'{file_prefix}{mode_name}_{config["name"]}.dat')
    label = _make_label(file_prefix, mode_flag, config['name'])

    env = build_env(config)
    extra_args = ['-s'] if extended_sizes else []
    svc_args = [svc_flag] if svc_flag else []

    # Start reply/server node in background; suppress its output so it does
    # not interfere with the Python-driven progress line.
    reply_cmd = [bench, '-r'] + svc_args + extra_args
    reply_proc = subprocess.Popen(reply_cmd, env=env,
                                  stdout=subprocess.DEVNULL,
                                  stderr=subprocess.DEVNULL)

    progress = _RunProgress(run_idx, total, label, suite_t0)
    pub_proc = None
    ok = True
    try:
        # Give discovery time to propagate before starting measurements.
        time.sleep(2)

        pub_cmd = ([bench, '-p', mode_flag] + svc_args +
                   ['-i', str(iterations), '-w', str(warmup), '-o', outfile] +
                   extra_args)
        # Capture stderr to parse PROGRESS lines; stdout goes to the .dat file
        # via -o so it is not needed here.
        pub_proc = subprocess.Popen(pub_cmd, env=env,
                                    stdout=subprocess.DEVNULL,
                                    stderr=subprocess.PIPE)
        reader = threading.Thread(
            target=_relay_bench_stderr, args=(pub_proc, progress), daemon=True)
        reader.start()
        returncode = pub_proc.wait()
        reader.join()
        if returncode != 0:
            ok = False
            sys.stderr.write(
                f'\n  WARNING: bench exited with code {returncode}'
                f' for {label}\n')
    except KeyboardInterrupt:
        ok = False
        raise
    finally:
        if pub_proc is not None:
            terminate_process(pub_proc)
        terminate_process(reply_proc)
        progress.done(ok)

    # Cool down between runs to let OS resources (sockets) be released.
    time.sleep(2)


def main():
    parser = argparse.ArgumentParser(
        description='Run benchmark comparison across transport configs.')
    parser.add_argument('-b', '--bench', default=None,
                        help='Path to the bench binary (default: auto-detect)')
    parser.add_argument('-o', '--output', default='./benchmark_results',
                        help='Output directory (default: ./benchmark_results)')
    parser.add_argument('-i', '--iterations', type=int, default=None,
                        help='Iterations per message size (default: 1000, '
                             'or 50 with --quick)')
    parser.add_argument('-s', '--extended-sizes', action='store_true',
                        help='Use extended range of message sizes')
    parser.add_argument('-t', '--throughput-only', action='store_true',
                        help='Throughput only (skip latency)')
    parser.add_argument('-l', '--latency-only', action='store_true',
                        help='Latency only (skip throughput)')
    parser.add_argument('-w', '--warmup', type=int, default=None,
                        help='Warmup iterations per message size '
                             '(default: 10, or 3 with --quick)')
    parser.add_argument('--svc', action='store_true',
                        help='Also run service call benchmarks '
                             '(two-way, one-way, no-input)')
    parser.add_argument('--no-pubsub', action='store_true',
                        help='Skip pub/sub benchmarks '
                             '(run only service calls, implies --svc)')
    parser.add_argument('--quick', action='store_true',
                        help='Quick preset: 50 iterations, 3 warmup, '
                             '--svc, --plot '
                             '(overridden by explicit -i/-w/-l)')
    parser.add_argument('--full', action='store_true',
                        help='Full preset: enable --svc, -s (extended sizes), '
                             'and --plot; composes with --quick')
    parser.add_argument('--plot', action='store_true',
                        help='Generate PNG charts after benchmarks '
                             '(requires matplotlib)')
    args = parser.parse_args()

    # Apply presets before falling back to hard defaults.
    # --full: enable svc + extended sizes + plots.
    if args.full:
        args.svc = True
        args.extended_sizes = True
        args.plot = True
    # --quick: conservative iteration/warmup counts with all test types enabled.
    # Only override flags not set explicitly by the user.
    if args.quick:
        if args.iterations is None:
            args.iterations = 50
        if args.warmup is None:
            args.warmup = 3
        args.svc = True
        args.plot = True
    # Hard defaults (used when neither --quick nor explicit flags override).
    if args.iterations is None:
        args.iterations = 1000
    if args.warmup is None:
        args.warmup = 10

    # Check for matplotlib early so we fail before running any benchmarks.
    if args.plot:
        try:
            import matplotlib  # noqa: F401
        except ImportError:
            print('Error: matplotlib is required for --plot / --full.')
            print('Install it with:')
            print('  pip install matplotlib')
            sys.exit(1)

    bench = find_bench(args.bench)
    output_dir = os.path.abspath(args.output)
    os.makedirs(output_dir, exist_ok=True)

    run_throughput = not args.latency_only
    run_latency = not args.throughput_only
    run_pubsub = not args.no_pubsub
    run_svc = args.svc or args.no_pubsub

    # Build the complete run list upfront so we know the total and can show
    # an accurate plan and ETA.
    run_list = _build_run_list(CONFIGS, run_pubsub, run_svc,
                               run_throughput, run_latency)
    total = len(run_list)

    # ── Header ────────────────────────────────────────────────────────────────
    W = 56
    tests = ('throughput' if run_throughput else '') + \
            (' + ' if run_throughput and run_latency else '') + \
            ('latency' if run_latency else '')
    sizes_label = 'extended' if args.extended_sizes else 'default'

    # Build a one-line plan summary: "12 pub/sub  ·  30 svc"
    n_pubsub = sum(1 for r in run_list if r['svc_flag'] is None)
    n_svc = total - n_pubsub
    plan_parts = []
    if n_pubsub:
        plan_parts.append(f'{n_pubsub} pub/sub')
    if n_svc:
        plan_parts.append(f'{n_svc} svc')
    plan_str = '  ·  '.join(plan_parts)

    preset_flags = ' '.join(
        flag for flag, active in (('--quick', args.quick), ('--full', args.full))
        if active
    )

    print('═' * W)
    print(f'  gz-transport Benchmark  ·  {total} runs  ·  {tests}')
    print(f'  bench:  {bench}')
    print(f'  output: {output_dir}')
    if preset_flags:
        print(f'  preset: {preset_flags}')
    print(f'  iter:   {args.iterations}  warmup: {args.warmup}'
          f'  sizes: {sizes_label}')
    print(f'  plan:   {plan_str}')
    print('═' * W)

    # ── Run loop ──────────────────────────────────────────────────────────────
    suite_t0 = time.monotonic()

    for i, run in enumerate(run_list, 1):
        run_config(
            bench, run, output_dir,
            args.iterations, args.extended_sizes, args.warmup,
            run_idx=i,
            total=total,
            suite_t0=suite_t0,
        )

    # ── Summary ───────────────────────────────────────────────────────────────
    suite_elapsed = time.monotonic() - suite_t0
    print()
    print('═' * W)
    print(f'  Done  ·  {total} runs  ·  {_fmt_time(suite_elapsed)} total')
    print(f'  Results in: {output_dir}/')
    for f in sorted(os.listdir(output_dir)):
        if f.endswith('.dat'):
            print(f'    {f}')
    print('═' * W)

    # ── Auto-plot ─────────────────────────────────────────────────────────────
    if args.plot:
        script_dir = os.path.dirname(os.path.abspath(__file__))
        plot_script = os.path.join(script_dir, 'plot_benchmark.py')
        print()
        print('Generating plots...')
        subprocess.run([sys.executable, plot_script, output_dir], check=True)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print('\nInterrupted.')
        sys.exit(130)
