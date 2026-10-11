#include <algorithm>
#include <cctype>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

constexpr int NO_ROUTE = std::numeric_limits<int>::max();
struct Node {
    int id;
    std::string name;
    int floor;
    std::string type;
};

struct Edge {
    int id;
    int from;
    int to;
    int timeSeconds;
    double widthMeters;
    std::string type;
    bool blocked;
    double smoke;
};

struct Occupants {
    int floor;
    int hubNode;
    int people;
};

struct RoutePlan {
    std::vector<int> time;
    std::vector<int> nextNode;
    int exitNode = -1;
};

struct TestCase {
    int id;
    std::string name;
    std::string fireFloors;
    std::string unsafeStairs;
};

using CsvTable = std::vector<std::vector<std::string>>;

std::string trim(const std::string& value) {
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

std::vector<std::string> splitCsvRow(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;

    for (std::size_t index = 0; index < line.size(); ++index) {
        const char character = line[index];
        if (character == '"') {
            if (quoted && index + 1 < line.size() && line[index + 1] == '"') {
                field += '"';
                ++index;
            } else {
                quoted = !quoted;
            }
        } else if (character == ',' && !quoted) {
            fields.push_back(trim(field));
            field.clear();
        } else {
            field += character;
        }
    }
    fields.push_back(trim(field));
    return fields;
}

CsvTable readCsv(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Could not open " + filename);
    }

    CsvTable rows;
    std::string line;
    while (std::getline(file, line)) {
        if (!trim(line).empty()) {
            rows.push_back(splitCsvRow(line));
        }
    }
    if (rows.empty()) {
        throw std::runtime_error(filename + " is empty");
    }
    return rows;
}

std::unordered_map<std::string, std::size_t> makeColumnMap(const CsvTable& rows) {
    std::unordered_map<std::string, std::size_t> columns;
    for (std::size_t index = 0; index < rows.front().size(); ++index) {
        columns[lowercase(trim(rows.front()[index]))] = index;
    }
    return columns;
}

std::size_t column(
    const std::unordered_map<std::string, std::size_t>& columns,
    const std::string& name,
    const std::string& filename) {
    const auto found = columns.find(name);
    if (found == columns.end()) {
        throw std::runtime_error(filename + " is missing column '" + name + "'");
    }
    return found->second;
}

std::string cell(
    const std::vector<std::string>& row,
    std::size_t index,
    const std::string& filename,
    std::size_t rowNumber) {
    if (index >= row.size()) {
        throw std::runtime_error(filename + " has an incomplete row " + std::to_string(rowNumber));
    }
    return trim(row[index]);
}

int integerCell(
    const std::vector<std::string>& row,
    std::size_t index,
    const std::string& filename,
    std::size_t rowNumber) {
    const std::string value = cell(row, index, filename, rowNumber);
    std::size_t parsed = 0;
    int result = 0;
    try {
        result = std::stoi(value, &parsed);
    } catch (const std::exception&) {
        throw std::runtime_error(filename + " has an invalid integer on row " + std::to_string(rowNumber));
    }
    if (parsed != value.size()) {
        throw std::runtime_error(filename + " has an invalid integer on row " + std::to_string(rowNumber));
    }
    return result;
}

double numberCell(
    const std::vector<std::string>& row,
    std::size_t index,
    const std::string& filename,
    std::size_t rowNumber) {
    const std::string value = cell(row, index, filename, rowNumber);
    std::size_t parsed = 0;
    double result = 0;
    try {
        result = std::stod(value, &parsed);
    } catch (const std::exception&) {
        throw std::runtime_error(filename + " has an invalid number on row " + std::to_string(rowNumber));
    }
    if (parsed != value.size()) {
        throw std::runtime_error(filename + " has an invalid number on row " + std::to_string(rowNumber));
    }
    return result;
}

std::string stairwellName(const Edge& edge, const std::unordered_map<int, Node>& nodes) {
    for (int nodeId : {edge.from, edge.to}) {
        const std::string type = lowercase(nodes.at(nodeId).type);
        const std::size_t stairPosition = type.find("stair");
        if (stairPosition != std::string::npos && stairPosition + 5 < type.size()) {
            const char label = static_cast<char>(std::toupper(
                static_cast<unsigned char>(type[stairPosition + 5])));
            if (label >= 'A' && label <= 'Z') {
                return std::string(1, label);
            }
        }
    }

    const std::string type = lowercase(edge.type);
    if (type.rfind("stair", 0) == 0 && type.size() > 5) {
        return std::string(1, static_cast<char>(std::toupper(
            static_cast<unsigned char>(type[5]))));
    }
    return "";
}

