#pragma once
#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<vector>
#include<queue>
#include<stack>
#include<deque>
#include<map>
#include<unordered_map>
#include<functional>
#include<algorithm>
#include "json.hpp"
using json = nlohmann::json;
// ─────────────────────────────────────────────────────────────────────────────
// KEIRO — C++ Instrumentation Header
// version: 1.0
//
// Usage:
//   #include "keiro.h"
//   keiro::priority_queue<int> pq;
//   keiro::stack<int> st;
//   keiro::vector<int> v;
//   // ... rest of your code unchanged
//   // trace is written to keiro_trace.json on program exit
// ─────────────────────────────────────────────────────────────────────────────
namespace keiro {
// ─── Trace writer (singleton) ────────────────────────────────────────────────
class Tracer {
public:
    static Tracer& instance() {
        static Tracer t;
        return t;
    }
    void set_source(const std::string& filename, const std::string& stdin_data) {
        meta_["lang"]        = "cpp";
        meta_["source_file"] = filename;
        meta_["stdin"]       = stdin_data;
        meta_["exit_code"]   = 0;
    }
    void register_structure(const std::string& name, const std::string& type) {
        meta_["structures"][name] = type;
    }
    void emit(int line,
              const std::string& op,
              const std::string& ds_name,
              const std::string& ds_type,
              const json& val,
              const json& snapshot,
              const json& vars,
              const std::vector<std::string>& vars_changed)
    {
        json step;
        step["step"]         = step_counter_;
        step["line"]         = line;
        step["op"]           = op;
        step["ds_name"]      = ds_name;
        step["ds_type"]      = ds_type;
        step["val"]          = val;
        step["snapshot"]     = snapshot;
        step["vars"]         = vars;
        step["vars_changed"] = vars_changed;
        step["op_count"]     = ++op_count_;
        steps_.push_back(step);
        step_counter_++;
    }

    void flush(const std::string& outfile = "keiro_trace.json") {
        meta_["total_steps"]     = step_counter_;
        meta_["compile_time_ms"] = 0;
        meta_["run_time_ms"]     = 0;

        json trace;
        trace["keiro_version"] = "1.0";
        trace["meta"]          = meta_;
        trace["steps"]         = steps_;

        std::ofstream f(outfile);
        f << trace.dump(2);
        f.close();
    }
private:
    Tracer() = default;
    json meta_;
    json steps_ = json::array();
    int  step_counter_ = 0;
    int  op_count_     = 0;
};
// Convenience macro — captures __LINE__ at call site
#define KEIRO_EMIT(op, ds_name, ds_type, val, snapshot, vars, changed) \
    keiro::Tracer::instance().emit(__LINE__, op, ds_name, ds_type, val, snapshot, vars, changed)
// Auto-flush on exit
struct _AutoFlush {
    ~_AutoFlush() { keiro::Tracer::instance().flush(); }
};
static _AutoFlush _auto_flush;
// ─── Helpers ─────────────────────────────────────────────────────────────────
// Serialize a heap-ordered container to JSON array
template <typename T>
json heap_to_json(std::vector<T> data) {
    json arr = json::array();
    for (auto& x : data) arr.push_back(x);
    return arr;
}
// Serialize a linear container to JSON array
template <typename Container>
json linear_to_json(const Container& c) {
    json arr = json::array();
    for (auto& x : c) arr.push_back(x);
    return arr;
}
// ─── keiro::priority_queue ───────────────────────────────────────────────────
template <
    typename T,
    typename Container  = std::vector<T>,
    typename Compare    = std::less<typename Container::value_type>
>
class priority_queue {
public:
    explicit priority_queue(const std::string& name = "_pq")
        : name_(name) {
        Tracer::instance().register_structure(name_, "heap");
    }
    void push(const T& val) {
        pq_.push(val);
        snapshot_vec_.push_back(val);
        std::push_heap(snapshot_vec_.begin(), snapshot_vec_.end(), Compare{});
        KEIRO_EMIT("push", name_, "heap",
            json(val),
            heap_to_json(snapshot_vec_),
            json::object(),
            std::vector<std::string>{});
    }
    void pop() {
        T top = pq_.top();
        pq_.pop();
        std::pop_heap(snapshot_vec_.begin(), snapshot_vec_.end(), Compare{});
        snapshot_vec_.pop_back();
        KEIRO_EMIT("pop", name_, "heap",
            json(top),
            heap_to_json(snapshot_vec_),
            json::object(),
            std::vector<std::string>{});
    }
    const T& top() const {
        KEIRO_EMIT("peek", name_, "heap",
            json(pq_.top()),
            heap_to_json(snapshot_vec_),
            json::object(),
            std::vector<std::string>{});
        return pq_.top();
    }
    bool      empty() const { return pq_.empty(); }
    size_t    size()  const { return pq_.size(); }
private:
    std::priority_queue<T, Container, Compare> pq_;
    std::vector<T> snapshot_vec_;   // mirror for snapshot serialization
    std::string name_;
};
// ─── keiro::stack ────────────────────────────────────────────────────────────
template <typename T>
class stack {
public:
    explicit stack(const std::string& name = "_stack")
        : name_(name) {
        Tracer::instance().register_structure(name_, "array");
    }

