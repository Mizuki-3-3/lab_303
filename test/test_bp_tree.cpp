
#include "test.hpp"
#include "bp_tree.hpp"
 
#include <algorithm>
#include <climits>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <random>
#include <string>
#include <type_traits>
#include <vector>
 
// ---------- адаптер к API ----------
 
// Результат поиска: ведёт себя как указатель на значение (== nullptr, != nullptr, !p, *p),
// но хранит копию, потому что search() возвращает значение, а не ссылку.
template <class V>
struct lookup_result {
    std::optional<V> v;
    explicit operator bool() const { return v.has_value(); }
    const V& operator*() const { return *v; }
    bool operator==(std::nullptr_t) const { return !v.has_value(); }
    bool operator!=(std::nullptr_t) const { return v.has_value(); }
};
 
template <class T, class K> static auto get(T& t, const K& k) {
    using V = std::decay_t<decltype(t.search(k))>;
    lookup_result<V> r;
    try {
        r.v = t.search(k);
    } catch (const not_found&) {
    } catch (const empty_container&) {
    }
    return r;
}
template <class T, class K, class V> static bool ins(T& t, const K& k, const V& v) { return t.insert(k, v); }
template <class T, class K> static bool del(T& t, const K& k) {
    try {
        return t.remove(k);
    } catch (const empty_container&) {
        return false;
    }
}
 
// ---------- вспомогательное ----------
static int val(int k) { return static_cast<int>(static_cast<unsigned>(k) * 10u + 1u); }  // unsigned: без UB при INT_MAX
 
static void warn_no_size() {
    static bool warned = false;
    if (!warned) {
        warned = true;
        std::cerr << "WARNING: у bp_tree нет публичного size(), проверки размера пропущены\n";
    }
}
 
template <class T> static bool size_is(const T& t, long n) {
    if constexpr (requires { { t.size() } -> std::convertible_to<std::size_t>; }) {
        return static_cast<long>(t.size()) == n;
    } else {
        warn_no_size();
        return true;
    }
}
 
enum order_kind { ASC, DESC, RAND };
 
static std::vector<int> make_order(order_kind o, int n, unsigned seed) {
    std::vector<int> v(n);
    std::iota(v.begin(), v.end(), 0);
    if (o == DESC) std::reverse(v.begin(), v.end());
    if (o == RAND) std::shuffle(v.begin(), v.end(), std::mt19937(seed));
    return v;
}
 
// Вставляет N ключей в порядке `in`, проверяет всё, затем удаляет в порядке `out`.
// После КАЖДОГО удаления проверяет, что все оставшиеся ключи на месте (ловит поломку borrow/merge).
template <std::size_t D>
static bool fill_and_drain(order_kind in, order_kind out) {
    const int N = 300;
    bp_tree<int, int, D, std::less<int>> t;
    auto in_order = make_order(in, N, 1), out_order = make_order(out, N, 2);
 
    for (int k : in_order)
        if (!ins(t, k, val(k))) return false;
    if (!size_is(t, N)) return false;
    for (int k = 0; k < N; ++k) {
        auto p = get(t, k);
        if (!p || *p != val(k)) return false;
    }
    if (get(t, -1) || get(t, N)) return false;
 
    for (int i = 0; i < N; ++i) {
        int k = out_order[i];
        if (!del(t, k) || get(t, k) || !size_is(t, N - i - 1)) return false;
        for (int j = i + 1; j < N; ++j) {
            auto p = get(t, out_order[j]);
            if (!p || *p != val(out_order[j])) return false;
        }
    }
    return !del(t, 0) && !get(t, 0);
}
 
template <std::size_t D>
static void sweep() {
    const order_kind ks[] = {ASC, DESC, RAND};
    for (auto a : ks)
        for (auto b : ks) CHECK((fill_and_drain<D>(a, b)));
}
 
// Случайные операции против std::map как эталона; узкий диапазон ключей => много дублей/промахов.
template <std::size_t D>
static bool random_vs_map(unsigned seed, int range, int ops) {
    bp_tree<int, int, D, std::less<int>> t;
    std::map<int, int> ref;
    std::mt19937 g(seed);
    const int lo = -range / 2;
 
    auto verify = [&] {
        if (!size_is(t, static_cast<long>(ref.size()))) return false;
        for (int k = lo; k < lo + range; ++k) {
            auto it = ref.find(k);
            auto p = get(t, k);
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
            if (ins(t, k, i) != ref.emplace(k, i).second) return false;
        } else {
            if (del(t, k) != (ref.erase(k) == 1)) return false;
        }
        if (!size_is(t, static_cast<long>(ref.size()))) return false;
        if (i % 1000 == 999 && !verify()) return false;
    }
    return verify();
}
 