class DigitalTwin {
public:
    DigitalTwin(const std::string& edgesFile,
                const std::string& nodesFile,
                const std::string& occupantsFile) {
        loadNodes(nodesFile);
        loadEdges(edgesFile);
        loadOccupants(occupantsFile);
    }

    int minimumFloor() const {
        int result = std::numeric_limits<int>::max();
        for (const auto& entry : nodes) {
            if (entry.second.floor >= 0) {
                result = std::min(result, entry.second.floor);
            }
        }
        return result;
    }

    int maximumFloor() const {
        int result = -1;
        for (const auto& entry : nodes) {
            result = std::max(result, entry.second.floor);
        }
        return result;
    }

    bool hasFloor(int floor) const {
        return floor >= 0 && std::any_of(nodes.begin(), nodes.end(), [floor](const auto& entry) {
            const std::string type = lowercase(entry.second.type);
            return entry.second.floor == floor && type.find("exit") == std::string::npos;
        });
    }

    std::set<std::string> availableStairs() const {
        std::set<std::string> stairs;
        for (const Edge& edge : edges) {
            const std::string name = stairwellName(edge, nodes);
            if (!name.empty()) {
                stairs.insert(name);
            }
        }
        return stairs;
    }

    void showResponse(const std::set<int>& fireFloors,
                      const std::set<std::string>& unsafeStairs) const {
        const std::set<int> smokeFloors = findSmokeFloors(fireFloors);
        const int groundExit = findExitNode(false);
        const int roofExit = findExitNode(true);
        const RoutePlan groundRoutes = findRoutes(groundExit, fireFloors, smokeFloors, unsafeStairs);
        const RoutePlan roofRoutes = findRoutes(roofExit, fireFloors, smokeFloors, unsafeStairs);

        std::cout << "\n=== SMARTTOWER DIGITAL TWIN ===\n";
        std::cout << "Dataset nodes: " << nodes.size()
                  << " | edges: " << edges.size()
                  << " | occupant groups: " << occupants.size() << '\n';
        std::cout << "Fire floors: ";
        printSet(fireFloors);
        std::cout << "\nSmoke-affected floors: ";
        printSet(smokeFloors);
        std::cout << "\nUnsafe stairwells: ";
        if (unsafeStairs.empty()) {
            std::cout << "none";
        } else {
            printSet(unsafeStairs);
        }
        std::cout << "\nElevators: SHUT DOWN (stair-only drill)\n";

        std::cout << "Ground exit: " << exitName(groundExit) << '\n';
        std::cout << "Roof exit: " << exitName(roofExit) << '\n';

        printOccupantRoutes(fireFloors, smokeFloors, groundRoutes, roofRoutes);
        printZoneSummary(fireFloors, smokeFloors, groundRoutes, roofRoutes);

        std::cout << "\n=== SPRINKLER RESPONSE (DEMO ONLY) ===\n";
        waitForDrillStep("Press Enter to simulate sprinkler activation...");
        std::cout << "Sprinklers activated in the demonstration. Keep following evacuation instructions.\n";
        waitForDrillStep("Press Enter to simulate the suppression phase completing...");
        std::cout << "Fire marked CONTROLLED in the demonstration.\n";
        std::cout << "Elevators stay shut down until authorized inspection and clearance.\n";
        std::cout << "This is not a real all-clear. Follow emergency services and the approved building plan.\n";
    }

private:
    std::unordered_map<int, Node> nodes;
    std::vector<Edge> edges;
    std::unordered_map<int, std::vector<std::size_t>> adjacency;
    std::vector<Occupants> occupants;

