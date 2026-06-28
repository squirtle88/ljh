# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Test Commands

```bash
# Full build (from project root)
gcc -Wall -Wextra -I pzx -I yjk -I zxc pzx/ds.c yjk/parking.c yjk/fee.c zxc/stats.c zxc/ui.c zxc/file_io.c zxc/main.c -o parking.exe -lm

# Run standalone ds tests
gcc -I pzx pzx/ds.c pzx/test_ds.c -o test_ds.exe && ./test_ds.exe
```

The `-lm` flag is required because `fee.c` uses `ceil()`. Include paths (`-I`) for each module directory are required for header resolution.

## Project Architecture

Three-layer C project with strict interface boundaries:

```
zxc/  (UI, stats, file I/O, main)
  ↓ calls
yjk/  (parking business logic, fee calculation)
  ↓ calls
pzx/  (core data structures: stack, queue, hash table, system init/destroy)
```

- **pzx** owns `ParkingSystem` struct and all low-level data structure operations. No business logic.
- **yjk** implements vehicle entry/exit, queue dispatch, fee calc. Calls pzx functions only.
- **zxc** implements menu UI, statistics, binary file save/load, and `main()`. Calls yjk for business ops.

All modules share types through `pzx/ds.h` — the single public header for structs, enums, and function declarations.

## Key Design Decisions

- **FreeSpotStack** (stack of `SpotLocation`): O(1) spot allocation on entry, O(1) release on exit. Avoids O(n) scan of spots array.
- **HashMap** (10007 buckets, chaining): O(1) average plate lookup for entry duplicate check and exit billing.
- **WaitingQueue** (linked-list FIFO): unbounded queue for overflow vehicles. On exit, dequeued vehicle takes the freed spot directly (never goes through the stack).
- **Hash table not persisted**: `rebuild_hash_from_spots()` scans spots array after load_data. Caveat: entry_time becomes `time(NULL)` for rebuilt records.
- **VehicleRecord lifecycle**: allocated by yjk on entry, freed by yjk on exit. `ht_remove()` only frees the hash node, not the record.

## Code Conventions

| Category | Style | Example |
|----------|-------|---------|
| Structs | PascalCase | `VehicleRecord`, `ParkingSpot` |
| Enums | UPPER_SNAKE | `SPOT_FREE`, `ENTRY_SUCCESS` |
| Functions | snake_case | `vehicle_entry`, `ht_find` |
| Variables | snake_case | `free_count`, `entry_time` |
| Macros | UPPER_SNAKE | `TOTAL_SPOTS`, `HT_SIZE` |

Error handling: all `malloc`/`realloc`/`fopen` checked for NULL. Public functions validate `ps != NULL`. String copies use `strncpy` with explicit null termination.
