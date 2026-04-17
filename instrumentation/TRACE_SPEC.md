# KEIRO Trace Format Specification
### `.keiro` file schema — v1.0

A `.keiro` file is a UTF-8 encoded JSON document emitted by the instrumentation
layer (C++ header or Python module) during a single execution run. The frontend
consumes it to drive the replay UI.

---

## Top-level structure

```json
{
  "keiro_version": "1.0",
  "meta": { ... },
  "steps": [ ... ]
}
```

| Field | Type | Description |
|---|---|---|
| `keiro_version` | string | Schema version. Always `"1.0"` for now. |
| `meta` | object | Static info about the run — set once, never changes. |
| `steps` | array | Ordered list of execution events. One entry per DS operation. |

---

## `meta` object

```json
"meta": {
  "lang": "cpp",
  "source_file": "dijkstra.cpp",
  "stdin": "5 6\n1 2 4\n1 3 2\n2 3 1\n2 4 5\n3 4 8\n4 5 2",
  "structures": {
    "pq": "heap",
    "adj": "graph",
    "dist": "array"
  },
  "total_steps": 42,
  "exit_code": 0,
  "compile_time_ms": 312,
  "run_time_ms": 4
}
```

| Field | Type | Description |
|---|---|---|
| `lang` | `"cpp"` \| `"python"` \| `"java"` | Source language |
| `source_file` | string | Original filename |
| `stdin` | string | Exact input fed to the program |
| `structures` | object | Map of variable name → renderer type. Tells the frontend which canvas to load for each tracked structure. |
| `total_steps` | int | Total number of steps in the `steps` array |
| `exit_code` | int | Process exit code. `0` = clean exit |
| `compile_time_ms` | int | (C++ only) Compilation time in ms |
| `run_time_ms` | int | Total execution time in ms |

### Renderer types (valid values in `structures`)

| Value | Renders as |
|---|---|
| `"heap"` | Binary heap tree |
| `"graph"` | Force-directed node-link graph |
| `"array"` | Linear strip (stack, queue, vector) |
| `"grid"` | 2D table (DP arrays) |
| `"dsu"` | Union-find forest |
| `"hashmap"` | Bucket array with chains |
| `"tree"` | Generic BST / segment tree |

---

## `steps` array

Each element is a single operation event — one push, one pop, one graph edge
relaxation, one DP cell write, etc.

```json
{
  "step": 4,
  "line": 12,
  "op": "push",
  "ds_name": "pq",
  "ds_type": "heap",
  "val": [0, 1],
  "snapshot": [
    [0, 1]
  ],
  "vars": {
    "src": 1,
    "dist[1]": 0,
    "pq.size()": 1
  },
  "vars_changed": ["dist[1]", "pq.size()"],
  "op_count": 4
}
```

| Field | Type | Description |
|---|---|---|
| `step` | int | 0-indexed step number. Monotonically increasing. |
| `line` | int | Source line number that triggered this event |
| `op` | string | Operation name. See op types below. |
| `ds_name` | string | Variable name of the structure being operated on |
| `ds_type` | string | Renderer type (mirrors `meta.structures`) |
| `val` | any | The value involved in the operation. Type depends on `ds_type`. |
| `snapshot` | any | Full state of the structure *after* this operation. Type depends on `ds_type`. |
| `vars` | object | Current value of all tracked variables at this step |
| `vars_changed` | array[string] | Keys in `vars` that changed since the previous step. Used by the inspector to highlight diffs. |
| `op_count` | int | Cumulative count of all DS operations up to and including this step |

---

## Operation types (`op` field)

### Heap / priority queue
| Op | Meaning |
|---|---|
| `push` | Element inserted |
| `pop` | Top element removed |
| `peek` | Top element read (no structural change) |

### Array / stack / queue / deque
| Op | Meaning |
|---|---|
| `push_back` | Append to end |
| `push_front` | Prepend to front |
| `pop_back` | Remove from end |
| `pop_front` | Remove from front |
| `set` | Element at index modified (`arr[i] = x`) |
| `get` | Element at index read |

### Graph
| Op | Meaning |
|---|---|
| `visit` | Node marked visited |
| `relax` | Edge relaxed (Dijkstra / Bellman-Ford) |
| `enqueue` | Node added to BFS queue |
| `dequeue` | Node removed from BFS queue |
| `push_stack` | Node pushed to DFS stack |
| `pop_stack` | Node popped from DFS stack |

### Grid (2D array / DP)
| Op | Meaning |
|---|---|
| `set` | Cell `[i][j]` written |
| `get` | Cell `[i][j]` read |

