#include <array>

#include "biginteger.h"

template <size_t N, size_t M = 2, bool flag_ = false>
struct is_prime {
  constexpr static bool value = (M * M <= N ? N % M != 0 : true) &&
                                is_prime<N, M + 1, (M * M > N)>::value;
};

template <size_t N, size_t M>
struct is_prime<N, M, true> {
  constexpr static bool value = true;
};

template <>
struct is_prime<1> {
  constexpr static bool value = false;
};

template <size_t N>
class Residue {
 public:
  size_t value;

  Residue() : value(0) {}

  explicit Residue(int value) : value(((value % N) + N) % N) {}

  Residue(const Residue&) = default;

  Residue& operator=(const Residue&) = default;

  explicit operator int() const {
    return value;
  }

  Residue& operator+=(const Residue& other) {
    (value += other.value) %= N;
    return *this;
  }

  Residue& operator-=(const Residue& other) {
    (value -= other.value + N) %= N;
    return *this;
  }

  Residue& operator*=(const Residue& other) {
    (value *= other.value) %= N;
    return *this;
  }

  Residue& operator/=(const Residue& other) {  // devision by zero
    static_assert(is_prime<N>::value);

    for (int i = 0; i < N - 2; ++i) {
      (value *= other.value) %= N;
    }

    return *this;
  }
};

template <size_t N>
std::ostream& operator<<(std::ostream& output, const Residue<N>& elem) {
  output << elem.value;
  return output;
}

template <size_t N>
std::istream& operator>>(std::istream& input, Residue<N>& elem) {
  int value;
  input >> value;
  elem = Residue<N>(value);

  return input;
}

template <size_t N>
bool operator==(const Residue<N>& first, const Residue<N>& second) {
  return first.value == second.value;
}

template <size_t N>
bool operator!=(const Residue<N>& first, const Residue<N>& second) {
  return !(first == second);
}

template <size_t N>
Residue<N> operator+(const Residue<N>& first, const Residue<N>& second) {
  Residue<N> cnt = first;
  cnt += second;
  return cnt;
}

template <size_t N>
Residue<N> operator-(const Residue<N>& first, const Residue<N>& second) {
  Residue<N> cnt = first;
  cnt -= second;
  return cnt;
}

template <size_t N>
Residue<N> operator*(const Residue<N>& first, const Residue<N>& second) {
  Residue<N> cnt = first;
  cnt *= second;
  return cnt;
}

template <size_t N>
Residue<N> operator/(const Residue<N>& first, const Residue<N>& second) {
  static_assert(is_prime<N>::value);

  Residue<N> cnt = first;
  cnt /= second;

  return cnt;
}

template <size_t N, size_t M, typename Field = Rational>
class Matrix {
 private:
  std::array<std::array<Field, M>, N> matrix;

 public:
  Matrix() = default;

  Field& operator[](size_t i, size_t j) {
    return matrix[i][j];
  }

  const Field& operator[](size_t i, size_t j) const {
    return matrix[i][j];
  }

  Matrix& operator+=(const Matrix<N, M, Field>& other) {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < M; ++j) {
        matrix[i, j] += other[i, j];
      }
    }

    return *this;
  }
};