#include "tester_trie.h"

#include <array>
#include <set>
#include <iterator>
#include <algorithm>
#include <iostream>

typedef Trie<int, std::set<int> > SetTree;
typedef Trie<int, int> IntTree;

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
  std::set<int>* res = t.match_all(std::array<size_t, 2>{1,2});
  print_set(res);
  res = t.match_all(arr);
  print_set(res);

  IntTree it(10);
  it.add_branch(std::array<int, 3> {1, 2, 3});
  it.add_branch(std::array<int, 2> {1, 3}, new int{2});
  it.add_branch(std::array<int, 2> {2, 3}, new int{3});
  it.add_branch(std::array<int, 2> {3, 4}, new int{4});
  it.add_branch(std::array<int, 1> {1}, new int{1});
  auto collector = TrieCollector<int, int>();
  std::set<int*> output;
  collector.collect(&it, begin(arr), end(arr), std::inserter(output, output.begin()));
  for (auto ip = begin(output); ip != end(output); ++ip) {
    std::cout << *(*ip) << std::endl;
  }
}
