#include "tester_trie.h"

#include <array>
#include <set>
#include <iterator>
#include <algorithm>
#include <iostream>

typedef Trie<int, std::set<int> > Tree;

template <>
std::set<int> reduce_trie<int, std::set<int>>(Tree const* const trie) {
  return std::set<int>{(int)trie->branching};
}

int main(int argc, char* argv[]) {
  Tree t(1024);
  Tree* t2 = t.add(3, nullptr);
  Tree* t3 = t2->add(15, nullptr);
  std::array<size_t, 3> arr = {1, 2, 3};
  std::set<int>* pl = new std::set<int>{4, 5, 6};
  t.add_branch(begin(arr), end(arr), pl);
  std::set<int> red = reduce_trie(&t);
  std::copy(red.begin(), red.end(), std::ostream_iterator<int>(std::cout, " "));
}
