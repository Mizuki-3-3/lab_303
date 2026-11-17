
#include "test.hpp"
#include "hash_table.hpp"
 
#include <algorithm>
#include <climits>
#include <cstddef>
#include <numeric>
#include <random>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
 
template <class T, class K> static auto* get(T& t, const K& k) {
    auto it = t.search(k);
    auto e = t.end();
    using V = std::remove_reference_t<decltype(it->second)>;
    V* res = nullptr;
    if (!(it == e)) res = &it->second;
    return res;
}

template <class T, class K> static bool del(T& t, const K& k) { return t.remove(k); }
 
static int val(int k) { return static_cast<int>(static_cast<unsigned>(k) * 10u + 1u); }
 
template <class T> static long size_of(T& t) {
    long n = 0;
    auto e = t.end();
    for (auto it = t.begin(); !(it == e); ++it) ++n;
    return n;
}
 
template <class T> static bool size_is(T& t, long n) { return size_of(t) == n; }
 
template <std::size_t N>
struct mod_hash {
    std::size_t operator()(int k) const { return static_cast<std::size_t>(k) % N; }
};
 
using table = hash_table<int, int, std::hash<int>>;
 
static std::vector<int> ascending(int n) {
    std::vector<int> v(n);
    std::iota(v.begin(), v.end(), 0);
    return v;
}
 
static std::vector<int> descending(int n) {
    auto v = ascending(n);
    std::reverse(v.begin(), v.end());
    return v;
}
 
static std::vector<int> shuffled(int n, unsigned seed) {
    auto v = ascending(n);
    std::shuffle(v.begin(), v.end(), std::mt19937(seed));
    return v;
}
 
// Порядок "от середины к краям": 20, 19, 21, 18, 22, ...
static std::vector<int> middle_out(int n) {
    std::vector<int> v;
    int mid = n / 2;
    for (int d = 0; static_cast<int>(v.size()) < n; ++d) {
        if (mid - d >= 0) v.push_back(mid - d);
        if (d > 0 && mid + d < n) v.push_back(mid + d);
    }
    return v;
}
 
template <class HT>
static bool drain_in_order(const std::vector<int>& order) {
    const int M = static_cast<int>(order.size());
    HT t;
    for (int k = 0; k < M; ++k)
        if (!t.insert(k, val(k))) return false;
    if (!size_is(t, M)) return false;
    for (int i = 0; i < M; ++i) {
        int k = order[i];
        if (!del(t, k) || get(t, k) || !size_is(t, M - i - 1)) return false;
        for (int j = i + 1; j < M; ++j) {
            auto* p = get(t, order[j]);
            if (!p || *p != val(order[j])) return false;
        }
    }
    return true;
}
 
template <class HT>
static bool random_vs_map(unsigned seed, int range, int ops) {
    HT t;
    std::unordered_map<int, int> ref;
    std::mt19937 g(seed);
    const int lo = -range / 2;
 
    auto verify = [&] {
        if (size_of(t) != static_cast<long>(ref.size())) return false;
        for (int k = lo; k < lo + range; ++k) {
            auto it = ref.find(k);
            auto* p = get(t, k);
            if ((it == ref.end()) != (p == nullptr)) return false;
            if (p && *p != it->second) return false;
        }
        return true;
    };
 
    for (int i = 0; i < ops; ++i) {
        int k = lo + static_cast<int>(g() % range);
        int r = static_cast<int>(g() % 3);
        bool do_insert = (i < ops / 2) ? (r < 2) : (r < 1);  // сначала рост, потом сжатие
        if (do_insert) {
            if (t.insert(k, i) != ref.emplace(k, i).second) return false;
        } else {
            if (del(t, k) != (ref.erase(k) == 1)) return false;
        }
        if (size_of(t) != static_cast<long>(ref.size())) return false;
        if (i % 1000 == 999 && !verify()) return false;
    }
    return verify();
}
 
 
TEST(empty_table) {
    table t;
    CHECK(size_is(t, 0));
    CHECK(get(t, 0) == nullptr);
    CHECK(!del(t, 0));
    CHECK(size_is(t, 0));
    CHECK(t.insert( 1, 10));
    CHECK(size_is(t, 1));
}
 
