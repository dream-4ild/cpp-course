#include <iostream>
#include <cstring>

struct string {
  char* ptr = nullptr;
  int size = 0;
  int capacity;
};

struct Node {
  char* value;
  Node* next = nullptr;
  Node* previous = nullptr;
};

struct List {
  Node* head = nullptr;
  Node* tail = nullptr;
  int size = 0;
};

string* GetNewString(int n = 1) {
  char* new_ptr = new char[n];
  string* str = new string;

  (*str).ptr = new_ptr;
  (*str).capacity = n;
  return str;
}

void DeleteCString(char* str) {
  delete[] str;
}

string* ReallocString(string* str) {
  string* new_str = GetNewString((*str).size * 2);
  std::memcpy((*new_str).ptr, (*str).ptr, (*str).size);

  DeleteCString((*str).ptr);

  (*new_str).capacity = (*str).capacity * 2;
  (*new_str).size = (*str).size;

  delete str;
  return new_str;
}

void AddSimbol(string*& str, char elem) {
  if ((*str).size == (*str).capacity) {
    str = ReallocString(str);
  }
  (*str).ptr[(*str).size] = elem;
  ++(*str).size; 
}

char* GetString() {
  string* str = GetNewString();

  char elem = std::cin.get();

  while (elem != '\n' && elem != ' ') {
    AddSimbol(str, elem);
    elem = std::cin.get();
  }

  AddSimbol(str, '\0');

  char* ans_ptr = (*str).ptr;

  delete str;
  return ans_ptr;
}

int GetSize(List& stack) {
  return stack.size;
}

bool IsEmpty(List& stack) {
  return GetSize(stack) == 0;
}

void AddNode(List& stack, char* str) {
  Node* new_node = new Node;
  (*new_node).value = str;
  (*new_node).previous = stack.tail;

  (IsEmpty(stack) ? stack.head : (*stack.tail).next) = new_node; 
  
  stack.tail = new_node;
  ++stack.size;

  std::cout << "ok\n";
}

void DeleteNode(List& stack, bool print = true) {
  if (IsEmpty(stack)) {
    if (print) {
      std::cout << "error\n";
    }
    return;
  }
  if (print) {
    std::cout << (*stack.tail).value << '\n';
  }
  
  DeleteCString((*stack.tail).value);

  --stack.size;
  Node* tmp = stack.tail;
  if (IsEmpty(stack)) {
    stack.head = nullptr;
    stack.tail = nullptr;
  } else {
    stack.tail = (*stack.tail).previous;
  }

  delete tmp;
}

void Back(List& stack) {
  if(IsEmpty(stack)) {
    std::cout << "error\n";
    return;
  }
  std::cout << (*stack.tail).value << '\n';
}

void ClearList(List& stack) {
  while (stack.size > 0) {
    DeleteNode(stack, false);
  }
}

void DeleteList(List& stack) {
  ClearList(stack);
  delete &stack;
}

int main() {

  List* stack = new List;

  for (char* str = GetString(); std::strcmp(str, "exit") != 0; str = GetString()) {
    if (std::strcmp(str, "push") == 0) {
      char* value = GetString();
      AddNode(*stack, value);
    }
    if (std::strcmp(str, "pop") == 0) {
      DeleteNode(*stack);
    }
    if (std::strcmp(str, "back") == 0) {
      Back(*stack);
    }
    if (std::strcmp(str, "size") == 0) {
      std::cout << GetSize(*stack) << '\n';
    }
    if (std::strcmp(str, "clear") == 0) {
      ClearList(*stack);
      std::cout << "ok\n";
    }
    delete str;
  }
  std::cout << "bye\n";
  DeleteList(*stack);
  
  return 0;
}