    void push(const T& val) {
        st_.push(val);
        data_.push_back(val);
        KEIRO_EMIT("push_back", name_, "array",
            json(val),
            linear_to_json(data_),
            json::object(),
            std::vector<std::string>{});
    }

    void pop() {
        T top = st_.top();
        st_.pop();
        data_.pop_back();
        KEIRO_EMIT("pop_back", name_, "array",
            json(top),
            linear_to_json(data_),
            json::object(),
            std::vector<std::string>{});
    }

    const T& top() const { return st_.top(); }
    bool     empty() const { return st_.empty(); }
    size_t   size()  const { return st_.size(); }

private:
    std::stack<T> st_;
    std::vector<T> data_;
    std::string name_;
};
// ─── keiro::queue ────────────────────────────────────────────────────────────
template <typename T>
class queue {
public:
    explicit queue(const std::string& name = "_queue")
        : name_(name) {
        Tracer::instance().register_structure(name_, "array");
    }

    void push(const T& val) {
        q_.push(val);
        data_.push_back(val);
        KEIRO_EMIT("push_back", name_, "array",
            json(val),
            linear_to_json(data_),
            json::object(),
            std::vector<std::string>{});
    }

    void pop() {
        T front = q_.front();
        q_.pop();
        data_.pop_front();
        KEIRO_EMIT("pop_front", name_, "array",
            json(front),
            linear_to_json(data_),
            json::object(),
            std::vector<std::string>{});
    }

    const T& front() const { return q_.front(); }
    const T& back()  const { return q_.back(); }
    bool     empty() const { return q_.empty(); }
    size_t   size()  const { return q_.size(); }

private:
    std::queue<T> q_;
    std::deque<T> data_;
    std::string name_;
};


// ─── keiro::deque ────────────────────────────────────────────────────────────

template <typename T>
class deque {
public:
    explicit deque(const std::string& name = "_deque")
        : name_(name) {
        Tracer::instance().register_structure(name_, "array");
    }

    void push_back(const T& val) {
        dq_.push_back(val);
        KEIRO_EMIT("push_back", name_, "array",
            json(val), linear_to_json(dq_), json::object(), {});
    }

    void push_front(const T& val) {
        dq_.push_front(val);
        KEIRO_EMIT("push_front", name_, "array",
            json(val), linear_to_json(dq_), json::object(), {});
    }

    void pop_back() {
        T val = dq_.back();
        dq_.pop_back();
        KEIRO_EMIT("pop_back", name_, "array",
            json(val), linear_to_json(dq_), json::object(), {});
    }

    void pop_front() {
        T val = dq_.front();
        dq_.pop_front();
        KEIRO_EMIT("pop_front", name_, "array",
            json(val), linear_to_json(dq_), json::object(), {});
    }

    T&       operator[](size_t i)       { return dq_[i]; }
    const T& operator[](size_t i) const { return dq_[i]; }
    bool     empty() const { return dq_.empty(); }
    size_t   size()  const { return dq_.size(); }
    T&       front()       { return dq_.front(); }
    T&       back()        { return dq_.back(); }

private:
    std::deque<T> dq_;
    std::string name_;
};


// ─── keiro::vector ───────────────────────────────────────────────────────────

template <typename T>
class vector {
public:
    explicit vector(const std::string& name = "_vec")
        : name_(name) {
        Tracer::instance().register_structure(name_, "array");
    }

    void push_back(const T& val) {
        data_.push_back(val);
        KEIRO_EMIT("push_back", name_, "array",
            json(val), linear_to_json(data_), json::object(), {});
    }

    void pop_back() {
        T val = data_.back();
        data_.pop_back();
        KEIRO_EMIT("pop_back", name_, "array",
            json(val), linear_to_json(data_), json::object(), {});
    }

    T& operator[](size_t i) {
        return data_[i];
    }

    // Tracked write via .set(i, val)
    void set(size_t i, const T& val) {
        data_[i] = val;
        KEIRO_EMIT("set", name_, "array",
            json({{"index", i}, {"val", val}}),
            linear_to_json(data_),
            json::object(),
            {});
    }

    // Tracked read via .get(i)
    const T& get(size_t i) {
        KEIRO_EMIT("get", name_, "array",
            json({{"index", i}, {"val", data_[i]}}),
            linear_to_json(data_),
            json::object(),
            {});
        return data_[i];
    }

    size_t size()  const { return data_.size(); }
    bool   empty() const { return data_.empty(); }
    void   resize(size_t n, T val = T{}) { data_.resize(n, val); }
    void   assign(size_t n, T val)       { data_.assign(n, val); }

