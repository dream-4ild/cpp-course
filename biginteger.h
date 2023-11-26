#include <algorithm>
#include <cmath>
#include <deque>
#include <iostream>

enum class Sign { Negative, Neutral, Positive };

class BigInteger {
 private:
  std::deque<int64_t> num_;
  Sign sign_;

  static const int64_t BASE_ = 1e9;

  void RemoveLeadingZeros() {
    for (size_t i = 0; (i < num_.size() - 1) && (num_[i] == 0);) {
      num_.pop_front();
    }
  }

  BigInteger(size_t size, bool) {
    num_.resize(size, 0);
    sign_ = Sign::Positive;
  }

 public:
  BigInteger(){};

  BigInteger(int64_t value) {
    sign_ = (value > 0 ? Sign::Positive
                       : (value == 0 ? Sign::Neutral : Sign::Negative));
    if (value == 0) base_shift();
    value = std::abs(value);
    while (value != 0) {
      num_.push_front(value % BASE_);
      value /= BASE_;
    }
  }

  friend std::ostream& operator<<(std::ostream&, const BigInteger&);
  friend std::istream& operator>>(std::istream&, BigInteger&);

  bool operator==(const BigInteger& second) const {
    if (num_.size() != second.num_.size() || sign_ != second.sign_)
      return false;

    for (size_t i = 0; i < num_.size(); ++i) {
      if (num_[i] != second.num_[i]) return false;
    }

    return true;
  }

  bool operator!=(const BigInteger& second) const { return !(*this == second); }

  bool operator<(const BigInteger& second) const {
    if (sign_ != second.sign_) return sign_ < second.sign_;
    if (sign_ == Sign::Positive) {
      if (num_.size() != second.num_.size()) {
        return num_.size() < second.num_.size();
      }
      for (size_t i = 0; i < num_.size(); ++i) {
        if (num_[i] != second.num_[i]) {
          return num_[i] < second.num_[i];
        }
      }
      return false;

    } else {
      if (sign_ == Sign::Neutral) return false;
      return second.abs() < abs();
    }
  }

  bool operator<=(const BigInteger& second) const {
    return *this == second || *this < second;
  }

  bool operator>(const BigInteger& second) const { return second < *this; }

  bool operator>=(const BigInteger& second) const { return !(*this < second); }

  BigInteger operator-() const {
    BigInteger new_bigint;
    new_bigint = *this;
    if (*this != 0) {
      new_bigint.sign_ =
          (sign_ == Sign::Positive ? Sign::Negative : Sign::Positive);
    }
    return new_bigint;
  }

  BigInteger& operator+=(const BigInteger& other) {
    if (*this == 0) {
      *this = other;
      return *this;
    }
    if (other == 0) return *this;

    if (sign_ == other.sign_) {
      int64_t transfer = 0;

      for (size_t i = 0; i < std::min(num_.size(), other.num_.size()); ++i) {
        int64_t cnt_num = num_[num_.size() - 1 - i];
        num_[num_.size() - 1 - i] =
            (num_[num_.size() - 1 - i] + other.num_[other.num_.size() - 1 - i] +
             transfer) %
            BASE_;
        transfer =
            (cnt_num + other.num_[other.num_.size() - 1 - i] + transfer) /
            BASE_;
      }

      if (num_.size() < other.num_.size()) {
        size_t size_num = num_.size();

        for (size_t i = 0; i < other.num_.size() - size_num; ++i) {
          num_.push_front(
              (other.num_[other.num_.size() - size_num - 1 - i] + transfer) %
              BASE_);
          transfer =
              (other.num_[other.num_.size() - size_num - 1 - i] + transfer) /
              BASE_;
        }

      } else {

        for (size_t i = 0; i < num_.size() - other.num_.size(); ++i) {
          int64_t cnt_num = num_[num_.size() - other.num_.size() - 1 - i];
          num_[num_.size() - other.num_.size() - 1 - i] =
              (num_[num_.size() - other.num_.size() - 1 - i] + transfer) %
              BASE_;
          transfer = (cnt_num + transfer) / BASE_;
        }
      }
      if (transfer != 0) {
        num_.push_front(transfer);
      }
    } else {
      *this -= (-other);
    }
    RemoveLeadingZeros();
    return *this;
  }

