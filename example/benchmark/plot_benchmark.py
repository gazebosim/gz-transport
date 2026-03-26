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

"""Plot benchmark charts as PNG files.

Reads .dat files produced by run_benchmark.py and writes one PNG per
chart into the output directory.  Uses the 'Agg' non-interactive backend so
it runs headless in CI environments (Jenkins, GitHub Actions) without a
display server.

Usage:
    python3 plot_benchmark.py RESULTS_DIR [--output OUTPUT_DIR]

Output files (only written when the corresponding .dat files exist):
    throughput_pubsub.png
    latency_pubsub.png
    summary.png
    svc_twoway_throughput.png / svc_twoway_latency.png
    svc_oneway_throughput.png
    svc_noinput_throughput.png / svc_noinput_latency.png

Requires:
    pip install matplotlib
"""

import argparse
import os
import sys
from collections import OrderedDict

try:
    import matplotlib
    matplotlib.use('Agg')  # non-interactive backend; must precede pyplot import
    import matplotlib.pyplot as plt
    import matplotlib.ticker
    import numpy as np
except ImportError as exc:
    print(f'Error: {exc}.')
    print('Install with:')
    print('  pip install matplotlib')
    sys.exit(1)


# ── Constants ──────────────────────────────────────────────────────────────────

# Fixed tick positions and labels for the log-scale message-size X axis.
_SIZE_TICKS = [
    256, 512, 1_024, 4_096, 8_192, 16_384, 32_768, 65_536,
    131_072, 262_144, 524_288, 1_048_576, 2_097_152, 4_194_304,
]
_SIZE_LABELS = [
    '256 B', '512 B', '1 KB', '4 KB', '8 KB', '16 KB', '32 KB', '64 KB',
    '128 KB', '256 KB', '512 KB', '1 MB', '2 MB', '4 MB',
]

# Per-configuration display style.  Order here determines legend order.
# Wong colorblind-safe palette — distinguishable by hue, even in greyscale.
STYLES: 'OrderedDict[str, dict]' = OrderedDict([
    ('zenoh',
     {'label': 'Zenoh',   'color': '#009E73'}),
    ('zeromq',
     {'label': 'ZeroMQ',  'color': '#0072B2'}),
])


# ── Global style ───────────────────────────────────────────────────────────────

plt.rcParams.update({
    'figure.facecolor':  'white',
    'axes.facecolor':    'white',
    'axes.edgecolor':    '#cccccc',
    'axes.linewidth':    0.8,
    'axes.grid':         True,
    'grid.color':        '#ebebeb',
    'grid.linewidth':    0.8,
    'axes.spines.top':   False,
    'axes.spines.right': False,
    'font.family':       'sans-serif',
    'font.size':         10,
    'legend.frameon':    False,
    'legend.fontsize':   9,
})


# ── Axis helpers ───────────────────────────────────────────────────────────────

def _configure_log_xaxis(ax: plt.Axes) -> None:
    """Set log scale, human-readable tick labels, and xlabel on *ax*."""
    ax.set_xscale('log')
    ax.set_xticks(_SIZE_TICKS)
    ax.set_xticklabels(_SIZE_LABELS, rotation=30, ha='right', fontsize=8)
    ax.set_xlabel('Message size', labelpad=6)
    # Remove the minor ticks that log scale adds by default to reduce visual
    # clutter on the dense custom tick grid.
    ax.xaxis.set_minor_locator(matplotlib.ticker.NullLocator())


def _right_legend(ax: plt.Axes) -> None:
    """Attach the legend to the right of *ax*, outside the plot area.

    ``bbox_inches='tight'`` in ``_save()`` ensures the legend is included
    in the saved PNG without clipping.
    """
    ax.legend(
        bbox_to_anchor=(1.01, 1), loc='upper left',
        borderaxespad=0, handlelength=1.8,
    )


# ── Data loading ───────────────────────────────────────────────────────────────

