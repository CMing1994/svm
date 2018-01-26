#pragma once

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>


namespace combinatorics {

    namespace {
        template <typename It, typename T = typename std::iterator_traits<It>::value_type>
        T multinomial_sorted(It begin, It end) {
            auto M = std::distance(begin, end);
            T n = std::accumulate(begin, end, 0);
            if (M < 2)
                return 1;
            --end;
            T c = 1, i = 1;
            auto m = M - 1;
            while (n > *end) {
                while (i > *begin) {
                    --m;
                    ++begin;
                }
                for (T j = 0; j < m; ++j, --n) {
                    if (c / i > std::numeric_limits<T>::max() / n)
                        throw std::range_error("Multinomial coefficient exceeds range");
                    c = c/i * n + c%i * n / i;
                }
                ++i;
            }
            return c;
        }
    }

    template <typename It, typename T = typename std::iterator_traits<It>::value_type>
    T multinomial(It const& begin, It const& end) {
        if (std::is_sorted(begin, end)) {
            return multinomial_sorted(begin, end);
        } else {
            std::vector<T> sorted(begin, end);
            std::sort(sorted.begin(), sorted.end());
            return multinomial_sorted(sorted.begin(), sorted.end());
        }
    }

    template <typename Container, typename T = typename Container::value_type>
    T multinomial(Container const& ks) {
        return multinomial(std::begin(ks), std::end(ks));
    }

    template <typename T>
    T multinomial(std::initializer_list<T> il) {
        return multinomial(il.begin(), il.end());
    }

    template <typename Container, typename T = typename Container::value_type>
    T multinomial_in_place(Container & ks) {
        std::sort(std::begin(ks), std::end(ks));
        return multinomial_sorted(std::begin(ks), std::end(ks));
    }

}