  BigInteger& operator-=(const BigInteger& other) {
    if (*this == other) {
      *this = 0;
      return *this;
    }

    if (sign_ == other.sign_ && sign_ == Sign::Positive) {
      if (*this > other) {
        int64_t transfer = 0;

        for (size_t i = 0; i < other.num_.size(); ++i) {
          int64_t cnt_num = num_[num_.size() - 1 - i];
          num_[num_.size() - 1 - i] =
              (num_[num_.size() - 1 - i] -
               other.num_[other.num_.size() - 1 - i] + transfer + BASE_) %
              BASE_;
          transfer = std::floor(
              static_cast<double>(
                  cnt_num - other.num_[other.num_.size() - 1 - i] + transfer) /
              BASE_);
        }

        for (size_t i = 0; i < num_.size() - other.num_.size(); ++i) {
          int64_t cnt_num = num_[num_.size() - other.num_.size() - 1 - i];
          num_[num_.size() - other.num_.size() - 1 - i] =
              (num_[num_.size() - other.num_.size() - 1 - i] + transfer +
               BASE_) %
              BASE_;
          transfer =
              std::floor(static_cast<double>(cnt_num + transfer) / BASE_);
        }
        
        RemoveLeadingZeros();
      } else {
        BigInteger cnt_bigint;
        cnt_bigint = other;
        *this = -(cnt_bigint -= *this);
      }
      return *this;
    }

    if (sign_ == other.sign_ && sign_ == Sign::Negative) {
      BigInteger cnt_bigint;
      cnt_bigint = -*this;
      *this = -(cnt_bigint -= (-other));
    } else {
      *this += -other;
    }

    return *this;
  }

  BigInteger& operator*=(const BigInteger& other) {
    if (*this == 0 || other == 0) {
      *this = 0;
      return *this;
    }

    BigInteger ans = BigInteger(num_.size(), true);

    BigInteger cnt_first = abs();
    BigInteger cnt_second = other.abs();

    int64_t transfer = 0;
    for (size_t i = 0; i < cnt_second.num_.size(); ++i) {
      for (size_t j = 0; j < cnt_first.num_.size(); ++j) {
        int64_t cnt = ans.num_[ans.num_.size() - 1 - j];
        ans.num_[ans.num_.size() - 1 - j] =
            (ans.num_[ans.num_.size() - 1 - j] +
             cnt_first.num_[cnt_first.num_.size() - 1 - j] *
                 cnt_second.num_[i] +
             transfer) %
            BASE_;
        transfer = (cnt +
                    cnt_first.num_[cnt_first.num_.size() - 1 - j] *
                        cnt_second.num_[i] +
                    transfer) /
                   BASE_;
      }

      for (size_t i = cnt_first.num_.size();
           i < ans.num_.size() && transfer != 0; ++i) {
        ans.num_[ans.num_.size() - 1 - i] += transfer;
        transfer = ans.num_[ans.num_.size() - 1 - i] / BASE_;
        ans.num_[ans.num_.size() - 1 - i] %= BASE_;
      }

      while (transfer != 0) {
        ans.num_.push_front(transfer % BASE_);
        transfer /= BASE_;
      }

      if (i != cnt_second.num_.size() - 1) ans.base_shift();
    }
    ans.sign_ = (sign_ == other.sign_ ? Sign::Positive : Sign::Negative);
    *this = ans;
    return *this;
  }