def read_dat(filepath: str) -> tuple:
    """Parse a bench ``.dat`` file.

    Returns ``(sizes, columns)`` where *sizes* is a list of ints and *columns*
    is a list of lists — one inner list per value column after the size column.
    Comment lines (starting with ``#``) and malformed lines are silently
    skipped.
    """
    sizes: list = []
    columns: list = []
    try:
        with open(filepath, encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                parts = line.split()
                if len(parts) < 3:
                    continue
                try:
                    size = int(parts[1])
                    values = [float(x) for x in parts[2:]]
                except (ValueError, IndexError):
                    continue
                sizes.append(size)
                if not columns:
                    columns = [[] for _ in values]
                for i, v in enumerate(values):
                    columns[i].append(v)
    except OSError:
        pass
    return sizes, columns


def _load_pubsub_throughput(results_dir: str) -> dict:
    """Load pub/sub throughput files; returns config → (sizes, MB/s, loss%)."""
    data = {}
    for cfg in STYLES:
        path = os.path.join(results_dir, f'throughput_{cfg}.dat')
        sizes, cols = read_dat(path)
        if not sizes or not cols:
            continue
        loss = cols[2] if len(cols) > 2 else [0.0] * len(sizes)
        data[cfg] = (sizes, cols[0], loss)
    return data


def _load_pubsub_latency(results_dir: str) -> dict:
    """Load pub/sub latency files; returns config → (sizes, avg, min, max) µs."""
    data = {}
    for cfg in STYLES:
        path = os.path.join(results_dir, f'latency_{cfg}.dat')
        sizes, cols = read_dat(path)
        if not sizes or len(cols) < 3:
            continue
        data[cfg] = (sizes, cols[0], cols[1], cols[2])
    return data


def _load_svc_throughput(results_dir: str, prefix: str) -> dict:
    """Load service throughput files; returns config → (sizes, Kcalls/s, loss%)."""
    data = {}
    for cfg in STYLES:
        path = os.path.join(results_dir, f'{prefix}throughput_{cfg}.dat')
        sizes, cols = read_dat(path)
        if not sizes or len(cols) < 2:
            continue
        loss = cols[2] if len(cols) > 2 else [0.0] * len(sizes)
        data[cfg] = (sizes, cols[1], loss)   # col 1 = Kcalls/s
    return data


def _load_svc_latency(results_dir: str, prefix: str) -> dict:
    """Load service latency files; returns config → (sizes, avg, min, max) µs."""
    data = {}
    for cfg in STYLES:
        path = os.path.join(results_dir, f'{prefix}latency_{cfg}.dat')
        sizes, cols = read_dat(path)
        if not sizes or len(cols) < 3:
            continue
        data[cfg] = (sizes, cols[0], cols[1], cols[2])
    return data


def _nearest_value(sizes: list, values: list, target: int):
    """Return the element of *values* whose corresponding size is nearest to *target*.

    Returns ``None`` when *sizes* is empty.
    """
    if not sizes:
        return None
    idx = min(range(len(sizes)), key=lambda i: abs(sizes[i] - target))
    return values[idx]


# ── Figure builders ────────────────────────────────────────────────────────────

def make_throughput_figure(all_data: dict,
                          title: str = 'Pub/Sub Throughput') -> plt.Figure:
    """Two-panel figure: throughput (MB/s) on top, loss % on bottom.

    The top panel shows raw throughput for every config on a log-scale X axis.
    Data points with message loss are highlighted with hollow markers on the
    throughput line so they stand out without cluttering the legend.

    The bottom panel shows loss percentage as filled areas — only for configs
    that actually experienced loss.  Zero-loss entries are stored as NaN so
    separate loss zones appear as distinct islands rather than a connected line.
    """
    fig, (ax1, ax2) = plt.subplots(
        2, 1, figsize=(12, 6.5), sharex=True,
        gridspec_kw={'height_ratios': [0.70, 0.30], 'hspace': 0.06},
    )

    any_loss = False
    for cfg, style in STYLES.items():
        if cfg not in all_data:
            continue
        sizes, mb_per_s, loss_pct = all_data[cfg]
        color = style['color']

        ax1.plot(sizes, mb_per_s, color=color, linewidth=2,
                 marker='o', markersize=4, label=style['label'])

        # Hollow markers highlight lossy data points on the throughput line.
        lossy = [(s, m) for s, m, l in zip(sizes, mb_per_s, loss_pct) if l > 0]
        if lossy:
            ls, lm = zip(*lossy)
            ax1.plot(ls, lm, 'o', ms=10, mfc='white', mec=color, mew=1.8)

        # Loss panel — NaN for zero-loss entries creates visual gaps between
        # separate loss zones instead of connecting them with a line.
        if any(l > 0 for l in loss_pct):
            any_loss = True
            loss_y = np.array(
                [l if l > 0 else np.nan for l in loss_pct], dtype=float)
            ax2.fill_between(sizes, 0, loss_y, alpha=0.30, color=color,
                             linewidth=0)
            ax2.plot(sizes, loss_y, color=color, linewidth=1)

    # Configure the shared X axis once on the bottom panel; hide top labels.
    # Setting ticks/scale on ax2 propagates to ax1 via sharex.
    for ax in (ax1, ax2):
        ax.set_xscale('log')
        ax.set_xticks(_SIZE_TICKS)
        ax.xaxis.set_minor_locator(matplotlib.ticker.NullLocator())
    ax2.set_xticklabels(_SIZE_LABELS, rotation=30, ha='right', fontsize=8)
    ax2.set_xlabel('Message size', labelpad=6)
    plt.setp(ax1.get_xticklabels(), visible=False)

    ax1.set_ylabel('Throughput (MB/s) ↑', labelpad=6)
    if any_loss:
        ax2.set_ylabel('Loss (%) ↑ worse', labelpad=6)
        ax2.set_ylim(0, 100)

    fig.align_ylabels([ax1, ax2])
    _right_legend(ax1)
    fig.suptitle(title, fontsize=13, fontweight='bold', y=0.98)
    return fig


def _latency_figure(all_data: dict, figsize: tuple,
                    title: str = '') -> plt.Figure:
    """Shared implementation for latency charts (average line per config)."""
    fig, ax = plt.subplots(figsize=figsize)

    for cfg, style in STYLES.items():
        if cfg not in all_data:
            continue
        sizes, avg_us, _min_us, _max_us = all_data[cfg]
        color = style['color']

        ax.plot(sizes, avg_us, color=color, linewidth=2, marker='o',
                markersize=4, label=style['label'])

    ax.set_yscale('log')
    _configure_log_xaxis(ax)
    ax.set_ylabel('Latency (µs) ↓', labelpad=6)
    _right_legend(ax)
    if title:
        fig.suptitle(title, fontsize=13, fontweight='bold', y=0.98)
    return fig


def make_latency_figure(all_data: dict) -> plt.Figure:
    """Pub/sub latency chart."""
    return _latency_figure(all_data, figsize=(12, 5),
                           title='Pub/Sub Latency')


def make_summary_figure(tput_data: dict, lat_data: dict):
    """Side-by-side bar charts for a quick at-a-glance comparison.

    Left panel: throughput (MB/s) at ~1 MB.
    Right panel: average latency (µs) at ~1 KB — shows small-message cost.
    Values are labelled directly on the bars so the chart can stand alone.

    Returns ``None`` when neither data set has usable entries.
    """
    TARGET_TPUT = 1_000_000   # 1 MB
    TARGET_LAT  = 1_024       # 1 KB

    labels, colors, tput_vals, lat_vals = [], [], [], []
    for cfg, style in STYLES.items():
        td = tput_data.get(cfg)
        ld = lat_data.get(cfg)
        if td is None and ld is None:
            continue
        labels.append(style['label'])
        colors.append(style['color'])
        tput_vals.append(
            _nearest_value(td[0], td[1], TARGET_TPUT) or 0 if td else 0)
        lat_vals.append(
            _nearest_value(ld[0], ld[1], TARGET_LAT) or 0 if ld else 0)

    if not labels:
        return None

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 4))
    x = range(len(labels))

    def _bar(ax, vals, ylabel, title):
        bars = ax.bar(x, vals, color=colors, width=0.6,
                      edgecolor='white', linewidth=0.5)
        ax.set_xticks(list(x))
        ax.set_xticklabels(labels, rotation=30, ha='right', fontsize=8)
        ax.set_ylabel(ylabel)
        ax.set_title(title, fontsize=10, pad=8)
        for bar, val in zip(bars, vals):
            if val:
                ax.text(
                    bar.get_x() + bar.get_width() / 2,
                    bar.get_height() * 1.02, f'{val:.0f}',
                    ha='center', va='bottom', fontsize=8,
                )

    _bar(ax1, tput_vals, 'MB/s', 'Throughput at 1 MB  ↑ higher is better')
    _bar(ax2, lat_vals,  'µs',   'Latency at 1 KB  ↓ lower is better')
    fig.tight_layout(pad=2.0)
    return fig


