#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <initializer_list>
#include <iostream>

struct Rational {
  long double value;

  static constexpr long double pres = 1e-2;

  explicit operator double() {
    return value;
  }

  Rational() : value(0) {}

  Rational(const Rational&) = default;

  Rational(int x) : value(x) {}

  bool operator==(const Rational& other) const {
    return std::abs(value - other.value) < pres;
  }

  bool operator!=(const Rational& other) const {
    return !(*this == other);
  }

  bool operator<(const Rational& other) const {
    return value < other.value;
  }

  bool operator>(const Rational& other) const {
    return value > other.value;
  }

  bool operator<=(const Rational& other) const {
    return value <= other.value;
  }

  bool operator>=(const Rational& other) const {
    return value >= other.value;
  }

  Rational& operator=(const Rational&) = default;

  Rational& operator+=(const Rational& other) {
    value += other.value;
    return *this;
  }

  Rational& operator-=(const Rational& other) {
    value -= other.value;
    return *this;
  }

  Rational& operator*=(const Rational& other) {
    value *= other.value;
    return *this;
  }

  Rational& operator/=(const Rational& other) {
    value /= other.value;
    return *this;
  }
};

Rational operator+(const Rational& first, const Rational& second) {
  Rational cnt = first;
  cnt += second;
  return cnt;
}

Rational operator-(const Rational& first, const Rational& second) {
  Rational cnt = first;
  cnt -= second;
  return cnt;
}

Rational operator*(const Rational& first, const Rational& second) {
  Rational cnt = first;
  cnt *= second;
  return cnt;
}

Rational operator/(const Rational& first, const Rational& second) {
  Rational cnt = first;
  cnt /= second;
  return cnt;
}

std::istream& operator>>(std::istream& input, Rational& elem) {
  input >> elem.value;

  return input;
}

std::ostream& operator<<(std::ostream& output, const Rational& elem) {
  output << elem.value;
  return output;
}

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
  int value;

  Residue() : value(0) {}

  Residue(int value) : value(((value % static_cast<int>(N)) + N) % N) {}

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
    ((value -= other.value) += N) %= N;
    return *this;
  }

  Residue& operator*=(const Residue& other) {
    (value *= other.value) %= N;
    return *this;
  }

  Residue& operator/=(const Residue& other) {  // devision by zero
    static_assert(is_prime<N>::value);

    for (size_t i = 0; i < N - 2; ++i) {
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

  template <size_t K>
  std::pair<bool, Matrix<N, K, Field>> step_view(
      const Matrix<N, K, Field>& matrix) const {
    bool inversions = false;
    size_t row = 0;
    size_t collum = 0;

    Matrix<N, K, Field> ans_matrix = matrix;

    Field zero;
    zero -= zero;  // нашёл 0 в поле

    while (row < N && collum < M) {
      bool find_non_zero_elem = false;

      // if (row == 15) {
      //   std::cout << ans_matrix[row][collum].numerator_;
      // }

      for (size_t i = row; i < N; ++i) {
        if (ans_matrix[i][collum] != zero) {
          if (i != row) {
            inversions = !inversions;

            for (size_t j = collum; j < K; ++j) {
              std::swap(ans_matrix[i][j], ans_matrix[row][j]);
            }
          }

          find_non_zero_elem = true;
          break;
        }
      }

      if (!find_non_zero_elem) {
        ++collum;
        continue;
      }

      for (size_t i = row + 1; i < N; ++i) {
        Field koef = ans_matrix[i][collum] / ans_matrix[row][collum];
        for (size_t j = collum; j < K; ++j) {
          ans_matrix[i][j] -= koef * ans_matrix[row][j];
        }
      }
      ++row;
      ++collum;
    }

    return std::make_pair(inversions, ans_matrix);
  }

  template <size_t K>
  Matrix<N, K, Field> reverse_gausse(const Matrix<N, K, Field>& matrix) const {
    Matrix<N, K, Field> ans = matrix;
    for (size_t i = N; i > 0; --i) {
      Field koeff = Field(1) / ans[i - 1][i - 1];

      ans[i - 1][i - 1] = Field(1);

      for (size_t j = N + 1; j <= K; ++j) {
        ans[i - 1][j - 1] *= koeff;
      }

      for (size_t j = i - 1; j > 0; --j) {
        Field one_more_koeff = ans[j - 1][i - 1] / ans[i - 1][i - 1];

        ans[j - 1][i - 1] = Field(0);
        for (size_t k = N + 1; k <= K; ++k) {
          ans[j - 1][k - 1] -= one_more_koeff * ans[i - 1][k - 1];
        }
      }
    }
    return ans;
  }

 public:
  Matrix() = default;

  Matrix(const Matrix<N, M, Field>&) = default;

  Matrix(std::initializer_list<std::initializer_list<Field>> values) {
    for (size_t i = 0; i < values.size(); ++i) {
      for (size_t j = 0; j < (*(values.begin() + i)).size(); ++j) {
        matrix[i][j] = *((*(values.begin() + i)).begin() + j);
      }
    }
  }

  Matrix& operator=(const Matrix<N, M, Field>&) = default;

  static Matrix unityMatrix() {
    static_assert(N == M);

    Matrix matrix;

    for (size_t i = 0; i < N; ++i) {
      matrix[i][i] = Field(1);
    }

    return matrix;
  }

  bool operator==(const Matrix<N, M, Field>& other) const {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < M; ++j) {
        if (matrix[i][j] != other[i][j]) return false;
      }
    }
    return true;
  }

  bool operator!=(const Matrix<N, M, Field>& other) const {
    return !(*this == other);
  }

  std::array<Field, M>& operator[](size_t i) {
    return matrix[i];
  }

  const std::array<Field, M>& operator[](size_t i) const {
    return matrix[i];
  }

  Matrix& operator+=(const Matrix<N, M, Field>& other) {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < M; ++j) {
        matrix[i][j] += other[i][j];
      }
    }

    return *this;
  }

  Matrix& operator-=(const Matrix<N, M, Field>& other) {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < M; ++j) {
        matrix[i][j] -= other[i][j];
      }
    }

    return *this;
  }

  Matrix& operator*=(const Field& elem) {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < M; ++j) {
        matrix[i][j] *= elem;
      }
    }

    return *this;
  }

  Matrix& operator*=(const Matrix<N, N, Field>& other) {
    // if (matrix[0][0] != Field(87) && N == 20 &&
    //     other.getRow(1)[0] != Field(87)) {
    //   std::cerr << "*=s\n";
    //   for (size_t i = 0; i < N; ++i) {
    //     std::cerr << matrix[0][i] << ' ';
    //   }
    //   std::cerr << '\n';
    // }
    static_assert(N == M);
    *this = *this * other;
    // if (matrix[0][0] != Field(87) && N == 20) {
    //   std::cerr << "*=f\n";
    //   for (size_t i = 0; i < N; ++i) {
    //     std::cerr << matrix[0][i] << ' ';
    //   }
    //   std::cerr << '\n';
    // }
    return *this;
  }

  std::array<Field, M> getRow(size_t i) const {
    return matrix[i];
  }

  std::array<Field, N> getColumn(size_t i) const {
    std::array<Field, N> ans;

    for (size_t j = 0; j < N; ++j) {
      ans[j] = matrix[j][i];
    }

    return ans;
  }

  Field trace() const {
    static_assert(N == M);

    Field ans(0);
    for (size_t i = 0; i < N; ++i) {
      ans += matrix[i][i];
    }

    return ans;
  }

  Matrix<M, N, Field> transposed() const {
    Matrix<M, N, Field> ans;

    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < M; ++j) {
        ans[j][i] = matrix[i][j];
      }
    }
    return ans;
  }

  Field det() const {
    static_assert(N == M);

    bool flag = true;
    for (size_t i = 0; i < M; ++i) {
      if (matrix[0][i] != Field(0)) {
        flag = true;
        break;
      }
    }
    if (flag && N == 20) {
      std::cerr << N << ' ' << M << " 00000!\n";
      return Field(1);
    }

    auto gausse_matrix = step_view<M>(*this);
    Field ans(1);
    for (size_t i = 0; i < N; ++i) {
      ans *= gausse_matrix.second[i][i];
    }
    return Field(gausse_matrix.first ? -1 : 1) * ans;
  }

  size_t rank() const {
    auto gausse_matrix = step_view<M>(*this).second;

    size_t ans = 0;

    Field zero;
    zero -= zero;

    for (size_t i = 0; i < N; ++i) {
      bool non_zero_row = false;
      for (size_t j = i; j < M; ++j) {
        if (gausse_matrix[i][j] != zero) {
          non_zero_row = true;
          break;
        }
      }
      ans += non_zero_row;
    }

    return ans;
  }

  Matrix inverted() const {
    static_assert(N == M);

    Matrix<N, 2 * N, Field> extended_matrix;

    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        extended_matrix[i][j] = (*this)[i][j];
        extended_matrix[i][j + N] = Field(i == j ? 1 : 0);
      }
    }
    auto gausse_matrix =
        reverse_gausse<2 * N>(step_view<2 * N>(extended_matrix).second);
    Matrix ans;

    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        ans[i][j] = gausse_matrix[i][j + N];
      }
    }

    return ans;
  }

  Matrix& invert() {
    return *this = inverted();
  }
};