  BigInteger& operator/=(const BigInteger& other) {
    if (*this == 0) return *this;

    BigInteger abs_other = other.abs();
    BigInteger ans;
    BigInteger cnt_divisible;
    cnt_divisible.sign_ = Sign::Positive;

    bool one_more = false;
    bool need_one_more = false;

    for (size_t i = 0; i < num_.size(); ++i) {
      if (cnt_divisible.num_.size() < abs_other.num_.size() ||
          cnt_divisible == 0 || one_more || need_one_more) {
        if (cnt_divisible == 0) {
          cnt_divisible = num_[i];
        } else {
          cnt_divisible.num_.push_back(num_[i]);
        }
        if (cnt_divisible.num_.size() < abs_other.num_.size()) {
          ans.base_shift();
          continue;
        }
      }

      int64_t left = 0;
      int64_t right = BASE_ + 1;

      while (right - left > 1) {
        int64_t mid = left + (right - left) / 2;

        BigInteger product = abs_other;
        product *= mid;

        if (product <= cnt_divisible) {
          left = mid;
        } else {
          right = mid;
        }
      }

      BigInteger product = abs_other;
      product *= left;

      cnt_divisible -= product;
      if (cnt_divisible.num_.size() >= abs_other.num_.size() &&
          cnt_divisible != 0)
        need_one_more = true;
      
      ans.num_.push_back(left);

      if (left == 0) one_more = true;
    }

    ans.RemoveLeadingZeros();
    ans.sign_ = (sign_ == other.sign_ ? Sign::Positive : Sign::Negative);
    *this = ans;
    if (num_.size() == 1 && num_[0] == 0) sign_ = Sign::Neutral;
    return *this;
  }

  BigInteger& operator%=(const BigInteger& other) {
    BigInteger cnt = *this;
    cnt /= other;
    cnt *= other;
    *this -= cnt;
    return *this;
  }

  BigInteger& operator++() {
    *this += 1;
    return *this;
  }

  BigInteger operator++(int) {
    BigInteger ans = *this;
    *this += 1;
    return ans;
  }

  BigInteger& operator--() {
    *this -= 1;
    return *this;
  }

  BigInteger operator--(int) {
    BigInteger ans = *this;
    *this -= 1;
    return ans;
  }

  explicit operator bool() const { return sign_ != Sign::Neutral; }

  std::string toString() const {
    static int STEP = 9;
    std::string ans;

    if (sign_ == Sign::Negative) ans += '-';

    ans += std::to_string(num_[0]);

    for (size_t i = 1; i < num_.size(); ++i) {
      std::string str = std::to_string(num_[i]);
      str = std::string(STEP - str.size(), '0') + str;
      ans += str;
    }

    return ans;
  }

  BigInteger abs() const {
    BigInteger cnt_bigint;
    cnt_bigint = *this;

    if (sign_ != Sign::Neutral) cnt_bigint.sign_ = Sign::Positive;

    return cnt_bigint;
  }

  void base_shift() { num_.push_back(0); }
};

BigInteger operator+(const BigInteger& first, const BigInteger& second) {
  BigInteger ans;
  ans = first;
  return ans += second;
}

BigInteger operator-(const BigInteger& first, const BigInteger& second) {
  BigInteger ans;
  ans = first;
  return ans -= second;
}

BigInteger operator*(const BigInteger& first, const BigInteger& second) {
  BigInteger ans;
  ans = first;
  return ans *= second;
}

BigInteger operator/(const BigInteger& first, const BigInteger& second) {
  BigInteger ans;
  ans = first;
  return ans /= second;
}

BigInteger operator%(const BigInteger& first, const BigInteger& second) {
  BigInteger ans;
  ans = first;
  return ans %= second;
}

BigInteger operator""_bi(unsigned long long x) { return BigInteger(x); }

std::ostream& operator<<(std::ostream& out, const BigInteger& bigint) {
  out << bigint.toString();
  return out;
}

std::istream& operator>>(std::istream& inp, BigInteger& bigint) {
  static const int STEP = 9;

  bigint.num_.clear();
  bigint.sign_ = Sign::Neutral;

  std::string str;
  inp >> str;

  Sign sign;

  if (str[0] == '-') {
    str = str.substr(1);
    sign = Sign::Negative;
  } else {
    sign = Sign::Positive;
  }

  std::reverse(str.begin(), str.end());

  for (size_t i = 0; i < str.size(); i += STEP) {
    std::string cnt_str = str.substr(i, STEP);
    std::reverse(cnt_str.begin(), cnt_str.end());
    bigint.num_.push_front(std::stoi(cnt_str));
  }

  bigint.RemoveLeadingZeros();

  if (bigint != 0_bi) {
    bigint.sign_ = sign;
  }

  return inp;
}

