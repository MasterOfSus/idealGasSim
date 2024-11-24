
#ifndef ALGORITHM_HPP
#define ALGORITHM_HPP

#include <functional>
#include <iterator>

namespace gasSim {
namespace algorithm {

template <typename Iterator, typename Function>
void for_each_couple(Iterator first, Iterator last, Function f) {
    for (auto it1 = first; it1 != last; ++it1) {
        for (auto it2 = std::next(it1); it2 != last; ++it2) {
            f(*it1, *it2);
        }
    }
}

}  // namespace algorithm
}  // namespace gasSim

#endif