def make_svc_latency_figure(all_data: dict,
                            title: str = '') -> plt.Figure:
    """Service-call RTT latency chart."""
    return _latency_figure(all_data, figsize=(12, 4.5), title=title)


def make_svc_throughput_figure(all_data: dict,
                              title: str = '') -> plt.Figure:
    """Service-call throughput chart (Kcall/s vs message size)."""
    fig, ax = plt.subplots(figsize=(12, 4))

    for cfg, style in STYLES.items():
        if cfg not in all_data:
            continue
        sizes, kcalls, _ = all_data[cfg]
        ax.plot(sizes, kcalls, color=style['color'], linewidth=2,
                marker='o', markersize=4, label=style['label'])

    _configure_log_xaxis(ax)
    ax.set_ylabel('Throughput (Kcall/s) ↑', labelpad=6)
    _right_legend(ax)
    if title:
        fig.suptitle(title, fontsize=13, fontweight='bold', y=0.98)
    return fig


# ── Save ───────────────────────────────────────────────────────────────────────

def _save(fig: plt.Figure, path: str) -> None:
    """Save *fig* to *path* as a 150 dpi PNG and close it.

    ``bbox_inches='tight'`` expands the bounding box to include the right-side
    legend that sits outside the axes area.
    """
    fig.savefig(path, dpi=150, bbox_inches='tight')
    plt.close(fig)
    print(f'  → {path}')


