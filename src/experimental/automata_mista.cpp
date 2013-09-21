/**
 * A C++ implementation of mixed automata (Automata Mista), as described in
 * Huet, Gérard. "Automata mista." Verification: Theory and Practice (2004),
 * pp. 271-273.
 *
 * @author David Mark Nemeskey
 */

#include "automata_mista.h"
#include <cstdlib>
#include <iostream>

Trie::Trie(bool final_, const ArcMap& arcs_)
  : final(final_), arcs(arcs_) {}

Trie::Trie(bool final_) : final(final_) {}

Trie::~Trie() {
  for (ArcMap::const_iterator it = arcs.begin(); it != arcs.end(); ++it) {
    delete it->second;
  }
  arcs.clear();
}

bool Trie::mem(Word w) {
  const Trie* curr = this;
  for (size_t i = 0; i < w.size(); i++) {
    ArcMap::const_iterator next = curr->arcs.find(w[i]);
    if (next != curr->arcs.end()) {
      curr = next->second;
    } else {
      return false;
    }
  }
  return curr->final;
}
