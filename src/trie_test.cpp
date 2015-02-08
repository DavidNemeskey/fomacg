#include "tester_trie.h"

#include <array>
#include <set>
#include <iterator>
#include <algorithm>
#include <iostream>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

typedef Trie<int, std::set<int> > SetTree;
typedef Trie<int, int> IntTree;

using namespace rapidjson;

template <class T>
class Data {
  class DataIterator;

  T i;
public:
  Data(T _i) : i(_i) {}
  DataIterator begin() { return DataIterator(0); }
  DataIterator end() { return DataIterator(i); }

private:
  class DataIterator : public std::iterator<std::input_iterator_tag, T> {
    T i;
  public:
    DataIterator(T i);
    DataIterator& operator++() { ++i; return *this; }
    DataIterator operator++(int) { DataIterator tmp(*this); operator++(); return tmp; }
    bool operator==(const DataIterator& other) { return i == other.i; }
    inline bool operator!=(const DataIterator& other) { return !(i == other.i); }
    T& operator*() { return i; }
  };
};

template <class T>
Data<T>::DataIterator::DataIterator(T _i) : i(_i) {}

//template <>
//std::set<int> reduce_trie<int, std::set<int>>(Tree const* const trie) {
//  return std::set<int>{(int)trie->branching};
//}

void print_set(std::set<int> const* const to_print) {
  std::cout << "( ";
  if (to_print != nullptr) {
    std::copy(to_print->begin(), to_print->end(),
              std::ostream_iterator<int>(std::cout, " "));
  }
  std::cout << ")" << std::endl;
}

int main(int argc, char* argv[]) {
  SetTree t(1024);
  SetTree* t2 = t.add(3, nullptr);
  SetTree* t3 = t2->add(15, nullptr);
  std::array<size_t, 3> arr = {1, 2, 3};
  std::set<int>* pl = new std::set<int>{4, 5, 6};
  t.add_branch(begin(arr), end(arr), pl);
  t.add_branch(std::array<int, 2> {2, 3}, new std::set<int>{7, 8});
  t.add_branch(std::array<int, 2> {1, 4}, new std::set<int>{9});
  std::set<int>* res = t.match_all(std::array<size_t, 2>{1,2});
  print_set(res);
  res = t.match_all(arr);
  print_set(res);
  std::set<int> collect_res;
  t.collect(begin(arr), end(arr), std::inserter(collect_res, collect_res.begin()), ElementPayloadTransformer<std::set<int> >());
  print_set(&collect_res);

  IntTree it(10);
  it.add_branch(std::array<int, 3> {1, 2, 3});
  it.add_branch(std::array<int, 2> {1, 3}, new int{2});
  it.add_branch(std::array<int, 2> {2, 3}, new int{3});
  it.add_branch(std::array<int, 2> {3, 4}, new int{4});
  it.add_branch(std::array<int, 1> {1}, new int{1});
  std::set<int*> output;
  it.collect(begin(arr), end(arr), std::inserter(output, output.begin()));
  for (auto ip = begin(output); ip != end(output); ++ip) {
    std::cout << *(*ip) << std::endl;
  }

  // 1. Parse a JSON string into DOM.
  const char* json = "{\"project\":\"rapidjson\",\"stars\":10}";
  Document d;
  d.Parse(json);

  // 2. Modify it by DOM.
  Value& s = d["stars"];
  s.SetInt(s.GetInt() + 1);

  // 3. Stringify the DOM
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  d.Accept(writer);

  // Output {"project":"rapidjson","stars":11}
  std::cout << buffer.GetString() << std::endl;

  Data<int> data(6);
  for (auto i : data) {
    std::cout << "Data: " << i << std::endl;
  }

  for (auto it = t.begin(); it != t.end(); ++it) {
    std::cout << "it " << *it << std::endl;
  }
}