TEST(single_element) {
    table t;
    CHECK(t.insert(42, 7));
    CHECK(size_is(t, 1));
    CHECK(get(t, 42) != nullptr && *get(t, 42) == 7);
    std::cout<<(get(t, 42) != nullptr)<<std::endl;
    CHECK(get(t, 41) == nullptr && get(t, 43) == nullptr);
    CHECK(del(t, 42));
    CHECK(size_is(t, 0));
    CHECK(get(t, 42) == nullptr);
    CHECK(!del(t, 42));//повторное удаление
}
 
TEST(duplicate_insert_keeps_old_value) {
    table t;
    CHECK(t.insert(5, 50));
    CHECK(!t.insert(5, 99));
    CHECK(size_is(t, 1));
    CHECK(*get(t, 5) == 50);
 
    for (int k = 0; k < 200; ++k) t.insert(k, val(k));
    bool ok = true;
    for (int k = 0; k < 200; ++k) ok &= !t.insert(k, -1);
    CHECK(ok);
    CHECK(size_is(t, 200));
    ok = true;
    for (int k = 1; k < 200; ++k) ok &= (*get(t, k) == (k == 5 ? 50 : val(k)));
    CHECK(ok);
}
 
TEST(reinsert_after_remove_gets_new_value) {
    table t;
    for (int k = 0; k < 100; ++k) t.insert(k, val(k));
    CHECK(del(t, 50));
    CHECK(t.insert(50, -5));
    CHECK(*get(t, 50) == -5);
    CHECK(size_is(t, 100));
}
 
TEST(extreme_and_negative_keys) {
    table t;
    const int keys[] = {INT_MAX, INT_MIN, 0, -1, 1, INT_MAX - 1, INT_MIN + 1};
    bool ok = true;
    for (int k : keys) ok &= t.insert(k, val(k));
    CHECK(ok);
    ok = true;
    for (int k : keys) ok &= (get(t, k) != nullptr && *get(t, k) == val(k));
    CHECK(ok);
    CHECK(del(t, INT_MIN) && del(t, INT_MAX));
    CHECK(get(t, INT_MIN) == nullptr && get(t, INT_MAX) == nullptr);
    CHECK(get(t, INT_MIN + 1) != nullptr && get(t, INT_MAX - 1) != nullptr);
}

TEST(remove_absent_keys_in_populated_table) {
    table t;
    for (int k = 0; k < 400; k += 2) t.insert(k, val(k));
    bool ok = true;
    for (int k = 1; k < 400; k += 2) ok &= !del(t, k);
    ok &= !del(t, -1) && !del(t, 400) && !del(t, INT_MIN) && !del(t, INT_MAX);
    CHECK(ok);
    CHECK(size_is(t, 200));
}

TEST(many_inserts_force_rehash) {
    table t;
    const int N = 20000;
    auto order = shuffled(N, 3);
    bool ok = true;
    for (int i = 0; i < N; ++i) {
        ok &= t.insert(order[i], val(order[i]));
        // проверка после каждого "степени двойки", чтобы поймать потерю элементов именно при рехеше
        if ((i & (i + 1)) == 0) {
            for (int j = 0; j <= i; ++j) ok &= (get(t, order[j]) != nullptr);
        }
    }
    CHECK(ok);
    CHECK(size_is(t, N));
    ok = true;
    for (int k = 0; k < N; ++k) ok &= (get(t, k) != nullptr && *get(t, k) == val(k));
    ok &= (get(t, -1) == nullptr) && (get(t, N) == nullptr);
    CHECK(ok);
}
 
TEST(remove_all_then_reuse) {
    table t;
    bool ok = true;
    for (int cycle = 0; cycle < 3; ++cycle) {
        auto order = shuffled(1000, 50 + cycle);
        for (int k : order) ok &= t.insert(k, val(k) + cycle);
        std::cout<<ok<<std::endl;
        for (int k : order) ok &= (*get(t, k) == val(k) + cycle);
        std::cout<<ok<<std::endl;
        for (int k : order) ok &= del(t, k);
        std::cout<<ok<<std::endl;
        ok &= size_is(t, 0) && get(t, order[0]) == nullptr;
        std::cout<<size_is(t, 0) << (get(t, order[0]) == nullptr)<<ok<<std::endl;
    }
    CHECK(ok);
}
 
//коллизии: цепочки
 
