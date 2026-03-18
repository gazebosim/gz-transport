# gz-transport Benchmark Suite

Throughput and latency benchmarks for gz-transport, comparing different backends
and configurations across a range of message sizes.

## Prerequisites

- **gz-transport** built with Zenoh support (`HAVE_ZENOH`)
- **GFlags** (`libgflags-dev` on Ubuntu)
- **Python 3.8+**
- **matplotlib** (for plots): `pip install matplotlib`

## Build

```bash
cd example/benchmark
mkdir build && cd build
cmake ..
make
```

The output binary is `example/benchmark/build/bench`.

## Quick Start

Run a fast comparison (50 iterations, all test types, generates plots):

```bash
python3 run_benchmark.py -b build/bench --quick
```

Results are written to `./benchmark_results/`.

## Full Benchmark

Run the complete suite (1000 iterations, extended sizes, service benchmarks, plots):

```bash
python3 run_benchmark.py -b build/bench --full
```

This takes roughly 30–60 minutes depending on hardware.

## Options

| Option | Description |
|--------|-------------|
| `-b BENCH_PATH` | Path to the `bench` binary (required unless auto-detected) |
| `-o OUTPUT_DIR` | Output directory (default: `./benchmark_results`) |
| `-i ITERATIONS` | Iterations per message size (default: 1000) |
| `-w WARMUP` | Warmup iterations per message size (default: 10) |
| `-s` | Extended range of message sizes |
| `-t` | Throughput only (skip latency) |
| `-l` | Latency only (skip throughput) |
| `--svc` | Also run service call benchmarks (two-way, one-way, no-input) |
| `--no-pubsub` | Skip pub/sub benchmarks |
| `--quick` | Quick preset: 50 iterations, 3 warmup, `--svc`, `--plot` |
| `--full` | Full preset: implies `--svc`, `-s`, and `--plot` |
| `--plot` | Generate PNG charts after benchmarks |

## Configurations

Two configurations are compared in each run:

| Name | Description |
|------|-------------|
| `zenoh` | Zenoh (default settings) |
| `zeromq` | ZeroMQ with default high-water mark |

## Output Files

All output is written to the results directory (`./benchmark_results/` by default).

### Data files (`.dat`)

Tab-separated data, one row per message size:

```
size_bytes  metric_value  [loss_pct]
```

### Plot files (`.png`, generated with `--plot`)

| File | Description |
|------|-------------|
| `throughput_pubsub.png` | Pub/sub throughput (MB/s) and loss % vs message size |
| `latency_pubsub.png` | Pub/sub round-trip latency (µs, log scale) vs message size |
| `svc_twoway_throughput.png` | Two-way service throughput |
| `svc_twoway_latency.png` | Two-way service latency (log scale) |
| `svc_oneway_throughput.png` | One-way service throughput |
| `svc_noinput_throughput.png` | No-input service throughput |
| `svc_noinput_latency.png` | No-input service latency (log scale) |
| `summary.png` | Bar chart summary at representative sizes (1 MB and 1 KB) |

## Generating Plots from Existing Data

If you already have `.dat` files, regenerate the plots without re-running benchmarks:

```bash
python3 plot_benchmark.py benchmark_results/
```

## Running bench Directly

The `bench` binary can be used standalone for quick single-configuration tests:

```bash
# Throughput test (pub/sub, all default sizes)
GZ_TRANSPORT_IMPLEMENTATION=zenoh ./bench -t

# Latency test
GZ_TRANSPORT_IMPLEMENTATION=zenoh ./bench -l

# Extended sizes, 200 iterations, 5 warmup
GZ_TRANSPORT_IMPLEMENTATION=zenoh ./bench -t -s -i 200 -w 5

# Service benchmarks
GZ_TRANSPORT_IMPLEMENTATION=zenoh ./bench --svc

# ZeroMQ comparison
GZ_TRANSPORT_IMPLEMENTATION=zeromq ./bench -t
```

Run `./bench --help` for the full list of flags.
