#include <algorithm>
#include <iostream>
#include <cstring>

class String {
 private:
  char* ptr;
  size_t sz;
  size_t cap;

  String(size_t sz): ptr(new char[sz + 1]), sz(sz), cap(sz + 1) {
    ptr[sz] = '\0';
  }

  void realloc(size_t new_cap) {
    char* new_ptr = new char[new_cap + 1];
    memcpy(new_ptr, ptr, sz);
    delete[] ptr;
    ptr = new_ptr;
    cap = new_cap + 1;
  }

 public:
  String(): String(static_cast<size_t>(0)) {}

  String(const char* other): String(strlen(other)) {
    memcpy(ptr, other, sz); 
  }

  String(size_t sz, char value): String(sz) {
    memset(ptr, value, sz);
  }

  String(const String& other): String(other.sz) {
    memcpy(ptr, other.ptr, sz);
  }

  ~String() {
    delete[] ptr;
  }
  
  friend bool operator==(const String& first_str, const String& second_str);

  friend bool operator<(const String& first_str, const String& second_str);

  friend std::ostream& operator<<(std::ostream& out, const String& str);

  friend std::istream& operator>>(std::istream& inp, String& str);

  String& operator=(String other) {
    if (this != &other) {
      if (cap < other.cap) {
        delete[] ptr;
        ptr = new char[other.cap];
      }
      sz = other.sz;
      cap = other.cap;
      memcpy(ptr, other.ptr, sz + 1);
    }
    return *this;
  }

  char& operator[](size_t pos) {
    return ptr[pos];
  }

  const char& operator[](size_t pos) const{
    return ptr[pos];
  }

  String& operator+=(const String& other) {
    if (sz + other.sz >= cap) {
      realloc(sz + other.sz);
    }
    memcpy(ptr + sz, other.ptr, other.sz);
    sz += other.sz;
    ptr[sz] = '\0';
    return *this;
  }

  String& operator+=(char elem) {
    this->push_back(elem);
    return *this;
  }

  size_t length() const{
    return sz;
  }

  size_t size() const{
    return sz;
  }

  size_t capacity() const{
    return cap - 1;
  }
  
  void push_back(char value) {
    if (sz == 0) {
      realloc(1);
      ptr[sz] = value;
      ptr[++sz] = '\0';
      return;
    }
    if (sz == cap - 1) {
      realloc(2 * sz);
    }
    ptr[sz] = value;
    ptr[++sz] = '\0';
  }
  
  void pop_back() {
    --sz;
  }

  char& front() {
    return ptr[0];
  }
  
  const char& front() const {
    return ptr[0];
  }

  char& back() {
    if (sz == 0) return ptr[0];
    return ptr[sz - 1];
  }
  
  const char& back() const {
    if (sz == 0) return ptr[0];
    return ptr[sz - 1];
  }

  size_t find(const String& substr) const{
    for (size_t i = 0; i < sz - substr.sz + 1; ++i) {
      bool bad_substr = false;
      for (size_t j = 0; j < substr.sz; ++j) {
        if (substr[j] != ptr[i + j]) {
          bad_substr = true;
          break;
        }
      }
      if (!bad_substr) return i;
    }
    return sz;
  }

  size_t rfind(const String& substr) const{
    for (size_t i = sz - substr.sz + 1; i > 0; --i) {
      bool bad_substr = false;
      for (size_t j = 0; j < substr.sz; ++j) {
        if (ptr[i - 1 + j] != substr[j]) {
          bad_substr = true;
          break;
        }
      }
      if (!bad_substr) return i - 1;
    }
    return sz;
  }

  String substr(size_t start, size_t count) const{
    //if (start >= sz) return String();

    String cnt_string(std::min(count, sz - start));

    memcpy(cnt_string.ptr, ptr + start, count);
    cnt_string.ptr[cnt_string.sz] = '\0';

    return cnt_string;
  }

  bool empty() const{
    return (sz == 0);
  }

  void clear() {
    sz = 0;
    ptr[0] = '\0';
  }

  void shrink_to_fit() {
    if (sz != cap - 1) {
      realloc(sz);
    }
  }
  
  char* data() {
    return ptr;
  }

  const char* data() const {
    return ptr;
  }
};

bool operator==(const String& first_str, const String& second_str) {
  if (first_str.sz != second_str.sz) return false;
  for (size_t i = 0; i < first_str.sz; ++i) {
    if (first_str.ptr[i] != second_str.ptr[i]) return false;
  }
  return true;
}

bool operator!=(const String& first_str, const String& second_str) {
  return !(first_str == second_str);
}

bool operator<(const String& first_str, const String& second_str) {
  for (size_t i = 0; i < std::min(first_str.sz, second_str.cap); ++i) {
    if (first_str.ptr[i] != second_str.ptr[i]) {
      return (first_str.ptr[i] < second_str.ptr[i]);
    }
  }
  return (first_str.sz < second_str.sz);
}

bool operator<=(const String& first_str, const String& second_str) {
  return (first_str < second_str) || (first_str == second_str);
}

bool operator>(const String& first_str, const String& second_str) {
  return !(first_str <= second_str);
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
  out << str.ptr;
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