    void loadNodes(const std::string& filename) {
        const CsvTable rows = readCsv(filename);
        const auto columns = makeColumnMap(rows);
        const std::size_t idColumn = column(columns, "node_id", filename);
        const std::size_t nameColumn = column(columns, "name", filename);
        const std::size_t floorColumn = column(columns, "floor", filename);
        const std::size_t typeColumn = column(columns, "type", filename);

        for (std::size_t index = 1; index < rows.size(); ++index) {
            const std::size_t rowNumber = index + 1;
            Node node{
                integerCell(rows[index], idColumn, filename, rowNumber),
                cell(rows[index], nameColumn, filename, rowNumber),
                integerCell(rows[index], floorColumn, filename, rowNumber),
                cell(rows[index], typeColumn, filename, rowNumber)
            };
            if (node.id < 0 || !nodes.emplace(node.id, node).second) {
                throw std::runtime_error(filename + " has an invalid or duplicate node_id on row " +
                                         std::to_string(rowNumber));
            }
        }
        if (nodes.empty()) {
            throw std::runtime_error(filename + " contains no nodes");
        }
    }

    void loadEdges(const std::string& filename) {
        const CsvTable rows = readCsv(filename);
        const auto columns = makeColumnMap(rows);
        const std::size_t idColumn = column(columns, "edge_id", filename);
        const std::size_t fromColumn = column(columns, "from", filename);
        const std::size_t toColumn = column(columns, "to", filename);
        const std::size_t timeColumn = column(columns, "time_s", filename);
        const std::size_t widthColumn = column(columns, "width_m", filename);
        const std::size_t typeColumn = column(columns, "type", filename);
        const std::size_t blockedColumn = column(columns, "blocked", filename);
        const std::size_t smokeColumn = column(columns, "smoke", filename);

        for (std::size_t index = 1; index < rows.size(); ++index) {
            const std::size_t rowNumber = index + 1;
            Edge edge{
                integerCell(rows[index], idColumn, filename, rowNumber),
                integerCell(rows[index], fromColumn, filename, rowNumber),
                integerCell(rows[index], toColumn, filename, rowNumber),
                integerCell(rows[index], timeColumn, filename, rowNumber),
                numberCell(rows[index], widthColumn, filename, rowNumber),
                cell(rows[index], typeColumn, filename, rowNumber),
                integerCell(rows[index], blockedColumn, filename, rowNumber) != 0,
                numberCell(rows[index], smokeColumn, filename, rowNumber)
            };
            if (nodes.count(edge.from) == 0 || nodes.count(edge.to) == 0 ||
                edge.timeSeconds <= 0 || edge.widthMeters <= 0 || edge.smoke < 0) {
                throw std::runtime_error(filename + " has invalid edge data on row " +
                                         std::to_string(rowNumber));
            }
            const std::size_t edgeIndex = edges.size();
            edges.push_back(edge);
            adjacency[edge.from].push_back(edgeIndex);
            adjacency[edge.to].push_back(edgeIndex);
        }
    }

    void loadOccupants(const std::string& filename) {
        const CsvTable rows = readCsv(filename);
        const auto columns = makeColumnMap(rows);
        const std::size_t floorColumn = column(columns, "floor", filename);
        const std::size_t hubColumn = column(columns, "hub_node", filename);
        const std::size_t peopleColumn = column(columns, "persons", filename);

        for (std::size_t index = 1; index < rows.size(); ++index) {
            const std::size_t rowNumber = index + 1;
            Occupants group{
                integerCell(rows[index], floorColumn, filename, rowNumber),
                integerCell(rows[index], hubColumn, filename, rowNumber),
                integerCell(rows[index], peopleColumn, filename, rowNumber)
            };
            const auto hub = nodes.find(group.hubNode);
            if (hub == nodes.end() || hub->second.floor != group.floor || group.people < 0) {
                throw std::runtime_error(filename + " has invalid occupant data on row " +
                                         std::to_string(rowNumber));
            }
            occupants.push_back(group);
        }
    }

    std::set<int> findSmokeFloors(const std::set<int>& fireFloors) const {
        std::set<int> smokeFloors;
        for (int floor : fireFloors) {
            for (int nearby : {floor - 1, floor + 1}) {
                if (hasFloor(nearby) && fireFloors.count(nearby) == 0) {
                    smokeFloors.insert(nearby);
                }
            }
        }
        return smokeFloors;
    }

