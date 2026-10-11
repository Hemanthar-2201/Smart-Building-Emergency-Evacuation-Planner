#include <algorithm>
#include <iostream>
#include <queue>
#include <set>
#include <string>
#include <vector>
using namespace std;

struct Edge {
    int floor, time;
    char stair;
};

int main() {
    const int TOP = 100, STEP_TIME = 5;
    vector<vector<Edge>> graph(TOP + 1); // Adjacency list
    set<char> unsafeStairs = {'A', 'B'};
    vector<bool> fire(TOP + 1, false), smoke(TOP + 1, false);

    int fireFloor = 68;
    fire[fireFloor] = true;
    if (fireFloor > 0) smoke[fireFloor - 1] = true;
    if (fireFloor < TOP) smoke[fireFloor + 1] = true;

    // Connect each floor to its neighbors using four stairwells.
    for (int floor = 0; floor < TOP; ++floor) {
        for (char stair = 'A'; stair <= 'D'; ++stair) {
            graph[floor].push_back({floor + 1, STEP_TIME, stair});
            graph[floor + 1].push_back({floor, STEP_TIME, stair});
        }
    }

    // Find the quickest safe route from one floor to ground (0) or roof (100).
    auto findRoute = [&](int start) {
        vector<int> distance(TOP + 1, 1'000'000), previous(TOP + 1, -1);
        vector<char> usedStair(TOP + 1);
        priority_queue<pair<int, int>, vector<pair<int, int>>,
                       greater<pair<int, int>>> pending; // Min-heap

        distance[start] = 0;
        pending.push({0, start});
        while (!pending.empty()) {
            auto [cost, floor] = pending.top();
            pending.pop();
            if (cost != distance[floor]) continue;
            if (floor == 0 || floor == TOP) break;

            for (const Edge& edge : graph[floor]) {
                if (fire[edge.floor]) continue; // Never route into a fire floor.
                bool nearHazard = fire[floor] || smoke[floor] || smoke[edge.floor];
                if (nearHazard && unsafeStairs.count(edge.stair)) continue;

                int nextCost = cost + edge.time;
                if (nextCost < distance[edge.floor]) {
                    distance[edge.floor] = nextCost;
                    previous[edge.floor] = floor;
                    usedStair[edge.floor] = edge.stair;
                    pending.push({nextCost, edge.floor});
                }
            }
        }

        int exit = distance[0] <= distance[TOP] ? 0 : TOP;
        if (distance[exit] == 1'000'000) {
            cout << "Floor " << start << ": no safe route found.\n";
            return;
        }

        vector<int> path;
        for (int floor = exit; floor != -1; floor = previous[floor]) {
            path.push_back(floor);
        }
        reverse(path.begin(), path.end());
        cout << "Floor " << start << " -> " << (exit == 0 ? "Ground" : "Roof")
             << " using Stairwell " << usedStair[path[1]]
             << " (" << distance[exit] << " seconds)\n";
    };

    cout << "Fire: Floor " << fireFloor
         << " | Smoke: Floors " << fireFloor - 1 << " and " << fireFloor + 1
         << " | Elevators: OFF\n";
    findRoute(fireFloor - 1); // Nearby smoke floor
    findRoute(fireFloor);     // Fire floor evacuates outward, never into the fire.
    findRoute(fireFloor + 1); // Nearby smoke floor
    findRoute(85);            // Upper floors route toward the roof.
}