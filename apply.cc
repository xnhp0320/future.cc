#include <tuple>
#include <iostream>


template <typename ...T>
class TD;
//directly using apply results in gcc to use std::apply.
//use namespace. As we overload our apply with tuple.
//Guess it's related to ADL (Argument Dependent Lookup) stuff.

namespace my { 

template <typename Func, typename ...Args>
auto apply(Func&& f, Args&&... args) -> std::invoke_result_t<Func, Args...>
{
  return std::forward<Func>(f)(std::forward<Args>(args)...);
}

template<typename Func, typename Tuple, size_t ...Is>
auto apply_tuple(Func&& f, Tuple& t, std::index_sequence<Is...>) -> std::invoke_result_t<Func, decltype(std::get<Is>(t))...> {
  return std::forward<Func>(f)(std::get<Is>(t)...);
}

template <typename Func, typename ...Args>
auto apply(Func&& f, const std::tuple<Args...>& tuple) -> std::invoke_result_t<Func, Args...>
{
  return apply_tuple(std::forward<Func>(f), tuple, std::index_sequence_for<Args...>()); 
}

}

int main() {
  int x = 1;
  std::cout << my::apply([](int x, int y) { return x + y; }, x, 2) << std::endl;
}