    RoutePlan findRoutes(int exitNode,
                         const std::set<int>& fireFloors,
                         const std::set<int>& smokeFloors,
                         const std::set<std::string>& unsafeStairs) const {
        const int maxNode = std::max_element(nodes.begin(), nodes.end(),
            [](const auto& left, const auto& right) {
                return left.first < right.first;
            })->first;
        RoutePlan routes{
            std::vector<int>(maxNode + 1, NO_ROUTE),
            std::vector<int>(maxNode + 1, -1),
            exitNode
        };
        if (exitNode == -1 || fireFloors.count(nodes.at(exitNode).floor)) {
            return routes;
        }

        using QueueEntry = std::pair<int, int>;
        std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> queue;
        routes.time[exitNode] = 0;
        queue.push({0, exitNode});

        while (!queue.empty()) {
            const auto [currentTime, currentNode] = queue.top();
            queue.pop();
            if (currentTime != routes.time[currentNode]) {
                continue;
            }

            const auto adjacentEdges = adjacency.find(currentNode);
            if (adjacentEdges == adjacency.end()) {
                continue;
            }
            for (std::size_t edgeIndex : adjacentEdges->second) {
                const Edge& edge = edges[edgeIndex];
                const int sourceNode = edge.from == currentNode ? edge.to : edge.from;
                const Node& current = nodes.at(currentNode);
                const Node& source = nodes.at(sourceNode);
                const bool sourceIsFire = fireFloors.count(source.floor) > 0;
                const bool currentIsFire = fireFloors.count(current.floor) > 0;
                const bool touchesHazard =
                    fireFloors.count(source.floor) || fireFloors.count(current.floor) ||
                    smokeFloors.count(source.floor) || smokeFloors.count(current.floor);
                const std::string stairwell = stairwellName(edge, nodes);

                if (edge.blocked || edge.smoke > 0 ||
                    (currentIsFire && !sourceIsFire) ||
                    (touchesHazard && !stairwell.empty() && unsafeStairs.count(stairwell))) {
                    continue;
                }

                const int newTime = currentTime + edge.timeSeconds;
                if (newTime < routes.time[sourceNode]) {
                    routes.time[sourceNode] = newTime;
                    routes.nextNode[sourceNode] = currentNode;
                    queue.push({newTime, sourceNode});
                }
            }
        }
        return routes;
    }

    void printOccupantRoutes(const std::set<int>& fireFloors,
                             const std::set<int>& smokeFloors,
                             const RoutePlan& groundRoutes,
                             const RoutePlan& roofRoutes) const {
        std::cout << "\n=== OCCUPANT ROUTES & SIMULATED MOBILE ALERTS ===\n";
        std::unordered_map<std::string, int> stairLoads;
        std::unordered_map<std::string, double> stairWidths;

        for (const Occupants& group : occupants) {
            const bool priorityFloor =
                fireFloors.count(group.floor) || smokeFloors.count(group.floor);
            std::cout << "[Floor " << group.floor << " | " << group.people << " people] ";
            const RoutePlan* routePlan = selectRoute(group.hubNode, groundRoutes, roofRoutes);
            if (!priorityFloor) {
                const bool upperZone = groundRoutes.time[group.hubNode] == NO_ROUTE &&
                    roofRoutes.time[group.hubNode] != NO_ROUTE;
                if (upperZone) {
                    std::cout << "UPPER ZONE: " << describeRoute(group.hubNode, roofRoutes)
                              << " (estimated path time " << roofRoutes.time[group.hubNode] << " s).\n";
                    std::cout << "  [MOBILE ALERT SIMULATED] Floor " << group.floor
                              << ": Follow the approved plan to the designated roof refuge. "
                              << "Do not use elevators.\n";
                    const std::string firstStair = firstStairwell(group.hubNode, roofRoutes);
                    if (!firstStair.empty()) {
                        stairLoads[firstStair] += group.people;
                        stairWidths[firstStair] = firstStairWidth(group.hubNode, roofRoutes);
                    }
                } else {
                    std::cout << "STAGED: keep stairs clear and await floor-specific instructions.\n";
                }
                continue;
            }

            if (routePlan == nullptr) {
                std::cout << "NO MODELED EXIT. Seek a protected refuge and alert responders "
                          << "according to the approved building plan.\n";
                continue;
            }

            const std::string route = describeRoute(group.hubNode, *routePlan);
            std::cout << "Priority evacuation: " << route
                      << " (estimated path time " << routePlan->time[group.hubNode] << " s).\n";
            std::cout << "  [MOBILE ALERT SIMULATED] Floor " << group.floor << ": "
                      << route << " Do not use elevators.\n";

            const std::string firstStair = firstStairwell(group.hubNode, *routePlan);
            if (!firstStair.empty()) {
                stairLoads[firstStair] += group.people;
                stairWidths[firstStair] = firstStairWidth(group.hubNode, *routePlan);
            }
        }

        if (!stairLoads.empty()) {
            std::cout << "\nShared stairwell loads (capacity data is not provided):\n";
            for (const auto& entry : stairLoads) {
                std::cout << "  Stairwell " << entry.first << ": " << entry.second
                          << " people assigned; first-link width "
                          << stairWidths[entry.first] << " m. Check for queues.\n";
            }
        }
    }

