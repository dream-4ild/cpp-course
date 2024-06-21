//
// Created by dmitry on 6/18/24.
//

#include <functional>
#include <iostream>

#include "function.hpp"

struct Adder {
  std::vector<int> aaa;

  int operator()(int x, int y) const {
    return y - x;
  }
  Adder() : aaa(123, 4) {}

  Adder(const Adder&) {
    std::cout << "copy\n";
  };
  Adder(Adder&&) {
    std::cout << "move\n";
  }

  Adder& operator=(const Adder&) = default;
  Adder& operator=(Adder&&)      = default;
};

int ff(int x, int y) {
  return 13 * x + y;
}

struct TestStructRvalue {
  void usual_method(int) {}
  void rvalue_method(int) && {}
  void lvalue_method(int) & {}
  void const_lvalue_method(int) const& {}
};

int main() {
  Function<void(const TestStructRvalue&, int)> ff;
  Function<void(TestStructRvalue&&, int)> f(ff);

  return 0;
}
