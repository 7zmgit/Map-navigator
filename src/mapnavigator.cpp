#include "mapnavigator.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>


    // Helper: Split tab-separated lines

    static void split_tabs(const std::string& line,
                           DynamicArray<std::string>& out) {
    out.clear();
    std::string cur;
    for (size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];
        if (ch == '\t') {
            out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(ch);
        }
    }
    out.push_back(cur);
}


Graph::Graph() {}

Graph::~Graph() {
    for (int i = 0; i < cities_.size(); ++i) {
        delete[] cities_[i].name;
        cities_[i].name = nullptr;
    }
}

int Graph::add_city(const char* nm) {
    City c;
    c.id = cities_.size();
    int L = (nm ? (int) std::strlen(nm) : 0);
    c.name = new char[L + 1];
    if (nm) std::strncpy(c.name, nm, L + 1);
    else c.name[0] = '\0';
    cities_.push_back(c);
    adj_.push_back(DynamicArray<Edge>()); // empty adjacency list for new city
    return c.id;
}

void Graph::add_directed_edge(int u, int v, const DynamicArray<double>& wts) {
    if (u < 0 || v < 0 || u >= num_cities() || v >= num_cities()) return;
    Edge e;
    e.to = v;
    e.weights = wts;
    adj_[u].push_back(e);
}

void Graph::add_undirected_edge(int u, int v, const DynamicArray<double>& wts) {
    add_directed_edge(u, v, wts);
    add_directed_edge(v, u, wts);
}

int Graph::find_city_id_by_name(const char* nm) const {
    for (int i = 0; i < cities_.size(); ++i) {
        if (std::strcmp(cities_[i].name, nm) == 0) return i;
    }
    return -1;
}

void Graph::load_from_file(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error opening file: " << filename << "\n";
        return;
    }

    // clear existing graph
    for (int i = 0; i < cities_.size(); ++i) {
        delete[] cities_[i].name;
        cities_[i].name = nullptr;
    }
    cities_.clear();
    adj_.clear();
    weightNames_.clear();

    std::string headerLine;
    if (!std::getline(file, headerLine)) {
        std::cout << "Empty file or read error.\n";
        file.close();
        return;
    }

    DynamicArray<std::string> tokens;
    split_tabs(headerLine, tokens);

    if (tokens.size() < 3) {
        std::cout << "Expected at least 3 columns (City1, City2, weight1,...).\n";
        file.close();
        return;
    }

    for (int i = 2; i < tokens.size(); ++i) weightNames_.push_back(tokens[i]);

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        split_tabs(line, tokens);
        if (tokens.size() < 3) continue;

        const std::string& city1Str = tokens[0];
        const std::string& city2Str = tokens[1];

        int numW = tokens.size() - 2;
        if (numW != weightNames_.size()) continue;

        DynamicArray<double> wts(numW);
        for (int k = 0; k < numW; ++k) {
            const std::string& wStr = tokens[2 + k];
            wts[k] = std::atof(wStr.c_str());
        }

        int i = find_city_id_by_name(city1Str.c_str());
        if (i == -1) i = add_city(city1Str.c_str());
        int j = find_city_id_by_name(city2Str.c_str());
        if (j == -1) j = add_city(city2Str.c_str());

        add_undirected_edge(i, j, wts);
    }

    file.close();
}

// Dijkstra implementation

static void reconstruct_path(const DynamicArray<int>& parent,
                             int src, int dst,
                             PathResult& out) {
    out.nodes.clear();
    if (src == -1 || dst == -1) { out.found = false; return; }

    DynamicArray<int> rev;
    int cur = dst;
    while (cur != -1) {
        rev.push_back(cur);
        if (cur == src) break;
        cur = parent[cur];
    }

    if (rev.size() == 0 || rev[rev.size() - 1] != src) {
        out.found = false;
        return;
    }

    for (int i = rev.size() - 1; i >= 0; --i) out.nodes.push_back(rev[i]);
    out.found = true;
}

