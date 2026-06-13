#ifndef MAP_NAVIGATOR_H
#define MAP_NAVIGATOR_H

#include <iostream>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>


template <typename T>
class DynamicArray {
public:
    DynamicArray() : data_(nullptr), sz_(0), cap_(0) {}

    DynamicArray(int n) : data_(nullptr), sz_(0), cap_(0) {
        resize(n);
    }

    // Copy constructor
    DynamicArray(const DynamicArray& other) : data_(nullptr), sz_(0), cap_(0) {
        reserve(other.sz_);
        for (int i = 0; i < other.sz_; ++i) data_[i] = other.data_[i];
        sz_ = other.sz_;
    }

    // Assignment operator
    DynamicArray& operator=(const DynamicArray& other) {
        if (this == &other) return *this;
        reserve(other.sz_);
        for (int i = 0; i < other.sz_; ++i) data_[i] = other.data_[i];
        sz_ = other.sz_;
        return *this;
    }

    ~DynamicArray() {
        delete[] data_;
        data_ = nullptr;
        sz_ = 0;
        cap_ = 0;
    }

    int size() const { return sz_; }
    bool empty() const { return sz_ == 0; }

    T& operator[](int i) { return data_[i]; }
    const T& operator[](int i) const { return data_[i]; }

    void push_back(const T& x) {
        if (sz_ == cap_) grow();
        data_[sz_++] = x;
    }

    void pop_back() {
        if (sz_ > 0) --sz_;
    }

    void clear() { sz_ = 0; }

    void reserve(int newCap) {
        if (newCap <= cap_) return;
        T* nd = new T[newCap];
        for (int i = 0; i < sz_; ++i) nd[i] = data_[i];
        delete[] data_;
        data_ = nd;
        cap_ = newCap;
    }

    void resize(int newSize) {
        if (newSize < 0) return;
        if (newSize > cap_) reserve(newSize);
        for (int i = sz_; i < newSize; ++i) data_[i] = T();
        sz_ = newSize;
    }

private:
    T* data_;
    int sz_;
    int cap_;

    void grow() {
        int newCap = (cap_ == 0 ? 2 : cap_ * 2);
        reserve(newCap);
    }
};

// ---------------------------
// MinBinaryHeap (template) using DynamicArray

template <typename Key, typename Val>
class MinBinaryHeap {
public:
    struct Item { Key key; Val val; };

    MinBinaryHeap() {}

    bool empty() const { return a_.empty(); }
    int size() const { return a_.size(); }

    void push(Key k, Val v) {
        Item it{ k, v };
        a_.push_back(it);
        heapify_up(a_.size() - 1);
    }

    Item top() const { return a_[0]; }

    void pop() {
        int n = a_.size();
        if (n == 0) return;
        if (n == 1) { a_.pop_back(); return; }

        Item tmp = a_[0];
        a_[0] = a_[n - 1];
        a_[n - 1] = tmp;
        a_.pop_back();
        heapify_down(0);
    }

private:
    DynamicArray<Item> a_;

    int parent(int i) { return (i - 1) / 2; }
    int left(int i)   { return 2 * i + 1; }
    int right(int i)  { return 2 * i + 2; }

    void heapify_up(int i) {
        while (i > 0) {
            int p = parent(i);
            if (a_[i].key < a_[p].key) {
                Item tmp = a_[i];
                a_[i] = a_[p];
                a_[p] = tmp;
                i = p;
            } else break;
        }
    }

    void heapify_down(int i) {
        while (true) {
            int n = a_.size();
            int L = left(i), R = right(i);
            int smallest = i;
            if (L < n && a_[L].key < a_[smallest].key) smallest = L;
            if (R < n && a_[R].key < a_[smallest].key) smallest = R;
            if (smallest != i) {
                Item tmp = a_[i];
                a_[i] = a_[smallest];
                a_[smallest] = tmp;
                i = smallest;
            } else break;
        }
    }
};



struct City {
    int id;
    char* name;
};

struct Edge {
    int to;
    DynamicArray<double> weights;
};

// FullEdge used by Kruskal/Prim MST
struct FullEdge {
    int u;
    int v;
    double w;
};


struct PathResult {
    bool found;
    double total_cost;
    DynamicArray<int> nodes;
};


// Graph class declaration

class Graph {
public:
    Graph();
    ~Graph();

    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;

    int add_city(const char* nm);
    void add_directed_edge(int u, int v, const DynamicArray<double>& wts);
    void add_undirected_edge(int u, int v, const DynamicArray<double>& wts);

    void load_from_file(const char* filename);

    int num_cities() const { return cities_.size(); }
    const City& city(int id) const { return cities_[id]; }
    const DynamicArray<Edge>& neighbors(int id) const { return adj_[id]; }
    int find_city_id_by_name(const char* nm) const;

    int num_weights() const { return weightNames_.size(); }
    const std::string& weight_name(int idx) const { return weightNames_[idx]; }

private:
    DynamicArray<City>               cities_;
    DynamicArray< DynamicArray<Edge> > adj_;
    DynamicArray<std::string>        weightNames_;
};



// Dijkstra
PathResult dijkstra(const Graph& g, int src, int dst, int weightIndex);

// Kruskal MST
DynamicArray<FullEdge> kruskal_mst(const Graph& g, int weightIndex);

// Prim MST
DynamicArray<FullEdge> prim_mst(const Graph& g, int weightIndex);

// Build adjacency from MST edges
DynamicArray< DynamicArray<FullEdge> > build_mst_adj(const DynamicArray<FullEdge>& mst, int n);

// DFS to find path inside MST adjacency
bool dfs_mst(int u, int dst,
             const DynamicArray< DynamicArray<FullEdge> >& adj,
             DynamicArray<int>& path,
             DynamicArray<bool>& vis);

// Minimum spanning path (choose Prim or Kruskal based on density)
PathResult minimum_spanning_path(const Graph& g,
                                 int src, int dst,
                                 int weightIndex);

#endif // MAP_NAVIGATOR_H