    std::string describeRoute(int startNode, const RoutePlan& routes) const {
        const std::string stairwell = firstStairwell(startNode, routes);
        if (stairwell.empty()) {
            if (startNode == routes.exitNode) {
                return "Proceed to " + nodes.at(routes.exitNode).name;
            }
            return "Follow the marked route to " + nodes.at(routes.exitNode).name;
        }
        return "Take Stairwell " + stairwell + " toward " + nodes.at(routes.exitNode).name;
    }

    std::string firstStairwell(int startNode, const RoutePlan& routes) const {
        int currentNode = startNode;
        std::set<int> visited;
        while (currentNode != routes.exitNode && currentNode >= 0 &&
               currentNode < static_cast<int>(routes.nextNode.size()) &&
               routes.nextNode[currentNode] != -1 && visited.insert(currentNode).second) {
            const int next = routes.nextNode[currentNode];
            const std::string stairwell = stairwellName(findEdge(currentNode, next), nodes);
            if (!stairwell.empty()) {
                return stairwell;
            }
            currentNode = next;
        }
        return "";
    }

    double firstStairWidth(int startNode, const RoutePlan& routes) const {
        int currentNode = startNode;
        std::set<int> visited;
        while (currentNode != routes.exitNode && currentNode >= 0 &&
               currentNode < static_cast<int>(routes.nextNode.size()) &&
               routes.nextNode[currentNode] != -1 && visited.insert(currentNode).second) {
            const int next = routes.nextNode[currentNode];
            const Edge& edge = findEdge(currentNode, next);
            if (!stairwellName(edge, nodes).empty()) {
                return edge.widthMeters;
            }
            currentNode = next;
        }
        return 0;
    }

    const Edge& findEdge(int firstNode, int secondNode) const {
        for (std::size_t edgeIndex : adjacency.at(firstNode)) {
            const Edge& edge = edges[edgeIndex];
            if ((edge.from == firstNode && edge.to == secondNode) ||
                (edge.to == firstNode && edge.from == secondNode)) {
                return edge;
            }
        }
        throw std::runtime_error("Route references an edge that is not in the graph");
    }

    void printZoneSummary(const std::set<int>& fireFloors,
                         const std::set<int>& smokeFloors,
                         const RoutePlan& groundRoutes,
                         const RoutePlan& roofRoutes) const {
        std::cout << "\n=== FLOOR ZONES ===\n";
        for (const Occupants& group : occupants) {
            if (fireFloors.count(group.floor)) {
                std::cout << "Floor " << group.floor << ": FIRE zone; priority evacuation.\n";
            } else if (smokeFloors.count(group.floor)) {
                std::cout << "Floor " << group.floor << ": SMOKE zone; priority evacuation.\n";
            } else if (groundRoutes.time[group.hubNode] == NO_ROUTE &&
                       roofRoutes.time[group.hubNode] != NO_ROUTE) {
                std::cout << "Floor " << group.floor
                          << ": upper zone; route to designated roof refuge.\n";
            } else if (groundRoutes.time[group.hubNode] != NO_ROUTE &&
                       roofRoutes.time[group.hubNode] == NO_ROUTE) {
                std::cout << "Floor " << group.floor
                          << ": lower zone; keep stairs clear and await instructions.\n";
            } else if (groundRoutes.time[group.hubNode] == NO_ROUTE &&
                       roofRoutes.time[group.hubNode] == NO_ROUTE) {
                std::cout << "Floor " << group.floor
                          << ": no modeled safe exit; use protected refuge and alert responders.\n";
            } else {
                std::cout << "Floor " << group.floor
                          << ": both exits modeled; lower-priority zone, keep stairs clear and await instructions.\n";
            }
        }
    }