PathResult dijkstra(const Graph& g, int src, int dst, int weightIndex) {
    const int n = g.num_cities();
    const double INF = 1e300;

    PathResult result;
    result.found = false;
    result.total_cost = INF;
    result.nodes.clear();

    if (src < 0 || src >= n) return result;
    if (dst != -1 && (dst < 0 || dst >= n)) return result;
    if (g.num_weights() <= 0) return result;
    if (weightIndex < 0 || weightIndex >= g.num_weights()) return result;

    DynamicArray<double> dist(n);
    DynamicArray<int> parent(n);
    for (int i = 0; i < n; ++i) { dist[i] = INF; parent[i] = -1; }
    dist[src] = 0.0;

    MinBinaryHeap<double, int> pq;
    pq.push(0.0, src);

    while (!pq.empty()) {
        auto it = pq.top(); pq.pop();
        double d = it.key;
        int u = it.val;

        if (d > dist[u]) continue;
        if (dst != -1 && u == dst) break;

        const DynamicArray<Edge>& N = g.neighbors(u);
        for (int i = 0; i < N.size(); ++i) {
            const Edge& e = N[i];
            int v = e.to;
            if (weightIndex >= e.weights.size()) continue;
            double w = e.weights[weightIndex];
            if (w < 0) continue;
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                parent[v] = u;
                pq.push(dist[v], v);
            }
        }
    }

    if (dst == -1) {
        result.found = true;
        result.total_cost = 0.0;
        result.nodes.push_back(src);
        return result;
    }

    if (dist[dst] == INF) {
        result.found = false;
        return result;
    }

    result.total_cost = dist[dst];
    reconstruct_path(parent, src, dst, result);
    return result;
}


// DSU for Kruskal

struct DSU {
    DynamicArray<int> parent;
    DynamicArray<int> rank;
    DSU(int n = 0) {
        parent.resize(n);
        rank.resize(n);
        for (int i = 0; i < n; ++i) { parent[i] = i; rank[i] = 0; }
    }
    int find(int x) {
        if (parent[x] != x) parent[x] = find(parent[x]);
        return parent[x];
    }
    bool unite(int a, int b) {
        a = find(a); b = find(b);
        if (a == b) return false;
        if (rank[a] < rank[b]) {
            parent[a] = b;
        } else if (rank[a] > rank[b]) {
            parent[b] = a;
        } else {
            parent[b] = a;
            rank[a] = rank[a] + 1;
        }
        return true;
    }
};

// Kruskal MST

DynamicArray<FullEdge> kruskal_mst(const Graph& g, int weightIndex) {
    DynamicArray<FullEdge> out;
    int n = g.num_cities();
    if (n == 0) return out;
    if (g.num_weights() == 0) return out;
    if (weightIndex < 0 || weightIndex >= g.num_weights()) return out;

    // collect undirected edges (u < v)
    DynamicArray<FullEdge> edges;

    for (int u = 0; u < n; ++u) {
        const DynamicArray<Edge>& nbr = g.neighbors(u);
        for (int i = 0; i < nbr.size(); ++i) {
            int v = nbr[i].to;
            if (u < v) {
                if (weightIndex >= nbr[i].weights.size()) continue;
                FullEdge fe;
                fe.u = u;
                fe.v = v;
                fe.w = nbr[i].weights[weightIndex];
                edges.push_back(fe);
            }
        }
    }


    int m = edges.size();
    for (int i = 0; i < m; ++i) {
        int min_idx = i;
        for (int j = i + 1; j < m; ++j) {
            if (edges[j].w < edges[min_idx].w) min_idx = j;
        }
        if (min_idx != i) {
            FullEdge tmp = edges[i];
            edges[i] = edges[min_idx];
            edges[min_idx] = tmp;
        }
    }

    DSU dsu(n);
    for (int i = 0; i < m; ++i) {
        if (dsu.unite(edges[i].u, edges[i].v)) {
            out.push_back(edges[i]);
            if (out.size() == n - 1) break;
        }
    }

    return out;
}

// Prim MST