using tree3 = bp_tree<int, int, 3, std::less<int>>;
 
// ================= базовые случаи =================
 
TEST(empty_tree) {
    tree3 t;
    CHECK(size_is(t, 0));
    CHECK(get(t, 0) == nullptr);
    CHECK(!del(t, 0));
    CHECK(size_is(t, 0));
    CHECK(ins(t, 1, 10));  // дерево после неудачного erase работоспособно
    CHECK(size_is(t, 1));
}
 
TEST(single_element) {
    tree3 t;
    CHECK(ins(t, 42, 7));
    CHECK(size_is(t, 1));
    CHECK(get(t, 42) != nullptr && *get(t, 42) == 7);
    CHECK(get(t, 41) == nullptr && get(t, 43) == nullptr);
    CHECK(del(t, 42));
    CHECK(size_is(t, 0));
    CHECK(get(t, 42) == nullptr);
    CHECK(!del(t, 42));  // повторное удаление
}
 
TEST(duplicate_insert_keeps_old_value) {
    tree3 t;
    CHECK(ins(t, 5, 50));
    CHECK(!ins(t, 5, 99));
    CHECK(size_is(t, 1));
    CHECK(*get(t, 5) == 50);
 
    // то же самое в многоуровневом дереве
    for (int k = 0; k < 200; ++k) ins(t, k, val(k));
    bool ok = true;
    for (int k = 0; k < 200; ++k) ok &= !ins(t, k, -1);
    CHECK(ok);
    CHECK(size_is(t, 200));
    ok = true;
    for (int k = 1; k < 200; ++k) ok &= (*get(t, k) == (k == 5 ? 50 : val(k)));  // ключ 5 был вставлен раньше со значением 50
    CHECK(ok);
}
 
TEST(reinsert_after_erase_gets_new_value) {
    tree3 t;
    for (int k = 0; k < 100; ++k) ins(t, k, val(k));
    CHECK(del(t, 50));
    CHECK(ins(t, 50, -5));
    CHECK(*get(t, 50) == -5);
    CHECK(size_is(t, 100));
}
 
TEST(extreme_and_negative_keys) {
    tree3 t;
    const int keys[] = {INT_MAX, INT_MIN, 0, -1, 1, INT_MAX - 1, INT_MIN + 1};
    bool ok = true;
    for (int k : keys) ok &= ins(t, k, val(k));
    for (int k = -100; k < 0; ++k) ins(t, k * 1000, val(k * 1000));
    CHECK(ok);
    ok = true;
    for (int k : keys) ok &= (get(t, k) != nullptr && *get(t, k) == val(k));
    CHECK(ok);
    CHECK(del(t, INT_MIN) && del(t, INT_MAX));
    CHECK(get(t, INT_MIN) == nullptr && get(t, INT_MAX) == nullptr);
    CHECK(get(t, INT_MIN + 1) != nullptr && get(t, INT_MAX - 1) != nullptr);
}
 
// ================= порядок вставки/удаления x степень дерева =================
 
TEST(insert_erase_all_orders_degree_2) { sweep<2>(); }
TEST(insert_erase_all_orders_degree_3) { sweep<3>(); }
TEST(insert_erase_all_orders_degree_4) { sweep<4>(); }
TEST(insert_erase_all_orders_degree_5) { sweep<5>(); }
 
// ================= удаление: специальные случаи =================
 
TEST(erase_absent_keys_in_populated_tree) {
    tree3 t;
    for (int k = 0; k < 400; k += 2) ins(t, k, val(k));  // только чётные
    bool ok = true;
    for (int k = 1; k < 400; k += 2) ok &= !del(t, k);   // нечётные - между ключами
    ok &= !del(t, -1) && !del(t, 400) && !del(t, INT_MIN) && !del(t, INT_MAX);  // за границами
    CHECK(ok);
    CHECK(size_is(t, 200));
    ok = true;
    for (int k = 0; k < 400; k += 2) ok &= (get(t, k) != nullptr);
    CHECK(ok);
}
 
TEST(erase_alternately_from_both_ends) {
    tree3 t;
    const int N = 300;
    for (int k = 0; k < N; ++k) ins(t, k, val(k));
    bool ok = true;
    for (int lo = 0, hi = N - 1; lo < hi; ++lo, --hi) {
        ok &= del(t, lo) && del(t, hi);
        ok &= (get(t, lo) == nullptr) && (get(t, hi) == nullptr);
        if (lo + 1 < hi - 1) ok &= (get(t, lo + 1) != nullptr) && (get(t, hi - 1) != nullptr);
    }
    CHECK(ok);
    CHECK(size_is(t, 0));
}
 