class Rational {
 private:
  BigInteger numerator_;
  BigInteger denominator_;

  BigInteger GCD_(BigInteger first, BigInteger second) {

    while (first != 0 && second != 0) {

      if (first >= second) {
        first %= second;
      } else {
        second %= first;
      }
    }

    return (first == 0 ? second : first);
  }

  void do_beauty_() {
    BigInteger gcd = GCD_(numerator_.abs(), denominator_);
    numerator_ /= gcd;
    denominator_ /= gcd;
  }

 public:
  Rational() = default;

  Rational(BigInteger bigint) : numerator_(bigint), denominator_(1) {}

  Rational(int value) : Rational(BigInteger(value)) {}

  Rational& operator+=(const Rational& other) {
    numerator_ =
        numerator_ * other.denominator_ + denominator_ * other.numerator_;
    denominator_ *= other.denominator_;
    do_beauty_();
    return *this;
  }

  Rational& operator-=(const Rational& other) {
    numerator_ =
        numerator_ * other.denominator_ - denominator_ * other.numerator_;
    denominator_ *= other.denominator_;
    do_beauty_();
    return *this;
  }

  Rational& operator*=(const Rational& other) {
    numerator_ = numerator_ * other.numerator_;
    denominator_ *= other.denominator_;
    do_beauty_();
    return *this;
  }

  Rational& operator/=(const Rational& other) {
    numerator_ =
        numerator_ * other.denominator_ * (other.numerator_ > 0 ? 1 : -1);
    denominator_ *= other.numerator_.abs();
    do_beauty_();
    return *this;
  }

  Rational operator-() {
    numerator_ *= -1;
    return *this;
  }

  bool operator==(const Rational& other) const {
    return numerator_ == other.numerator_ && denominator_ == other.denominator_;
  }

  bool operator!=(const Rational& other) const { return !(*this == other); }

  bool operator<(const Rational& other) const {
    return numerator_ * other.denominator_ < denominator_ * other.numerator_;
  }

  bool operator<=(const Rational& other) const {
    return *this < other || *this == other;
  }

  bool operator>(const Rational& other) const { return !(*this <= other); }

  bool operator>=(const Rational& other) const { return !(*this < other); }

  std::string toString() {
    std::string ans;
    ans += numerator_.toString();

    if (denominator_ != 1) {
      ans += "/" + denominator_.toString();
    }

    return ans;
  }

  std::string asDecimal(size_t precision = 0) const {
    static const size_t STEP = 9;
    BigInteger integer_part = numerator_ / denominator_;
    std::string ans = (numerator_ > 0 ? "" : "-") + integer_part.toString();

    if (precision == 0 || denominator_ == 1) return ans;

    ans += ".";
    BigInteger float_part = numerator_.abs() % denominator_;

    size_t steps = std::ceil(static_cast<double>(precision) / STEP);

    do {
      float_part.base_shift();
      BigInteger cnt = float_part / denominator_;
      float_part %= denominator_;

      if (steps == 1) {
        std::string ccnt = cnt.toString();
        ccnt = std::string(STEP - ccnt.size(), '0') + ccnt;
        ans += ccnt.substr(
            0, precision -
                   std::floor(static_cast<double>(precision) / STEP) * STEP);
        break;
      }

      std::string ccnt = cnt.toString();
      ccnt = std::string(STEP - ccnt.size(), '0') + ccnt;
      ans += ccnt;
    } while (--steps);
    
    return ans;
  }

  explicit operator double() const { return std::stod(asDecimal(20)); }
};

Rational operator+(const Rational& first, const Rational& second) {
  Rational ans = first;
  ans += second;
  return ans;
}

Rational operator-(const Rational& first, const Rational& second) {
  Rational ans = first;
  ans -= second;
  return ans;
}

Rational operator*(const Rational& first, const Rational& second) {
  Rational ans = first;
  ans *= second;
  return ans;
}

Rational operator/(const Rational& first, const Rational& second) {
  Rational ans = first;
  ans /= second;
  return ans;
}