### DSU (Union-Find)
| Op | Meaning |
|---|---|
| `find` | Root lookup (with path compression) |
| `union` | Two components merged |

### Hash map
| Op | Meaning |
|---|---|
| `insert` | Key-value pair inserted |
| `erase` | Key removed |
| `lookup` | Key accessed |
| `update` | Existing key's value changed |

---

## `snapshot` format by type

The `snapshot` field contains the full structure state after each operation.
Exact shape depends on `ds_type`:

### `heap`
Array in heap order (index 0 = root). The frontend derives the tree layout
from array positions using the standard `parent = (i-1)/2` formula.

```json
"snapshot": [1, 3, 2, 7, 4, 5]
```

### `graph`
```json
"snapshot": {
  "nodes": [
    { "id": 1, "visited": true,  "dist": 0 },
    { "id": 2, "visited": false, "dist": 4 },
    { "id": 3, "visited": false, "dist": 2 }
  ],
  "edges": [
    { "u": 1, "v": 2, "w": 4, "relaxed": false },
    { "u": 1, "v": 3, "w": 2, "relaxed": true  }
  ],
  "active_node": 1
}
```

### `array` / `stack` / `queue`
Ordered array of current elements, front to back.

```json
"snapshot": [3, 1, 4, 1, 5]
```

### `grid`
2D array. Null = uncomputed cell.

```json
"snapshot": [
  [0,    1,    2,    3   ],
  [1,    null, null, null],
  [2,    null, null, null]
]
```

### `dsu`
Array where `parent[i]` is the parent of node `i`. Root nodes satisfy
`parent[i] == i`.

```json
"snapshot": {
  "parent": [0, 0, 2, 0, 4],
  "rank":   [2, 0, 1, 0, 0]
}
```

### `hashmap`
Array of buckets, each bucket is a list of `[key, value]` pairs (chain).

```json
"snapshot": [
  [[3, "c"]],
  [],
  [[7, "a"], [2, "b"]]
]
```

---

## Full example — 3-step Dijkstra trace

```json
{
  "keiro_version": "1.0",
  "meta": {
    "lang": "cpp",
    "source_file": "dijkstra.cpp",
    "stdin": "3 2\n1 2 4\n1 3 2",
    "structures": {
      "pq":   "heap",
      "dist": "array"
    },
    "total_steps": 3,
    "exit_code": 0,
    "compile_time_ms": 298,
    "run_time_ms": 2
  },
  "steps": [
    {
      "step": 0,
      "line": 8,
      "op": "push",
      "ds_name": "pq",
      "ds_type": "heap",
      "val": [0, 1],
      "snapshot": [[0, 1]],
      "vars": { "src": 1, "pq.size()": 1 },
      "vars_changed": ["pq.size()"],
      "op_count": 1
    },
    {
      "step": 1,
      "line": 12,
      "op": "pop",
      "ds_name": "pq",
      "ds_type": "heap",
      "val": [0, 1],
      "snapshot": [],
      "vars": { "src": 1, "u": 1, "d": 0, "pq.size()": 0 },
      "vars_changed": ["u", "d", "pq.size()"],
      "op_count": 2
    },
    {
      "step": 2,
      "line": 17,
      "op": "relax",
      "ds_name": "pq",
      "ds_type": "graph",
      "val": { "u": 1, "v": 2, "w": 4 },
      "snapshot": {
        "nodes": [
          { "id": 1, "visited": true,  "dist": 0 },
          { "id": 2, "visited": false, "dist": 4 },
          { "id": 3, "visited": false, "dist": 99999 }
        ],
        "edges": [
          { "u": 1, "v": 2, "w": 4, "relaxed": true  },
          { "u": 1, "v": 3, "w": 2, "relaxed": false }
        ],
        "active_node": 1
      },
      "vars": { "src": 1, "u": 1, "d": 0, "dist[2]": 4 },
      "vars_changed": ["dist[2]"],
      "op_count": 3
    }
  ]
}
```

---

## Constraints

| Property | Limit |
|---|---|
| Max steps | 100,000 |
| Max snapshot size per step | 1 MB |
| Max total file size | 50 MB |
| Execution timeout | 5 seconds |
| Max source file size | 64 KB |

---

## Versioning

The `keiro_version` field exists for forward compatibility. Breaking changes to
the schema bump the major version (`1.0` → `2.0`). Additive changes (new op
types, new optional fields) bump the minor version (`1.0` → `1.1`) and are
backward compatible.
