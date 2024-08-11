#include <iostream>
#include <type_traits>

template <typename T>
class TD;


class S {
public:
  S() {}
  S(const S&) {std::cout << "copied" << std::endl;}
  S(S&&) {std::cout << "moved" << std::endl;}
  ~S() {}
};

int main() {
  S s;
  S s2 = std::move(s);

  [&]() mutable {
    S s_ = std::move(s2);
    (void)s_;
  }();
}
