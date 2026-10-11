# SmartTower Digital Twin

A coursework demonstration of building-floor hazard modeling and evacuation route planning. It includes:

- A browser dashboard for interactively visualizing a 100-floor evacuation drill.
- A CSV-driven C++ console program that loads a sample building graph and occupant counts, then uses Dijkstra's algorithm to produce example routes and alerts.
- A small, standalone `source_code.cpp` that illustrates the core data structures and routing idea without loading files.

> **Safety notice:** This is a teaching simulation, not a life-safety system. It does not read real sensors, send phone notifications, control elevators or sprinklers, calculate certified crowd capacity, or provide authoritative evacuation advice. Follow the building's approved emergency plan and instructions from emergency services.

## Student Team Members

1. Hemantha R
2. Mohammed Hasanayn R
3. Dolsi Chanda
4. Sahana Sangappa Waghamore
5. Valligatla Rojitha
   
## Project files

| File | Purpose |
| --- | --- |
| `index.html` | Self-contained interactive browser visualization (HTML, CSS, and JavaScript). |
| `digital_twin.cpp` | Main CSV-backed C++ console simulation. |
| `source_code.cpp` | Short standalone example of an adjacency list, a min-priority queue, hazard checks, and route selection. |
| `sample_nodes.csv` | Building nodes, including floor hubs, stair nodes, and exits. |
| `sample_edges.csv` | Connections between nodes, travel times, widths, edge types, and hazard flags. |
| `sample_occupants.csv` | Example occupant count and hub node for each floor. |
| `testcases.txt` | Selectable fire-floor and unsafe-stair scenarios for `digital_twin.cpp`. |
| `launch_dashboard.bat` | Compiles and opens the C++ demo and browser page on Windows. |
| `.vscode/tasks.json` | VS Code C++ build task configuration. |
| `.vscode/settings.json` | Live Server port setting (5501). |

## Requirements

- Windows
- A C++17 compiler such as GCC (`g++`) available on `PATH`
- A modern browser
- Optional: Visual Studio Code with the Live Server extension

The C++ program uses only the standard library; no additional packages are required.

## Run the browser dashboard

Open `index.html` in a browser, or open the project folder in VS Code and choose **Open with Live Server**. The workspace Live Server port is configured as **5501**.

In the dashboard:

1. Enter one or more incident floors from 0 to 100, separated by commas, or click floor rows.
2. Select the stairwells considered unsafe near the incident.
3. Choose **Start incident drill** to display the simulated routes, floor states, and resident alerts.
4. Use **Reset model** to clear the drill and restore the default A/B unsafe selections.

The dashboard is self-contained and does not call the C++ program or load the CSV files. It has its own illustrative 100-floor graph and simulated sprinkler timeline.

## Run the CSV-backed C++ simulation

From the project directory, compile and run:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic digital_twin.cpp -o digital_twin.exe
.\digital_twin.exe
```

Or run `launch_dashboard.bat` to compile the C++ demo and open both the console program and `index.html`. The launcher needs `g++` on `PATH`.

The console program lists the test cases in `testcases.txt`:

| Case | Scenario |
| --- | --- |
| 1 | Fire on Floor 68; Stairwells A and B unsafe |
| 2 | Fire on Floor 68; all four stairwells unsafe |
| 3 | Fires on Floors 35 and 68; Stairwells A and B unsafe |
| 4 | Fire on Floor 100; Stairwells A and B unsafe |
| 5 | Fire on Floor 0; Stairwells A and B unsafe |

Choose a case number, or `0` to enter a custom list of fire floors and unsafe stairwells. Press Enter twice at the end of the simulation to advance its sprinkler demonstration stages.

By default, the program opens these data files in its **current working directory**:

```text
sample_edges.csv
sample_nodes.csv
sample_occupants.csv
testcases.txt
```

You can provide alternative files in this argument order:

```powershell
.\digital_twin.exe .\sample_edges.csv .\sample_nodes.csv .\sample_occupants.csv .\testcases.txt
```

## CSV formats

The loader reads columns by header name, so their order may vary. Column names are case-insensitive.

### `sample_nodes.csv`

```csv
node_id,name,floor,type
```

- `node_id`: unique non-negative graph node ID.
- `name`: display name for the node.
- `floor`: associated floor; exits may use special values such as `-1`.
- `type`: node kind. The sample uses `hub`, `stairA` through `stairD`, `exit_ground`, and `exit_roof`.

### `sample_edges.csv`

```csv
edge_id,from,to,time_s,width_m,type,blocked,smoke
```

- `edge_id`: edge identifier.
- `from`, `to`: node IDs that the edge connects. The current loader treats each connection as bidirectional.
- `time_s`: positive travel cost in seconds used by the route planner.
- `width_m`: positive width stored for display in the C++ output.
- `type`: connection type, such as `corridor`, `stair`, or `exit`.
- `blocked`: non-zero means the connection is unavailable.
- `smoke`: a value greater than zero makes the connection unavailable in the current C++ model.

Stairwell labels are inferred from connected node types such as `stairA`, `stairB`, `stairC`, and `stairD`.

### `sample_occupants.csv`

```csv
floor,hub_node,persons
```

- `floor`: occupant group's floor.
- `hub_node`: graph node for the group; it must exist on the same floor.
- `persons`: non-negative example population count.

### `testcases.txt`

Despite its extension, this file uses a CSV header and rows:

```csv
case_id,name,fire_floors,unsafe_stairs
```

Fire floors and unsafe stairwell labels are comma-separated. Quote fields containing commas, for example `"A,B"`.

## How the C++ model works

1. Loads graph nodes, connections, occupant groups, and test cases from the CSV files.
2. Marks the chosen fire floors and their adjacent floors as smoke-affected.
3. Finds graph routes from the ground and roof exits using Dijkstra's algorithm and a min-priority queue.
4. Excludes blocked or smoke-flagged edges, edges into fire floors, and selected unsafe stairwells where a route touches the incident zone.
5. Selects an available exit for each priority occupant group, prints simulated floor alerts, and summarizes upper/lower zones and shared stairwell loads.
6. Prints a manual, demonstration-only sprinkler sequence; it does not wait for real time or control equipment.

`source_code.cpp` is intentionally separate from the CSV-backed program. It is a compact teaching example with a fixed 100-floor graph, default incident on Floor 68, and Stairwells A/B marked unsafe.

## Current model assumptions and limitations

- The sample CSVs contain illustrative route times, widths, and occupant counts, not measured or certified building data.
- Smoke affects the floors immediately above and below each fire floor in the C++ demonstration. It is not a physical smoke or airflow model.
- The unsafe-stair selection is a user-provided drill input, not a sensor reading.
- The CSV C++ program does not simulate elevator or sprinkler hardware. It prints drill status only.
- Route time is the sum of edge travel times. The model does not calculate crowd flow, stairwell throughput, mobility assistance, fire-door behavior, or queuing delay.
- The browser dashboard and C++ console program are separate demonstrations; the dashboard does not consume the C++ results.