    const RoutePlan* selectRoute(int nodeId,
                                 const RoutePlan& groundRoutes,
                                 const RoutePlan& roofRoutes) const {
        const bool groundReachable = nodeId < static_cast<int>(groundRoutes.time.size()) &&
            groundRoutes.time[nodeId] != NO_ROUTE;
        const bool roofReachable = nodeId < static_cast<int>(roofRoutes.time.size()) &&
            roofRoutes.time[nodeId] != NO_ROUTE;
        if (!groundReachable && !roofReachable) {
            return nullptr;
        }
        if (!groundReachable) {
            return &roofRoutes;
        }
        if (!roofReachable || groundRoutes.time[nodeId] <= roofRoutes.time[nodeId]) {
            return &groundRoutes;
        }
        return &roofRoutes;
    }

    int findExitNode(bool roof) const {
        for (const auto& entry : nodes) {
            const std::string type = lowercase(entry.second.type);
            const std::string name = lowercase(entry.second.name);
            if (roof && (type.find("roof") != std::string::npos ||
                         name.find("roof") != std::string::npos)) {
                return entry.first;
            }
            if (!roof && (type == "exit" || type.find("ground") != std::string::npos ||
                          name.find("ground") != std::string::npos)) {
                return entry.first;
            }
        }
        return -1;
    }

    std::string exitName(int exitNode) const {
        return exitNode == -1 ? "NOT IN DATASET" : nodes.at(exitNode).name;
    }

    template <typename T>
    static void printSet(const std::set<T>& values) {
        bool first = true;
        for (const T& value : values) {
            if (!first) {
                std::cout << ", ";
            }
            std::cout << value;
            first = false;
        }
    }

    static void waitForDrillStep(const std::string& prompt) {
        std::cout << prompt << '\n';
        std::string line;
        std::getline(std::cin, line);
    }
};

std::set<int> parseFireFloors(const std::string& input, const DigitalTwin& twin) {
    std::set<int> floors;
    std::stringstream stream(input);
    std::string token;
    while (std::getline(stream, token, ',')) {
        token = trim(token);
        std::size_t parsed = 0;
        int floor = 0;
        try {
            floor = std::stoi(token, &parsed);
        } catch (const std::exception&) {
            throw std::runtime_error("Enter fire floors as comma-separated whole numbers.");
        }
        if (parsed != token.size() || !twin.hasFloor(floor)) {
            throw std::runtime_error("Floor " + token + " is not present in sample_nodes.csv.");
        }
        floors.insert(floor);
    }
    if (floors.empty()) {
        throw std::runtime_error("Enter at least one fire floor.");
    }
    return floors;
}

std::set<std::string> parseUnsafeStairs(const std::string& input) {
    std::set<std::string> stairs;
    std::stringstream stream(input);
    std::string token;
    while (std::getline(stream, token, ',')) {
        token = lowercase(trim(token));
        if (token.rfind("stairwell", 0) == 0) {
            token = trim(token.substr(9));
        } else if (token.rfind("stair", 0) == 0) {
            token = trim(token.substr(5));
        }
        std::transform(token.begin(), token.end(), token.begin(), [](unsigned char character) {
            return static_cast<char>(std::toupper(character));
        });
        if (token.size() != 1 || token[0] < 'A' || token[0] > 'Z') {
            throw std::runtime_error("Enter unsafe stairwells as comma-separated labels, e.g. A,B.");
        }
        stairs.insert(token);
    }
    return stairs;
}

std::set<std::string> validateUnsafeStairs(
    const std::set<std::string>& selected,
    const std::set<std::string>& available) {
    for (const std::string& stair : selected) {
        if (available.count(stair) == 0) {
            throw std::runtime_error("Stairwell " + stair + " is not present in sample_edges.csv.");
        }
    }
    return selected;
}

