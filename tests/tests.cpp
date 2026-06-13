// Unit tests for the MapNavigator graph engine.
//
// Build & run (from project root):
//   g++ -std=c++17 tests/tests.cpp src/mapnavigator.cpp -o tests/run_tests && tests/run_tests

#include "mapnavigator.h"
#include <cstdio>
#include <cmath>
#include <cstring>


static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;
static const char* current_test = "";

#define TEST(name) \
    static void test_##name(); \
    struct _reg_##name { _reg_##name() { \
        ++tests_run; \
        current_test = #name; \
        printf("  %-50s", #name); \
        test_##name(); \
        if (true) { ++tests_passed; printf("PASS\n"); } \
    }} _inst_##name; \
    static void test_##name()

#define ASSERT(cond) \
    do { if (!(cond)) { \
        ++tests_failed; --tests_passed; \
        printf("FAIL\n    Assertion failed: %s\n    at %s:%d\n", #cond, __FILE__, __LINE__); \
        return; \
    }} while(0)

#define ASSERT_EQ(a, b) \
    do { if ((a) != (b)) { \
        ++tests_failed; --tests_passed; \
        printf("FAIL\n    Expected %s == %s\n    at %s:%d\n", #a, #b, __FILE__, __LINE__); \
        return; \
    }} while(0)

#define ASSERT_NEAR(a, b, eps) \
    do { if (std::fabs((double)(a) - (double)(b)) > (eps)) { \
        ++tests_failed; --tests_passed; \
        printf("FAIL\n    Expected |%s - %s| <= %g, got |%g - %g| = %g\n    at %s:%d\n", \
               #a, #b, (double)(eps), (double)(a), (double)(b), \
               std::fabs((double)(a)-(double)(b)), __FILE__, __LINE__); \
        return; \
    }} while(0)

#define ASSERT_STR_EQ(a, b) \
    do { if (std::strcmp((a),(b)) != 0) { \
        ++tests_failed; --tests_passed; \
        printf("FAIL\n    Expected \"%s\" == \"%s\"\n    at %s:%d\n", (a),(b),__FILE__,__LINE__); \
        return; \
    }} while(0)

// ---------------------------------------------------------------------------
// ── DynamicArray ────────────────────────────────────────────────────────────
// ---------------------------------------------------------------------------

TEST(dynamic_array_push_and_size) {
    DynamicArray<int> a;
    ASSERT_EQ(a.size(), 0);
    a.push_back(10);
    a.push_back(20);
    a.push_back(30);
    ASSERT_EQ(a.size(), 3);
    ASSERT_EQ(a[0], 10);
    ASSERT_EQ(a[1], 20);
    ASSERT_EQ(a[2], 30);
}

TEST(dynamic_array_pop_back) {
    DynamicArray<int> a;
    a.push_back(1); a.push_back(2); a.push_back(3);
    a.pop_back();
    ASSERT_EQ(a.size(), 2);
    ASSERT_EQ(a[1], 2);
}

TEST(dynamic_array_clear) {
    DynamicArray<int> a;
    for (int i = 0; i < 10; ++i) a.push_back(i);
    a.clear();
    ASSERT_EQ(a.size(), 0);
}

TEST(dynamic_array_resize_grows) {
    DynamicArray<double> a;
    a.resize(5);
    ASSERT_EQ(a.size(), 5);
    // default-initialised to 0.0
    for (int i = 0; i < 5; ++i) ASSERT_EQ(a[i], 0.0);
}

TEST(dynamic_array_resize_shrinks) {
    DynamicArray<int> a;
    for (int i = 0; i < 8; ++i) a.push_back(i);
    a.resize(3);
    ASSERT_EQ(a.size(), 3);
    ASSERT_EQ(a[2], 2);
}

TEST(dynamic_array_copy_constructor) {
    DynamicArray<int> a;
    a.push_back(7); a.push_back(8); a.push_back(9);
    DynamicArray<int> b(a);
    ASSERT_EQ(b.size(), 3);
    ASSERT_EQ(b[0], 7);
    // Mutating b must not affect a
    b[0] = 99;
    ASSERT_EQ(a[0], 7);
}

TEST(dynamic_array_assignment_operator) {
    DynamicArray<int> a;
    a.push_back(1); a.push_back(2);
    DynamicArray<int> b;
    b.push_back(99);
    b = a;
    ASSERT_EQ(b.size(), 2);
    ASSERT_EQ(b[0], 1);
    b[0] = 0;
    ASSERT_EQ(a[0], 1); // deep copy
}

TEST(dynamic_array_grows_beyond_initial_capacity) {
    DynamicArray<int> a;
    // Initial capacity is 0, grows by doubling; push 100 items
    for (int i = 0; i < 100; ++i) a.push_back(i);
    ASSERT_EQ(a.size(), 100);
    ASSERT_EQ(a[99], 99);
}

TEST(dynamic_array_self_assignment) {
    DynamicArray<int> a;
    a.push_back(42);
    a = a; // must not crash or corrupt
    ASSERT_EQ(a.size(), 1);
    ASSERT_EQ(a[0], 42);
}

TEST(heap_push_pop_order) {
    MinBinaryHeap<double, int> h;
    h.push(5.0, 5);
    h.push(1.0, 1);
    h.push(3.0, 3);
    h.push(2.0, 2);
    h.push(4.0, 4);
    // should pop in ascending key order
    for (int expected = 1; expected <= 5; ++expected) {
        ASSERT(!h.empty());
        ASSERT_EQ(h.top().val, expected);
        h.pop();
    }
    ASSERT(h.empty());
}

TEST(heap_single_element) {
    MinBinaryHeap<int, int> h;
    h.push(42, 99);
    ASSERT_EQ(h.size(), 1);
    ASSERT_EQ(h.top().key, 42);
    ASSERT_EQ(h.top().val, 99);
    h.pop();
    ASSERT(h.empty());
}

TEST(heap_duplicate_keys) {
    MinBinaryHeap<double, int> h;
    h.push(3.0, 10);
    h.push(3.0, 20);
    h.push(1.0, 30);
    ASSERT_EQ(h.top().val, 30);
    h.pop();
    ASSERT_NEAR(h.top().key, 3.0, 1e-9);
}

TEST(heap_large_input) {
    MinBinaryHeap<int, int> h;
    for (int i = 1000; i >= 1; --i) h.push(i, i);
    ASSERT_EQ(h.size(), 1000);
    int prev = 0;
    while (!h.empty()) {
        int k = h.top().key; h.pop();
        ASSERT(k >= prev);
        prev = k;
    }
}

TEST(graph_add_cities) {
    Graph g;
    g.add_city("Alpha");
    g.add_city("Beta");
    g.add_city("Gamma");
    ASSERT_EQ(g.num_cities(), 3);
    ASSERT_STR_EQ(g.city(0).name, "Alpha");
    ASSERT_STR_EQ(g.city(1).name, "Beta");
    ASSERT_STR_EQ(g.city(2).name, "Gamma");
}

TEST(graph_find_city_by_name) {
    Graph g;
    g.add_city("Cairo");
    g.add_city("Aswan");
    ASSERT_EQ(g.find_city_id_by_name("Cairo"), 0);
    ASSERT_EQ(g.find_city_id_by_name("Aswan"), 1);
    ASSERT_EQ(g.find_city_id_by_name("Atlantis"), -1);
}

TEST(graph_undirected_edge_stored_both_ways) {
    Graph g;
    g.add_city("A"); g.add_city("B");
    DynamicArray<double> w(1); w[0] = 10.0;
    g.add_undirected_edge(0, 1, w);
    ASSERT_EQ(g.neighbors(0).size(), 1);
    ASSERT_EQ(g.neighbors(0)[0].to, 1);
    ASSERT_NEAR(g.neighbors(0)[0].weights[0], 10.0, 1e-9);
    ASSERT_EQ(g.neighbors(1).size(), 1);
    ASSERT_EQ(g.neighbors(1)[0].to, 0);
    ASSERT_NEAR(g.neighbors(1)[0].weights[0], 10.0, 1e-9);
}

TEST(graph_out_of_range_edge_ignored) {
    Graph g;
    g.add_city("A");
    DynamicArray<double> w(1); w[0] = 5.0;
    g.add_directed_edge(0, 99, w); // node 99 doesn't exist
    ASSERT_EQ(g.neighbors(0).size(), 0);
}

TEST(graph_load_from_tsv_file) {
    const char* path = "/tmp/test_graph.tsv";
    FILE* f = fopen(path, "w");
    fprintf(f, "City1\tCity2\tdist\ttime\n");
    fprintf(f, "X\tY\t100\t60\n");
    fprintf(f, "Y\tZ\t200\t90\n");
    fprintf(f, "X\tZ\t400\t200\n");
    fclose(f);

    Graph g;
    g.load_from_file(path);

    ASSERT_EQ(g.num_cities(), 3);
    ASSERT_EQ(g.num_weights(), 2);
    ASSERT_EQ(g.find_city_id_by_name("X"), 0);
    ASSERT_EQ(g.find_city_id_by_name("Y"), 1);
    ASSERT_EQ(g.find_city_id_by_name("Z"), 2);
}

TEST(graph_load_skips_malformed_rows) {
    const char* path = "/tmp/test_malformed.tsv";
    FILE* f = fopen(path, "w");
    fprintf(f, "City1\tCity2\tdist\n");
    fprintf(f, "A\tB\t50\n");
    fprintf(f, "C\n");
    fprintf(f, "D\tE\t30\n");
    fclose(f);

    Graph g;
    g.load_from_file(path);
    ASSERT_EQ(g.num_cities(), 4);
}

static Graph* make_dijkstra_graph() {
    const char* path = "/tmp/dijk_test.tsv";
    FILE* f = fopen(path, "w");
    fprintf(f, "City1\tCity2\tdistance\ttime\n");
    fprintf(f, "A\tB\t10\t15\n");
    fprintf(f, "B\tC\t20\t25\n");
    fprintf(f, "A\tC\t50\t40\n");
    fprintf(f, "C\tD\t5\t10\n");
    fclose(f);
    Graph* g = new Graph();
    g->load_from_file(path);
    return g;
}

TEST(dijkstra_direct_edge) {
    Graph* gp = make_dijkstra_graph(); Graph& g = *gp;
    PathResult r = dijkstra(g, 0, 1, 0);
    ASSERT(r.found);
    ASSERT_NEAR(r.total_cost, 10.0, 1e-9);
    ASSERT_EQ(r.nodes.size(), 2);
    ASSERT_EQ(r.nodes[0], 0);
    ASSERT_EQ(r.nodes[1], 1);
}

TEST(dijkstra_shortest_by_distance) {
    Graph* gp = make_dijkstra_graph(); Graph& g = *gp;
    PathResult r = dijkstra(g, 0, 2, 0);
    ASSERT(r.found);
    ASSERT_NEAR(r.total_cost, 30.0, 1e-9);
    ASSERT_EQ(r.nodes.size(), 3);
    ASSERT_EQ(r.nodes[0], 0);
    ASSERT_EQ(r.nodes[1], 1);
    ASSERT_EQ(r.nodes[2], 2);
}

TEST(dijkstra_shortest_by_time_different_path) {
    Graph* gp = make_dijkstra_graph(); Graph& g = *gp;
    PathResult r = dijkstra(g, 0, 2, 1);
    ASSERT(r.found);
    ASSERT_NEAR(r.total_cost, 40.0, 1e-9);
}

TEST(dijkstra_multi_hop_path) {
    Graph* gp = make_dijkstra_graph(); Graph& g = *gp;
    PathResult r = dijkstra(g, 0, 3, 0);
    ASSERT(r.found);
    ASSERT_NEAR(r.total_cost, 35.0, 1e-9);
    ASSERT_EQ(r.nodes.size(), 4);
    ASSERT_EQ(r.nodes[0], 0);
    ASSERT_EQ(r.nodes[3], 3);
}

TEST(dijkstra_same_src_dst) {
    Graph* gp = make_dijkstra_graph(); Graph& g = *gp;
    PathResult r = dijkstra(g, 0, 0, 0);
    ASSERT(r.found);
    ASSERT_NEAR(r.total_cost, 0.0, 1e-9);
}

TEST(dijkstra_unreachable_node) {
    Graph g;
    g.add_city("A"); g.add_city("B"); g.add_city("Island");
    DynamicArray<double> w(1); w[0] = 5.0;
    g.add_undirected_edge(0, 1, w);
    PathResult r = dijkstra(g, 0, 2, 0);
    ASSERT(!r.found);
}

TEST(dijkstra_invalid_weight_index) {
    Graph* gp = make_dijkstra_graph(); Graph& g = *gp;
    PathResult r = dijkstra(g, 0, 3, 99);
    ASSERT(!r.found);
}

TEST(dijkstra_path_nodes_in_order) {
    Graph* gp = make_dijkstra_graph(); Graph& g = *gp;
    PathResult r = dijkstra(g, 0, 3, 0);
    ASSERT(r.found);
    ASSERT_EQ(r.nodes[0], 0);
    ASSERT_EQ(r.nodes[r.nodes.size()-1], 3);
}

TEST(dijkstra_symmetry) {
    Graph* gp = make_dijkstra_graph(); Graph& g = *gp;
    PathResult fwd = dijkstra(g, 0, 3, 0);
    PathResult rev = dijkstra(g, 3, 0, 0);
    ASSERT(fwd.found && rev.found);
    ASSERT_NEAR(fwd.total_cost, rev.total_cost, 1e-9);
}

static Graph* make_mst_graph() {
    const char* path = "/tmp/mst_test.tsv";
    FILE* f = fopen(path, "w");
    fprintf(f, "City1\tCity2\tweight\n");
    fprintf(f, "A\tB\t1\n");
    fprintf(f, "B\tC\t2\n");
    fprintf(f, "A\tC\t3\n");
    fprintf(f, "A\tD\t4\n");
    fprintf(f, "C\tD\t5\n");
    fclose(f);
    Graph* g = new Graph();
    g->load_from_file(path);
    return g;
}

TEST(kruskal_edge_count) {
    Graph* gp = make_mst_graph(); Graph& g = *gp;
    auto mst = kruskal_mst(g, 0);
    ASSERT_EQ(mst.size(), 3);
}

TEST(kruskal_total_cost) {
    Graph* gp = make_mst_graph(); Graph& g = *gp;
    auto mst = kruskal_mst(g, 0);
    double total = 0;
    for (int i = 0; i < mst.size(); ++i) total += mst[i].w;
    ASSERT_NEAR(total, 7.0, 1e-9);
}

TEST(kruskal_no_cycles) {
    Graph* gp = make_mst_graph(); Graph& g = *gp;
    auto mst = kruskal_mst(g, 0);

    bool adj[4][4] = {};
    for (int i = 0; i < mst.size(); ++i) {
        adj[mst[i].u][mst[i].v] = true;
        adj[mst[i].v][mst[i].u] = true;
    }

    bool visited[4] = {};
    int queue[4]; int head=0, tail=0;
    queue[tail++] = 0; visited[0] = true;
    while (head < tail) {
        int u = queue[head++];
        for (int v = 0; v < 4; ++v)
            if (adj[u][v] && !visited[v]) { visited[v]=true; queue[tail++]=v; }
    }
    for (int i = 0; i < 4; ++i) ASSERT(visited[i]);
}

TEST(kruskal_empty_graph) {
    Graph g;
    auto mst = kruskal_mst(g, 0);
    ASSERT_EQ(mst.size(), 0);
}

TEST(kruskal_two_nodes) {
    FILE* f = fopen("/tmp/k2.tsv","w");
    fprintf(f,"City1\tCity2\tweight\nX\tY\t7\n"); fclose(f);
    Graph g; g.load_from_file("/tmp/k2.tsv");
    auto mst = kruskal_mst(g, 0);
    ASSERT_EQ(mst.size(), 1);
    ASSERT_NEAR(mst[0].w, 7.0, 1e-9);
}


TEST(prim_edge_count) {
    Graph* gp = make_mst_graph(); Graph& g = *gp;
    auto mst = prim_mst(g, 0);
    ASSERT_EQ(mst.size(), 3);
}

TEST(prim_total_cost) {
    Graph* gp = make_mst_graph(); Graph& g = *gp;
    auto mst = prim_mst(g, 0);
    double total = 0;
    for (int i = 0; i < mst.size(); ++i) total += mst[i].w;
    ASSERT_NEAR(total, 7.0, 1e-9);
}

TEST(prim_kruskal_same_cost) {
    // On the same graph both algorithms must produce equal MST cost
    // (MST cost is unique even if edge sets differ when weights are distinct)
    Graph* gp = make_mst_graph(); Graph& g = *gp;
    auto k = kruskal_mst(g, 0);
    auto p = prim_mst(g, 0);
    double kc = 0, pc = 0;
    for (int i = 0; i < k.size(); ++i) kc += k[i].w;
    for (int i = 0; i < p.size(); ++i) pc += p[i].w;
    ASSERT_NEAR(kc, pc, 1e-9);
}

TEST(prim_empty_graph) {
    Graph g;
    auto mst = prim_mst(g, 0);
    ASSERT_EQ(mst.size(), 0);
}

TEST(prim_two_nodes) {
    FILE* f = fopen("/tmp/p2.tsv","w");
    fprintf(f,"City1\tCity2\tweight\nX\tY\t9\n"); fclose(f);
    Graph g; g.load_from_file("/tmp/p2.tsv");
    auto mst = prim_mst(g, 0);
    ASSERT_EQ(mst.size(), 1);
    ASSERT_NEAR(mst[0].w, 9.0, 1e-9);
}

TEST(dijkstra_picks_different_path_per_weight) {

    const char* path = "/tmp/multiw_test.tsv";
    FILE* ff = fopen(path, "w");
    fprintf(ff, "City1\tCity2\tdistance\ttime\n");
    fprintf(ff, "A\tB\t10\t100\n");
    fprintf(ff, "A\tC\t50\t1\n");
    fprintf(ff, "B\tC\t1\t1\n");
    fclose(ff);
    Graph gobj; gobj.load_from_file(path);
    Graph& g = gobj;

    PathResult by_dist = dijkstra(g, 0, 2, 0);
    PathResult by_time = dijkstra(g, 0, 2, 1);

    ASSERT(by_dist.found && by_time.found);
    ASSERT_NEAR(by_dist.total_cost, 11.0, 1e-9);
    ASSERT_NEAR(by_time.total_cost,  1.0, 1e-9);

    ASSERT(by_dist.nodes.size() != by_time.nodes.size());
}

TEST(mst_weight_index_1_used_correctly) {

    const char* path = "/tmp/mstw_test.tsv";
    FILE* ff = fopen(path, "w");
    fprintf(ff, "City1\tCity2\tw0\tw1\n");
    fprintf(ff, "A\tB\t1\t100\n");
    fprintf(ff, "A\tC\t5\t1\n");
    fprintf(ff, "B\tC\t5\t1\n");
    fclose(ff);
    Graph g; g.load_from_file(path);

    auto k0 = kruskal_mst(g, 0);
    auto k1 = kruskal_mst(g, 1);

    double c0=0, c1=0;
    for (int i=0;i<k0.size();++i) c0+=k0[i].w;
    for (int i=0;i<k1.size();++i) c1+=k1[i].w;
    ASSERT_NEAR(c0, 6.0, 1e-9);
    ASSERT_NEAR(c1, 2.0, 1e-9);
}

TEST(egypt_dataset_loads_27_cities) {
    Graph g;
    g.load_from_file("../data/egypt_cities.tsv");
    ASSERT_EQ(g.num_cities(), 27);
    ASSERT_EQ(g.num_weights(), 3);
}

TEST(egypt_cairo_to_aswan_path_exists) {
    Graph g;
    g.load_from_file("../data/egypt_cities.tsv");
    int src = g.find_city_id_by_name("Cairo");
    int dst = g.find_city_id_by_name("Aswan");
    ASSERT(src != -1 && dst != -1);
    PathResult r = dijkstra(g, src, dst, 0);
    ASSERT(r.found);
    ASSERT(r.total_cost > 0);
    ASSERT(r.nodes.size() >= 2);
    ASSERT_EQ(r.nodes[0], src);
    ASSERT_EQ(r.nodes[r.nodes.size()-1], dst);
}

TEST(egypt_cairo_to_aswan_reasonable_distance) {
    Graph g;
    g.load_from_file("../data/egypt_cities.tsv");
    PathResult r = dijkstra(g,
        g.find_city_id_by_name("Cairo"),
        g.find_city_id_by_name("Aswan"), 0);
    ASSERT(r.total_cost >= 700 && r.total_cost <= 1100);
}

TEST(egypt_mst_has_26_edges) {
    Graph g;
    g.load_from_file("../data/egypt_cities.tsv");
    auto mst = kruskal_mst(g, 0);
    ASSERT_EQ(mst.size(), 26);
}

TEST(egypt_all_cities_reachable_from_cairo) {
    Graph g;
    g.load_from_file("../data/egypt_cities.tsv");
    int n = g.num_cities();
    int cairo = g.find_city_id_by_name("Cairo");
    ASSERT(cairo != -1);
    int reachable = 0;
    for (int i = 0; i < n; ++i) {
        if (i == cairo) { ++reachable; continue; }
        PathResult r = dijkstra(g, cairo, i, 0);
        if (r.found) ++reachable;
    }
    ASSERT_EQ(reachable, 27);
}

int main() {
    printf("\nMapNavigator Engine Unit Tests\n");
    printf("Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0)
        printf(", %d Failed", tests_failed);
    printf("\n");
    return tests_failed > 0 ? 1 : 0;
}