TEST(erase_every_other_then_reinsert) {
    tree3 t;
    const int N = 300;
    for (int k = 0; k < N; ++k) ins(t, k, val(k));
    for (int k = 0; k < N; k += 2) del(t, k);
    bool ok = true;
    for (int k = 0; k < N; ++k) ok &= ((get(t, k) != nullptr) == (k % 2 == 1));
    CHECK(ok);
    for (int k = 0; k < N; k += 2) ok &= ins(t, k, -k);
    CHECK(ok);
    CHECK(size_is(t, N));
    ok = true;
    for (int k = 0; k < N; ++k) ok &= (*get(t, k) == (k % 2 ? val(k) : -k));
    CHECK(ok);
}
 
TEST(tree_reusable_after_becoming_empty) {
    tree3 t;
    bool ok = true;
    for (int cycle = 0; cycle < 3; ++cycle) {
        auto order = make_order(RAND, 150, 100 + cycle);
        for (int k : order) ok &= ins(t, k, val(k) + cycle);
        for (int k : order) ok &= (*get(t, k) == val(k) + cycle);
        for (int k : order) ok &= del(t, k);
        ok &= size_is(t, 0) && get(t, order[0]) == nullptr;
    }
    CHECK(ok);
}
 
// ================= компаратор и типы =================
 
TEST(custom_comparator_greater) {
    bp_tree<int, int, 3, std::greater<int>> t;
    auto order = make_order(RAND, 300, 7);
    bool ok = true;
    for (int k : order) ok &= ins(t, k, val(k));
    CHECK(ok);
    CHECK(size_is(t, 300));
    ok = true;
    for (int k = 0; k < 300; ++k) ok &= (get(t, k) != nullptr && *get(t, k) == val(k));
    CHECK(ok);
    for (int k = 0; k < 300; k += 3) ok &= del(t, k);
    CHECK(ok);
    ok = true;
    for (int k = 0; k < 300; ++k) ok &= ((get(t, k) != nullptr) == (k % 3 != 0));
    CHECK(ok);
}
 
TEST(string_keys_and_values) {
    bp_tree<std::string, std::string, 3, std::less<std::string>> t;
    bool ok = true;
    ok &= ins(t, std::string(""), std::string("empty"));  // пустой ключ
    ok &= ins(t, std::string("a"), std::string("1"));
    ok &= ins(t, std::string("aa"), std::string("2"));  // ключи-префиксы друг друга
    ok &= ins(t, std::string("aaa"), std::string("3"));
    for (int i = 0; i < 200; ++i) ok &= ins(t, "key_" + std::to_string(i), "v" + std::to_string(i));
    CHECK(ok);
    CHECK(size_is(t, 204));
    CHECK(*get(t, std::string("")) == "empty");
    CHECK(*get(t, std::string("aa")) == "2");
    CHECK(get(t, std::string("aaaa")) == nullptr);
    CHECK(del(t, std::string("")) && get(t, std::string("")) == nullptr);
    ok = true;
    for (int i = 0; i < 200; ++i) ok &= (*get(t, "key_" + std::to_string(i)) == "v" + std::to_string(i));
    CHECK(ok);
}
 
TEST(random_ops_vs_std_map_degree_2) { CHECK((random_vs_map<2>(1, 200, 20000))); }
TEST(random_ops_vs_std_map_degree_3) { CHECK((random_vs_map<3>(2, 200, 20000))); }
TEST(random_ops_vs_std_map_degree_5) { CHECK((random_vs_map<5>(3, 500, 20000))); }
TEST(random_ops_vs_std_map_degree_16) { CHECK((random_vs_map<16>(4, 1000, 20000))); }
TEST(random_ops_tiny_key_range) { CHECK((random_vs_map<2>(5, 8, 5000))); }  // дерево часто пустеет/почти пустое
 
TEST(large_scale) {
    bp_tree<int, int, 4, std::less<int>> t;
    const int N = 50000;
    auto order = make_order(RAND, N, 9);
    bool ok = true;
    for (int k : order) ok &= ins(t, k, val(k));
    CHECK(ok);
    CHECK(size_is(t, N));
    for (int k : order)
        if (k % 2) ok &= del(t, k);
    CHECK(ok);
    CHECK(size_is(t, N / 2));
    ok = true;
    for (int k = 0; k < N; ++k) {
        auto p = get(t, k);
        ok &= (k % 2 == 0) ? (p != nullptr && *p == val(k)) : (p == nullptr);
    }
    CHECK(ok);
}
 
TEST_ENTRY_POINT