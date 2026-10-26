# KEIRO — ケイロ
### Competitive Programming Execution Replay Engine

> *"You wrote the code. Now watch it think."*

KEIRO is an execution replay engine for competitive programmers. Paste a solution, feed it a test case, and instead of staring at wrong output — watch your data structures evolve, step by step, in real time.

It is not a debugger. It is not a tutorial visualizer. It is a window into your own algorithm's execution, rendered in the language competitive programmers actually think in: heaps, graphs, DP tables, not raw memory addresses.

---

## Why KEIRO

Every CP programmer has spent an hour staring at code that *looks* correct. The bug is invisible in the source — but it wouldn't be invisible if you could watch the heap at step 23, or see exactly which node your BFS visited before it went wrong.

Existing visualizers (VisuAlgo, algorithm-visualizer.org) make you work inside *their* implementations. KEIRO works with *yours*. Your variable names. Your graph. Your exact execution path.

---

## Features

- **Step-by-step replay** — scrub forward, backward, jump to any execution step
- **Algorithm-aware rendering** — structures are rendered as structures, not raw values
- **Multi-language** — C++ (STL wrapper header) and Python (dunder override tracing)
- **Code sync** — highlighted source view tracks the active line at every step
- **Variable inspector** — live panel showing changed values at each step, deltas highlighted
- **Complexity analyzer** — cumulative operation count over time, visualized alongside the replay
- **JSON trace format** — open, inspectable, reproducible

---

## Supported Data Structures

| Structure | Visualization |
|---|---|
| `priority_queue` / `heapq` | Binary heap tree, push/pop animated |
| Graph (adjacency list) | Force-directed layout, BFS/DFS frontier highlighted |
| `stack` / `queue` / `deque` | Linear strip, push/pop slides in and out |
| BST / segment tree | Hierarchical node-link, rotations shown |
| Union-Find (DSU) | Forest of trees, union merges animated |
| 2D DP array | Color-filled grid, cell dependencies on hover |
| `unordered_map` / `dict` | Bucket array with chain rendering |

---

## Instrumentation

KEIRO is **non-invasive by design**. You write your solution normally — the only change is a single include or import.

**C++**
```cpp
#include "keiro.h"

// Use keiro:: wrappers instead of std::
keiro::priority_queue<int> pq;
pq.push(5);   // automatically traced
```

**Python**
```python
from keiro import trace

@trace
def dijkstra(graph, src):
    import heapq
    # your code unchanged — keiro wraps heapq at import time
```

Both converge on the same JSON trace format, replayed by a unified frontend.

---

## Trace Format

Each operation emits a structured event:

```json
{
  "step": 4,
  "line": 12,
  "lang": "cpp",
  "ds": "heap",
  "op": "push",
  "val": 5,
  "snapshot": [5, 3, 1],
  "vars": { "dist[1]": 0, "pq.size()": 1 },
  "op_count": 4
}
```

Traces are saved as `.keiro` files — shareable, version-controllable, and replayable without re-running the code.

---

## Tech Stack

| Layer | Technology |
|---|---|
| C++ instrumentation | Header-only STL wrapper (`keiro.h`) |
| Python instrumentation | Decorator + `__setitem__`/`__getitem__` overrides |
| Backend | Python, FastAPI |
| Frontend | React, HTML5 Canvas |
| DS rendering | Custom canvas renderer (no third-party graph libs) |
| Trace format | JSON (`.keiro`) |

---

## Project Structure

```
keiro/
├── instrumentation/
│   ├── cpp/
│   │   └── keiro.h              # C++ STL wrapper header
│   └── python/
│       └── keiro.py             # Python tracing layer
├── backend/
│   ├── main.py                  # FastAPI server
│   ├── compiler.py              # Compile + run + capture trace
│   └── trace_validator.py       # Schema validation for .keiro files
├── frontend/
│   ├── src/
│   │   ├── components/
│   │   │   ├── CodePanel.jsx    # Highlighted source view
│   │   │   ├── DSRenderer/      # Per-structure canvas renderers
│   │   │   │   ├── HeapRenderer.jsx
│   │   │   │   ├── GraphRenderer.jsx
│   │   │   │   ├── DPTableRenderer.jsx
│   │   │   │   └── ...
│   │   │   ├── Timeline.jsx     # Scrubber + step controls
│   │   │   ├── VarInspector.jsx # Live variable panel
│   │   │   └── ComplexityGraph.jsx
│   │   └── App.jsx
│   └── public/
├── examples/
│   ├── dijkstra.cpp
│   ├── bfs.py
│   ├── knapsack_dp.cpp
│   └── dsu_union.cpp
└── README.md
```

---

## Roadmap

- [x] Trace format spec
- [x] C++ instrumentation header
- [x] Python instrumentation layer
- [ ] Heap renderer
- [ ] Graph traversal renderer
- [ ] DP table renderer
- [ ] Timeline + scrubber UI
- [ ] Variable inspector panel
- [ ] Complexity analyzer
- [ ] DSU renderer
- [ ] Java support (planned)
- [ ] `.keiro` file sharing + embed links
---

## Authors

Ahmad — CS sophomore, [Codeforces: Ahmad-Skeptical](https://codeforces.com/profile/Ahmad-Skeptical)

---

*KEIRO is named after 経路 (keiro) — Japanese for path, route, or trace.*