TEST(all_keys_in_one_chain_remove_ascending) { CHECK((drain_in_order<hash_table<int, int, mod_hash<1>>>(ascending(40)))); }
TEST(all_keys_in_one_chain_remove_descending) { CHECK((drain_in_order<hash_table<int, int, mod_hash<1>>>(descending(40)))); }
TEST(all_keys_in_one_chain_remove_middle_out) { CHECK((drain_in_order<hash_table<int, int, mod_hash<1>>>(middle_out(40)))); }
TEST(all_keys_in_one_chain_remove_random) { CHECK((drain_in_order<hash_table<int, int, mod_hash<1>>>(shuffled(40, 11)))); }
 
TEST(few_chains_remove_random) { CHECK((drain_in_order<hash_table<int, int, mod_hash<7>>>(shuffled(100, 12)))); }
TEST(few_chains_remove_middle_out) { CHECK((drain_in_order<hash_table<int, int, mod_hash<7>>>(middle_out(100)))); }
 
TEST(collisions_duplicate_and_absent) {
    hash_table<int, int, mod_hash<1>> t;
    for (int k = 0; k < 30; ++k) t.insert(k, val(k));
    bool ok = true;
    for (int k = 0; k < 30; ++k) ok &= !t.insert(k, -1);  // дубль внутри длинной цепочки
    CHECK(ok);
    CHECK(size_is(t, 30));
    CHECK(get(t, 30) == nullptr && get(t, -1) == nullptr);  // промах: идём до конца цепочки
    CHECK(!del(t, 30));
    CHECK(size_is(t, 30));
}
 
TEST(collisions_reinsert_into_chain) {
    hash_table<int, int, mod_hash<1>> t;
    for (int k = 0; k < 20; ++k) t.insert( k, val(k));
    bool ok = true;
    for (int k = 0; k < 20; k += 3) ok &= del(t, k);
    for (int k = 0; k < 20; k += 3) ok &= t.insert(k, -k);
    CHECK(ok);
    CHECK(size_is(t, 20));
    ok = true;
    for (int k = 0; k < 20; ++k) ok &= (*get(t, k) == (k % 3 == 0 ? -k : val(k)));
    CHECK(ok);
}
 
TEST(negative_keys_with_collisions) {
    hash_table<int, int, mod_hash<5>> t;
    bool ok = true;
    for (int k = -50; k < 50; ++k) ok &= t.insert(k, val(k));
    CHECK(ok);
    ok = true;
    for (int k = -50; k < 50; ++k) ok &= (get(t, k) != nullptr && *get(t, k) == val(k));
    CHECK(ok);
    for (int k = -50; k < 50; k += 2) ok &= del(t, k);
    CHECK(ok);
    CHECK(size_is(t, 50));
}

TEST(string_keys_and_values) {
    hash_table<std::string, std::string, std::hash<std::string>> t;
    bool ok = true;
    ok &= t.insert(std::string(""), std::string("empty"));  // пустой ключ
    ok &= t.insert(std::string("a"), std::string("1"));
    ok &= t.insert(std::string("aa"), std::string("2"));
    for (int i = 0; i < 500; ++i) ok &= t.insert("key_" + std::to_string(i), "v" + std::to_string(i));
    CHECK(ok);
    CHECK(size_is(t, 503));
    CHECK(*get(t, std::string("")) == "empty");
    CHECK(get(t, std::string("aaa")) == nullptr);
    CHECK(del(t, std::string("")) && get(t, std::string("")) == nullptr);
    ok = true;
    for (int i = 0; i < 500; ++i) ok &= (*get(t, "key_" + std::to_string(i)) == "v" + std::to_string(i));
    CHECK(ok);
}


TEST(random_ops_vs_unordered_map) { CHECK((random_vs_map<table>(1, 500, 30000))); }
TEST(random_ops_vs_unordered_map_tiny_range) { CHECK((random_vs_map<table>(2, 8, 5000))); }  // таблица часто пустеет
TEST(random_ops_vs_unordered_map_single_chain) { CHECK((random_vs_map<hash_table<int, int, mod_hash<1>>>(3, 100, 10000))); }
TEST(random_ops_vs_unordered_map_few_chains) { CHECK((random_vs_map<hash_table<int, int, mod_hash<7>>>(4, 300, 20000))); }

TEST_ENTRY_POINT