    typename std::vector<T>::iterator begin() { return data_.begin(); }
    typename std::vector<T>::iterator end()   { return data_.end(); }

private:
    std::vector<T> data_;
    std::string name_;
};


// ─── keiro::grid (2D DP array) ───────────────────────────────────────────────

template <typename T>
class grid {
public:
    grid(size_t rows, size_t cols, T init = T{}, const std::string& name = "_dp")
        : rows_(rows), cols_(cols), name_(name),
          data_(rows, std::vector<T>(cols, init)),
          computed_(rows, std::vector<bool>(cols, false))
    {
        Tracer::instance().register_structure(name_, "grid");
    }

    void set(size_t i, size_t j, const T& val) {
        data_[i][j]     = val;
        computed_[i][j] = true;
        KEIRO_EMIT("set", name_, "grid",
            json({{"i", i}, {"j", j}, {"val", val}}),
            snapshot(),
            json::object(),
            {});
    }

    const T& get(size_t i, size_t j) {
        KEIRO_EMIT("get", name_, "grid",
            json({{"i", i}, {"j", j}, {"val", data_[i][j]}}),
            snapshot(),
            json::object(),
            {});
        return data_[i][j];
    }

    T& operator()(size_t i, size_t j) { return data_[i][j]; }

    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }

private:
    json snapshot() const {
        json s = json::array();
        for (size_t i = 0; i < rows_; i++) {
            json row = json::array();
            for (size_t j = 0; j < cols_; j++) {
                if (computed_[i][j]) row.push_back(data_[i][j]);
                else                 row.push_back(nullptr);
            }
            s.push_back(row);
        }
        return s;
    }

    size_t rows_, cols_;
    std::string name_;
    std::vector<std::vector<T>>    data_;
    std::vector<std::vector<bool>> computed_;
};


// ─── keiro::dsu (Union-Find) ─────────────────────────────────────────────────

class dsu {
public:
    explicit dsu(size_t n, const std::string& name = "_dsu")
        : name_(name), parent_(n), rank_(n, 0)
    {
        for (size_t i = 0; i < n; i++) parent_[i] = i;
        Tracer::instance().register_structure(name_, "dsu");
    }

    int find(int x) {
        if (parent_[x] != x) parent_[x] = find(parent_[x]);
        KEIRO_EMIT("find", name_, "dsu",
            json(x), snapshot(), json::object(), {});
        return parent_[x];
    }

    bool unite(int a, int b) {
        int ra = find(a), rb = find(b);
        if (ra == rb) return false;
        if (rank_[ra] < rank_[rb]) std::swap(ra, rb);
        parent_[rb] = ra;
        if (rank_[ra] == rank_[rb]) rank_[ra]++;
        KEIRO_EMIT("union", name_, "dsu",
            json({{"a", a}, {"b", b}}), snapshot(), json::object(), {});
        return true;
    }

    bool connected(int a, int b) { return find(a) == find(b); }

private:
    json snapshot() const {
        return json{{"parent", parent_}, {"rank", rank_}};
    }

    std::string name_;
    std::vector<int> parent_, rank_;
};


// ─── keiro::hashmap ──────────────────────────────────────────────────────────

template <typename K, typename V>
class hashmap {
public:
    explicit hashmap(const std::string& name = "_map", size_t buckets = 8)
        : name_(name), bucket_count_(buckets)
    {
        Tracer::instance().register_structure(name_, "hashmap");
    }

    void insert(const K& key, const V& val) {
        data_[key] = val;
        KEIRO_EMIT("insert", name_, "hashmap",
            json({{"key", key}, {"val", val}}),
            snapshot(), json::object(), {});
    }

    void erase(const K& key) {
        V val = data_[key];
        data_.erase(key);
        KEIRO_EMIT("erase", name_, "hashmap",
            json({{"key", key}, {"val", val}}),
            snapshot(), json::object(), {});
    }

    V& operator[](const K& key) {
        bool existed = data_.count(key) > 0;
        V& ref = data_[key];
        KEIRO_EMIT(existed ? "update" : "insert", name_, "hashmap",
            json({{"key", key}}),
            snapshot(), json::object(), {});
        return ref;
    }

    bool   count(const K& key) const { return data_.count(key); }
    size_t size()               const { return data_.size(); }
    bool   empty()              const { return data_.empty(); }

private:
    json snapshot() const {
        // Distribute keys across buckets using std::hash
        std::vector<json> buckets(bucket_count_, json::array());
        std::hash<K> hasher;
        for (auto& [k, v] : data_) {
            size_t idx = hasher(k) % bucket_count_;
            buckets[idx].push_back(json::array({k, v}));
        }
        return json(buckets);
    }

    std::unordered_map<K, V> data_;
    std::string name_;
    size_t bucket_count_;
};
} // namespace keiro