# ── Entry point ────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description='Generate PNG benchmark charts from bench .dat files.')
    parser.add_argument('results_dir',
                        help='Directory containing .dat files from '
                             'run_benchmark.py')
    parser.add_argument('--output', '-o', default=None,
                        help='Output directory for PNGs '
                             '(default: same as results_dir)')
    args = parser.parse_args()

    if not os.path.isdir(args.results_dir):
        print(f'Error: directory not found: {args.results_dir}')
        sys.exit(1)

    output_dir = args.output or args.results_dir
    os.makedirs(output_dir, exist_ok=True)

    generated = 0

    # ── Pub/sub throughput ────────────────────────────────────────────────
    tput_data = _load_pubsub_throughput(args.results_dir)
    if tput_data:
        print('  Building pub/sub throughput chart...')
        _save(make_throughput_figure(tput_data),
              os.path.join(output_dir, 'throughput_pubsub.png'))
        generated += 1

    # ── Pub/sub latency ───────────────────────────────────────────────────
    lat_data = _load_pubsub_latency(args.results_dir)
    if lat_data:
        print('  Building pub/sub latency chart...')
        _save(make_latency_figure(lat_data),
              os.path.join(output_dir, 'latency_pubsub.png'))
        generated += 1

    # ── Summary ───────────────────────────────────────────────────────────
    if tput_data and lat_data:
        print('  Building performance summary chart...')
        fig = make_summary_figure(tput_data, lat_data)
        if fig:
            _save(fig, os.path.join(output_dir, 'summary.png'))
            generated += 1

    # ── Service-call benchmarks ───────────────────────────────────────────
    SVC_TESTS = [
        # (file_prefix, has_throughput, has_latency, label)
        ('svc_twoway_',  True, True,  'Two-Way Service'),
        ('svc_oneway_',  True, False, 'One-Way Service'),
        ('svc_noinput_', True, True,  'No-Input Service'),
    ]
    for prefix, has_tput, has_lat, label in SVC_TESTS:
        tag = prefix.rstrip('_')
        if has_tput:
            svc_tput = _load_svc_throughput(args.results_dir, prefix)
            if svc_tput:
                print(f'  Building {tag} throughput chart...')
                _save(make_svc_throughput_figure(
                          svc_tput, title=f'{label} Throughput'),
                      os.path.join(output_dir, f'{tag}_throughput.png'))
                generated += 1
        if has_lat:
            svc_lat = _load_svc_latency(args.results_dir, prefix)
            if svc_lat:
                print(f'  Building {tag} latency chart...')
                _save(make_svc_latency_figure(
                          svc_lat, title=f'{label} Latency'),
                      os.path.join(output_dir, f'{tag}_latency.png'))
                generated += 1

    if not generated:
        print(f'Error: no recognisable .dat files found in {args.results_dir}')
        sys.exit(1)


if __name__ == '__main__':
    main()