DynamicArray<FullEdge> prim_mst(const Graph& g, int weightIndex) {
    DynamicArray<FullEdge> out;
    int n = g.num_cities();
    if (n == 0) return out;
    if (g.num_weights() == 0) return out;
    if (weightIndex < 0 || weightIndex >= g.num_weights()) return out;

    DynamicArray<double> key(n);
    DynamicArray<int> parent(n);
    DynamicArray<bool> inMST(n);

    for (int i = 0; i < n; ++i) { key[i] = 1e300; parent[i] = -1; inMST[i] = false; }
    key[0] = 0.0;

    MinBinaryHeap<double,int> pq;
    pq.push(0.0, 0);

    while (!pq.empty()) {
        auto it = pq.top(); pq.pop();
        int u = it.val;
        if (inMST[u]) continue;
        inMST[u] = true;

        if (parent[u] != -1) {
            FullEdge e; e.u = parent[u]; e.v = u; e.w = key[u];
            out.push_back(e);
        }

        const DynamicArray<Edge>& nbr = g.neighbors(u);
        for (int i = 0; i < nbr.size(); ++i) {
            int v = nbr[i].to;
            if (weightIndex >= nbr[i].weights.size()) continue;
            double w = nbr[i].weights[weightIndex];
            if (!inMST[v] && w < key[v]) {
                key[v] = w;
                parent[v] = u;
                pq.push(w, v);
            }
        }
    }

    return out;
}

// Build adjacency from MST

DynamicArray< DynamicArray<FullEdge> >
build_mst_adj(const DynamicArray<FullEdge>& mst, int n) {
    DynamicArray< DynamicArray<FullEdge> > adj;
    adj.resize(n);
    for (int i = 0; i < mst.size(); ++i) {
        FullEdge e = mst[i];
        adj[e.u].push_back(e);
        FullEdge rev = e;
        // swap endpoints manually
        int tmpu = rev.u;
        rev.u = rev.v;
        rev.v = tmpu;
        adj[rev.u].push_back(rev);
    }
    return adj;
}

// DFS to find path inside MST adjacency

bool dfs_mst(int u, int dst,
             const DynamicArray< DynamicArray<FullEdge> >& adj,
             DynamicArray<int>& path,
             DynamicArray<bool>& vis) {
    if (u == dst) { path.push_back(u); return true; }
    vis[u] = true;
    for (int i = 0; i < adj[u].size(); ++i) {
        int v = adj[u][i].v;
        if (!vis[v]) {
            if (dfs_mst(v, dst, adj, path, vis)) {
                path.push_back(u);
                return true;
            }
        }
    }
    return false;
}


// minimum_spanning_path: choose Prim or Kruskal based on density

PathResult minimum_spanning_path(const Graph& g,
                                 int src, int dst,
                                 int weightIndex) {
    PathResult result;
    result.found = false;
    result.total_cost = 0.0;
    result.nodes.clear();

    int n = g.num_cities();
    if (n == 0) return result;
    if (src < 0 || src >= n || dst < 0 || dst >= n) return result;
    if (g.num_weights() == 0) return result;
    if (weightIndex < 0 || weightIndex >= g.num_weights()) return result;

    long long edgeCount = 0;
    for (int u = 0; u < n; ++u) edgeCount += g.neighbors(u).size();
    edgeCount /= 2;

    double density = 0.0;
    if (n > 1) density = (2.0 * edgeCount) / (n * (n - 1));

    DynamicArray<FullEdge> mst;
    if (density > 0.5) {
        mst = prim_mst(g, weightIndex);
    } else {
        mst = kruskal_mst(g, weightIndex);
    }

    if (mst.size() != n - 1) {
        result.found = false;
        return result;
    }

    DynamicArray< DynamicArray<FullEdge> > mstAdj = build_mst_adj(mst, n);

    DynamicArray<int> path;
    DynamicArray<bool> vis(n);
    for (int i = 0; i < n; ++i) vis[i] = false;

    if (!dfs_mst(src, dst, mstAdj, path, vis)) {
        result.found = false;
        return result;
    }


    for (int i = path.size() - 1; i >= 0; --i) result.nodes.push_back(path[i]);

    double cost = 0.0;
    for (int i = 0; i + 1 < result.nodes.size(); ++i) {
        int u = result.nodes[i];
        int v = result.nodes[i + 1];
        bool found = false;
        for (int k = 0; k < mstAdj[u].size(); ++k) {
            if (mstAdj[u][k].v == v) {
                cost += mstAdj[u][k].w;
                found = true;
                break;
            }
        }
        if (!found) {

        }
    }

    result.total_cost = cost;
    result.found = true;
    return result;
}



