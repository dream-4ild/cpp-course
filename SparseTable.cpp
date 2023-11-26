#include <iostream>
#include <vector>
#include <cmath>

class SparseTable {
 private:
  std::vector<std::vector<int>> data_;

  void BuildTable() {
    for (size_t i = 0; i + 1 < data_.size(); ++i) {
      for (size_t j = 0; j + (1 << i) < data_[0].size(); ++j) {
        data_[i + 1][j] = std::min(data_[i][j], data_[i][j + (1 << i)]);
      }
    }
  }

 public:
  SparseTable(std::vector<int>& values) {
    data_.resize(std::ceil(std::log2(values.size() + 1)), std::vector<int>(values.size(), 0));
    data_[0] = values;
    BuildTable();
  }

  int FindMin(int left, int right) {
    int power = std::floor(std::log2(right - left));
    return std::min(data_[power][left], data_[power][right - (1 << power)]);
  } 
};

int main() {
  std::vector<int> a(10);
  for (auto& elem : a) {
    std::cin >> elem;
  }

  SparseTable table(a);

  while (true) {
    int l, r;
    std::cin >> l >> r;
    std::cout << table.FindMin(l - 1, r) << '\n';
  }
}