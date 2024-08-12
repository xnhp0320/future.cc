#include <iostream>
#include <type_traits>

template <typename T>
class TD;


class S {
public:
  S() {}
  S(const S&) {std::cout << "copied" << std::endl;}
  S(S&&) {std::cout << "moved" << std::endl;}
  ~S() {std::cout << "destruct " << std::endl;}
};

S return_S() {
  return S();
}

S&& moved(S& s) {
  return std::move(s);
}

int main() {
  S s;
  S s2 = std::move(s);

  [s3 = s2]() {
    //TD<decltype((s3))> xxx;
    //TD<decltype(s3)> xxx;
    S s_ = std::move(s3);
    (void)s_;
  }();

  S s3 = S();
  S&& s4 = moved(s3);
  S s5 = s4;
  std::cout << "S() is alive " << std::endl;
}
