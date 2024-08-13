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

void take_S(S s) {

}

void take_a_ref_S(const S& s) {

}

int main() {
  S s;
  S s2 = std::move(s); //moved

  [s3 = s2]() { //s3 = s2 copied
    //TD<decltype((s3))> xxx; // const S&, lambda without mutable
    //TD<decltype(s3)> xxx; //S
    S s_ = std::move(s3); // s3 is const, copied.
    (void)s_;
    // s_ destruct
    // s3 destruct
  }();

  S s3 = S(); //NRO construct. print nothing.
  S&& s4 = moved(s3);
  S s5 = s4; //copied
  std::cout << "S() is alive " << std::endl;
  //take_S(moved(s5)); //moved.
  take_a_ref_S(moved(s5)); //use const ref to take rvalue ref. neither copy or move. print nothing.
}
