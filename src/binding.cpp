// All functions return JSON strings via std::string so they can be passed to JavaScript using EM_ASM / ccall.
// This keeps the binding layer thin and avoids depending on embind.

#include "mapnavigator.h"
#include <emscripten/emscripten.h>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstring>

// One Graph instance lives for the page lifetime. JS calls load_graph() once
// at startup, then any number of find_path / find_mst calls.

static Graph g_graph;
static bool  g_loaded = false;

// Static buffer for return strings. Emscripten passes back C strings, so we
// need stable storage that lives until the next call. Using std::string with
// a static lifetime guarantees the .c_str() pointer stays valid long enough
// for JS to copy it out.
static std::string g_return_buffer;

// The original engine avoids the STL containers in its public
// API, but in the binding layer we use std::string freely

static void json_escape(const std::string& s, std::string& out) {
    out.push_back('"');
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out.push_back(c);
                }
        }
    }
    out.push_back('"');
}

static void json_number(double v, std::string& out) {
    char buf[64];
    // %g gives compact output (e.g. 220 not 220.000000), still is parsed as JSON
    std::snprintf(buf, sizeof(buf), "%g", v);
    out += buf;
}

extern "C" {

// Load graph from a TSV string passed in from JS.
// The Graph::load_from_file API takes a filename, so we write the TSV to a
// temp file in Emscripten's MEMFS and let the existing parser handle it.
EMSCRIPTEN_KEEPALIVE
const char* load_graph(const char* tsv_content) {
    g_return_buffer.clear();

    if (!tsv_content) {
        g_return_buffer = "{\"ok\":false,\"error\":\"null input\"}";
        return g_return_buffer.c_str();
    }

    const char* tmp_path = "/tmp_graph.tsv";
    {
        std::ofstream f(tmp_path);
        if (!f.is_open()) {
            g_return_buffer = "{\"ok\":false,\"error\":\"could not write temp file\"}";
            return g_return_buffer.c_str();
        }
        f << tsv_content;
    }

    g_graph.load_from_file(tmp_path);
    int n = g_graph.num_cities();
    int w = g_graph.num_weights();

    if (n == 0) {
        g_loaded = false;
        g_return_buffer = "{\"ok\":false,\"error\":\"no cities parsed\"}";
        return g_return_buffer.c_str();
    }

    g_loaded = true;

    g_return_buffer = "{\"ok\":true,\"num_cities\":";
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", n);
    g_return_buffer += buf;
    g_return_buffer += ",\"num_weights\":";
    std::snprintf(buf, sizeof(buf), "%d", w);
    g_return_buffer += buf;
    g_return_buffer += ",\"weight_names\":[";
    for (int i = 0; i < w; ++i) {
        if (i) g_return_buffer += ",";
        json_escape(g_graph.weight_name(i), g_return_buffer);
    }
    g_return_buffer += "]}";

    return g_return_buffer.c_str();
}

// Return JSON array of all city names in the order they were loaded.
EMSCRIPTEN_KEEPALIVE
const char* get_cities() {
    g_return_buffer.clear();

    if (!g_loaded) {
        g_return_buffer = "[]";
        return g_return_buffer.c_str();
    }

    g_return_buffer = "[";
    int n = g_graph.num_cities();
    for (int i = 0; i < n; ++i) {
        if (i) g_return_buffer += ",";
        // city().name is a char* C-string
        std::string nm(g_graph.city(i).name ? g_graph.city(i).name : "");
        json_escape(nm, g_return_buffer);
    }
    g_return_buffer += "]";

    return g_return_buffer.c_str();
}

// Find shortest path between two cities (by name) using Dijkstra.
EMSCRIPTEN_KEEPALIVE
const char* find_path(const char* src_name, const char* dst_name, int weight_index) {
    g_return_buffer.clear();

    if (!g_loaded) {
        g_return_buffer = "{\"found\":false,\"error\":\"graph not loaded\"}";
        return g_return_buffer.c_str();
    }
    if (!src_name || !dst_name) {
        g_return_buffer = "{\"found\":false,\"error\":\"null city name\"}";
        return g_return_buffer.c_str();
    }

    int src = g_graph.find_city_id_by_name(src_name);
    int dst = g_graph.find_city_id_by_name(dst_name);

    if (src == -1 || dst == -1) {
        g_return_buffer = "{\"found\":false,\"error\":\"city not found\"}";
        return g_return_buffer.c_str();
    }

    int nw = g_graph.num_weights();
    if (weight_index < 0 || weight_index >= nw) {
        g_return_buffer = "{\"found\":false,\"error\":\"invalid weight index\"}";
        return g_return_buffer.c_str();
    }

    PathResult res = dijkstra(g_graph, src, dst, weight_index);

    if (!res.found) {
        g_return_buffer = "{\"found\":false,\"error\":\"no path\"}";
        return g_return_buffer.c_str();
    }

    g_return_buffer = "{\"found\":true,\"total_cost\":";
    json_number(res.total_cost, g_return_buffer);
    g_return_buffer += ",\"nodes\":[";
    for (int i = 0; i < res.nodes.size(); ++i) {
        if (i) g_return_buffer += ",";
        std::string nm(g_graph.city(res.nodes[i]).name ? g_graph.city(res.nodes[i]).name : "");
        json_escape(nm, g_return_buffer);
    }
    g_return_buffer += "]}";

    return g_return_buffer.c_str();
}

// Find MST. Chooses Prim or Kruskal based on graph density.
// Returns JSON: { ok, algorithm, total_cost, edges: [{u, v, w}], error? }
EMSCRIPTEN_KEEPALIVE
const char* find_mst(int weight_index) {
    g_return_buffer.clear();

    if (!g_loaded) {
        g_return_buffer = "{\"ok\":false,\"error\":\"graph not loaded\"}";
        return g_return_buffer.c_str();
    }

    int n = g_graph.num_cities();
    int nw = g_graph.num_weights();
    if (weight_index < 0 || weight_index >= nw) {
        g_return_buffer = "{\"ok\":false,\"error\":\"invalid weight index\"}";
        return g_return_buffer.c_str();
    }
    if (n == 0) {
        g_return_buffer = "{\"ok\":false,\"error\":\"empty graph\"}";
        return g_return_buffer.c_str();
    }

    // Density check (same logic as MainWindow::on_MST_clicked)
    long long edge_count = 0;
    for (int u = 0; u < n; ++u) edge_count += g_graph.neighbors(u).size();
    edge_count /= 2;

    double density = (n > 1) ? (2.0 * edge_count) / (double)(n * (n - 1)) : 0.0;

    const char* algo;
    DynamicArray<FullEdge> mst;
    if (density > 0.5) {
        algo = "prim";
        mst  = prim_mst(g_graph, weight_index);
    } else {
        algo = "kruskal";
        mst  = kruskal_mst(g_graph, weight_index);
    }

    if (mst.size() != n - 1) {
        g_return_buffer = "{\"ok\":false,\"error\":\"graph disconnected\",\"algorithm\":\"";
        g_return_buffer += algo;
        g_return_buffer += "\"}";
        return g_return_buffer.c_str();
    }

    double total = 0.0;
    for (int i = 0; i < mst.size(); ++i) total += mst[i].w;

    g_return_buffer = "{\"ok\":true,\"algorithm\":\"";
    g_return_buffer += algo;
    g_return_buffer += "\",\"total_cost\":";
    json_number(total, g_return_buffer);
    g_return_buffer += ",\"edges\":[";

    for (int i = 0; i < mst.size(); ++i) {
        if (i) g_return_buffer += ",";
        std::string un(g_graph.city(mst[i].u).name ? g_graph.city(mst[i].u).name : "");
        std::string vn(g_graph.city(mst[i].v).name ? g_graph.city(mst[i].v).name : "");
        g_return_buffer += "{\"u\":";
        json_escape(un, g_return_buffer);
        g_return_buffer += ",\"v\":";
        json_escape(vn, g_return_buffer);
        g_return_buffer += ",\"w\":";
        json_number(mst[i].w, g_return_buffer);
        g_return_buffer += "}";
    }
    g_return_buffer += "]}";

    return g_return_buffer.c_str();
}

} // extern "C"
