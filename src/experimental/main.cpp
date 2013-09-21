#include "automata_mista.h"

#include <iostream>

int main(int argc, char* argv[]) {
  Trie* t4 = new Trie(true);
  Trie* t5 = new Trie(true);
  Trie* t6 = new Trie(true);
  ArcMap m2;
  m2[2] = t4;
  Trie* t2 = new Trie(false, m2);
  ArcMap m3;
  m3[2] = t5;
  m3[3] = t6;
  Trie* t3 = new Trie(true, m3);
  ArcMap m1;
  m1[1] = t2;
  m1[2] = t3;
  Trie* t1 = new Trie(false, m1);

  std::vector<Letter> w1;
  w1.push_back(1);
  w1.push_back(2);
  std::cout << "12 in trie: " << t1->mem(w1) << std::endl;
  std::vector<Letter> w2;
  w2.push_back(2);
  w2.push_back(2);
  std::cout << "22 in trie: " << t1->mem(w2) << std::endl;
  std::vector<Letter> w4;
  w4.push_back(2);
  w4.push_back(1);
  std::cout << "21 in trie: " << t1->mem(w4) << std::endl;
  std::vector<Letter> w3;
  w3.push_back(1);
  w3.push_back(2);
  w3.push_back(3);
  std::cout << "123 in trie: " << t1->mem(w3) << std::endl;
  delete t1;
}

