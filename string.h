#include <algorithm>
#include <iostream>
#include <cstring>

class String {
 private:
  char* ptr_;
  size_t sz_;
  size_t cap_;

  String(size_t sz_): ptr_(new char[sz_ + 1]), sz_(sz_), cap_(sz_ + 1) {
    ptr_[sz_] = '\0';
  }

  void realloc(size_t new_cap) {
    char* new_ptr = new char[new_cap + 1];
    memcpy(new_ptr, ptr_, sz_);

    delete[] ptr_;

    ptr_ = new_ptr;
    cap_ = new_cap + 1;
  }

 public:
  String(): String(static_cast<size_t>(0)) {}

  String(const char* other): String(strlen(other)) {
    memcpy(ptr_, other, sz_); 
  }

  String(size_t sz_, char value): String(sz_) {
    memset(ptr_, value, sz_);
  }

  String(const String& other): String(other.sz_) {
    memcpy(ptr_, other.ptr_, sz_);
  }

  ~String() {
    delete[] ptr_;
  }

  String& operator=(const String& other) {
    if (this != &other) {
      if (cap_ < other.cap_) {
        delete[] ptr_;
        ptr_ = new char[other.cap_];
      }

      sz_ = other.sz_;
      cap_ = other.cap_;
      memcpy(ptr_, other.ptr_, sz_ + 1);
    }
    
    return *this;
  }

  char& operator[](size_t pos) {
    return ptr_[pos];
  }

  const char& operator[](size_t pos) const {
    return ptr_[pos];
  }

  String& operator+=(const String& other) {
    if (sz_ + other.sz_ >= cap_) {
      realloc(sz_ + other.sz_);
    }

    memcpy(ptr_ + sz_, other.ptr_, other.sz_ + 1);
    sz_ += other.sz_;

    return *this;
  }

  String& operator+=(char elem) {
    this->push_back(elem);
    return *this;
  }

  size_t length() const {
    return sz_;
  }

  size_t size() const {
    return sz_;
  }

  size_t capacity() const {
    return cap_ - 1;
  }
  
  void push_back(char value) {
    if (sz_ == cap_ - 1) {
      realloc(2 * sz_ + 1);
    }

    ptr_[sz_] = value;
    ptr_[++sz_] = '\0';
  }
  
  void pop_back() {
    ptr_[--sz_] = '\0';
  }

  char& front() {
    return ptr_[0];
  }
  
  const char& front() const {
    return ptr_[0];
  }

  char& back() {
    return ptr_[sz_ - 1];
  }
  
  const char& back() const {
    return ptr_[sz_ - 1];
  }

  size_t find(const String& substr) const {
    return std::distance(ptr_, std::search(ptr_, ptr_ + sz_, substr.ptr_, substr.ptr_ + substr.sz_));
  }

  size_t rfind(const String& substr) const {
    return std::distance(ptr_, std::find_end(ptr_, ptr_ + sz_, substr.ptr_, substr.ptr_ + substr.sz_));
  }

  String substr(size_t start, size_t count) const {
    String cnt_string(std::min(count, sz_ - start));

    memcpy(cnt_string.ptr_, ptr_ + start, cnt_string.sz_);
    cnt_string.ptr_[cnt_string.sz_] = '\0';

    return cnt_string;
  }

  bool empty() const {
    return (sz_ == 0);
  }

  void clear() {
    sz_ =0;
    ptr_[0] = '\0';
  }

  void shrink_to_fit() {
    if (sz_ != cap_ - 1) {
      realloc(sz_);
    }
  }
  
  char* data() {
    return ptr_;
  }

  const char* data() const {
    return ptr_;
  }
};

bool operator==(const String& first_str, const String& second_str) {
  if (first_str.size() != second_str.size()) {
    return false;
  }

  const char* first_ptr = first_str.data();
  const char* second_ptr = second_str.data();

  for (size_t i = 0; i < first_str.size(); ++i) {
    if (first_ptr[i] != second_ptr[i]) {
      return false;
    }
  }
  return true;
}

bool operator<(const String& first_str, const String& second_str) {
  const char* first_ptr = first_str.data();
  const char* second_ptr = second_str.data();

  for (size_t i = 0; i < std::min(first_str.size(), second_str.capacity() + 1); ++i) {
    if (first_ptr[i] != second_ptr[i]) {
      return (first_ptr[i] < second_ptr[i]);
    }
  }
  return (first_str.size() < second_str.size());
}

bool operator!=(const String& first_str, const String& second_str) {
  return !(first_str == second_str);
}

bool operator<=(const String& first_str, const String& second_str) {
  return !(second_str < first_str);
}

bool operator>(const String& first_str, const String& second_str) {
  return second_str < first_str;
}

bool operator>=(const String& first_str, const String& second_str) {
  return !(first_str < second_str);
}

String operator+(const String& first_str, const String& second_str) {
  String cnt_string = first_str;
  cnt_string += second_str;
  return cnt_string;
}

String operator+(const String& str, char elem) {
  String cnt_string = str;
  cnt_string.push_back(elem);
  return cnt_string;
}

String operator+(char elem, const String& str) {
  String cnt_string(1, elem);
  cnt_string += str;
  return cnt_string;
}

std::ostream& operator<<(std::ostream& out, const String& str) {
  out << str.data();
  return out;
}

std::istream& operator>>(std::istream& inp, String& str) {
  String cnt_string;
  char elem;
  elem = inp.get();
  
  while (elem != ' ' && elem != '\n' && !inp.eof()) {
    cnt_string.push_back(elem);
    elem = inp.get();
  }

  str = cnt_string;
  return inp;
}