std::vector<TestCase> loadTestCases(const std::string& filename) {
    const CsvTable rows = readCsv(filename);
    const auto columns = makeColumnMap(rows);
    const std::size_t idColumn = column(columns, "case_id", filename);
    const std::size_t nameColumn = column(columns, "name", filename);
    const std::size_t fireColumn = column(columns, "fire_floors", filename);
    const std::size_t stairsColumn = column(columns, "unsafe_stairs", filename);
    std::vector<TestCase> testCases;

    for (std::size_t index = 1; index < rows.size(); ++index) {
        const std::size_t rowNumber = index + 1;
        TestCase testCase{
            integerCell(rows[index], idColumn, filename, rowNumber),
            cell(rows[index], nameColumn, filename, rowNumber),
            cell(rows[index], fireColumn, filename, rowNumber),
            cell(rows[index], stairsColumn, filename, rowNumber)
        };
        if (testCase.id < 1 || testCase.name.empty() || testCase.fireFloors.empty()) {
            throw std::runtime_error(filename + " has invalid test case data on row " +
                                     std::to_string(rowNumber));
        }
        testCases.push_back(testCase);
    }
    if (testCases.empty()) {
        throw std::runtime_error(filename + " contains no test cases");
    }
    return testCases;
}

int main(int argc, char* argv[]) {
    const std::string edgesFile = argc > 1 ? argv[1] : "sample_edges.csv";
    const std::string nodesFile = argc > 2 ? argv[2] : "sample_nodes.csv";
    const std::string occupantsFile = argc > 3 ? argv[3] : "sample_occupants.csv";
    const std::string testCasesFile = argc > 4 ? argv[4] : "testcases.txt";

    try {
        const DigitalTwin twin(edgesFile, nodesFile, occupantsFile);
        const std::vector<TestCase> testCases = loadTestCases(testCasesFile);
        std::string input;

        std::cout << "SmartTower CSV-driven evacuation logic demo\n";
        std::cout << "Loaded: " << nodesFile << ", " << edgesFile
                  << ", " << occupantsFile << ", " << testCasesFile << '\n';
        std::cout << "\nAvailable test cases:\n";
        for (const TestCase& testCase : testCases) {
            std::cout << "  " << testCase.id << ". " << testCase.name << '\n';
        }
        std::cout << "  0. Enter a custom scenario\n";
        std::cout << "Select test case (default 1): ";
        std::getline(std::cin, input);
        int selectedCase = 1;
        if (!input.empty()) {
            std::istringstream caseInput(input);
            char extraCharacter = '\0';
            if (!(caseInput >> selectedCase) || (caseInput >> extraCharacter) ||
                selectedCase < 0) {
                throw std::runtime_error("Select a listed test case number, or 0 for a custom scenario.");
            }
        }

        const std::set<std::string> availableStairs = twin.availableStairs();
        std::cout << "Stairwells in edge data: ";
        for (const std::string& stair : availableStairs) {
            std::cout << stair << ' ';
        }
        std::set<int> fireFloors;
        std::set<std::string> unsafeStairs;

        if (selectedCase == 0) {
            std::cout << "\nEnter fire floor(s), comma separated (available "
                      << twin.minimumFloor() << "-" << twin.maximumFloor() << "): ";
            std::getline(std::cin, input);
            fireFloors = parseFireFloors(input, twin);
            std::cout << "Enter unsafe stairwells near the incident (comma separated): ";
            std::getline(std::cin, input);
            unsafeStairs = validateUnsafeStairs(parseUnsafeStairs(input), availableStairs);
        } else {
            const auto selected = std::find_if(testCases.begin(), testCases.end(),
                [selectedCase](const TestCase& testCase) {
                    return testCase.id == selectedCase;
                });
            if (selected == testCases.end()) {
                throw std::runtime_error("That test case number is not in " + testCasesFile);
            }
            fireFloors = parseFireFloors(selected->fireFloors, twin);
            unsafeStairs = validateUnsafeStairs(
                parseUnsafeStairs(selected->unsafeStairs), availableStairs);
            std::cout << "\nRunning test case " << selected->id << ": "
                      << selected->name << '\n';
        }

        twin.showResponse(fireFloors, unsafeStairs);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Digital twin error: " << error.what() << '\n';
        return 1;
    }
}