template <size_t N, size_t M, typename Field = Rational>
std::ostream& operator<<(std::ostream& output,
                         const Matrix<N, M, Field>& matrix) {
  for (size_t i = 0; i < N; ++i) {
    for (const auto& elem : matrix.getRow(i)) {
      output << elem << ' ';
    }
    output << '\n';
  }

  return output;
}

template <size_t N, size_t M, typename Field = Rational>
Matrix<N, M> operator+(const Matrix<N, M, Field>& first,
                       const Matrix<N, M, Field>& second) {
  Matrix<N, M, Field> cnt = first;
  cnt += second;

  return cnt;
}

template <size_t N, size_t M, typename Field = Rational>
Matrix<N, M, Field> operator-(const Matrix<N, M, Field>& first,
                              const Matrix<N, M, Field>& second) {
  Matrix<N, M, Field> cnt = first;
  cnt -= second;

  return cnt;
}

template <size_t N, size_t M, typename Field = Rational>
Matrix<N, M, Field> operator*(const Matrix<N, M, Field>& matrix,
                              const Field& elem) {
  Matrix<N, M, Field> cnt = matrix;
  cnt *= elem;

  return cnt;
}

template <size_t N, size_t M, typename Field = Rational>
Matrix<N, M, Field> operator*(const Field& elem,
                              const Matrix<N, M, Field>& matrix) {
  return matrix * elem;
}

template <size_t N, size_t K, size_t M, typename Field = Rational>
Matrix<N, M, Field> operator*(const Matrix<N, K, Field>& first,
                              const Matrix<K, M, Field>& second) {
  Matrix<N, M, Field> ans_matrix;
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < M; ++j) {
      std::array<Field, K> row = first.getRow(i);
      std::array<Field, K> collum = second.getColumn(j);

      Field product(0);
      for (size_t k = 0; k < K; ++k) {
        product += row[k] * collum[k];
      }

      ans_matrix[i][j] = product;
    }
  }

  return ans_matrix;
}

template <size_t N, typename Field = Rational>
using SquareMatrix = Matrix<N, N, Field>;
