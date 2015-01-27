#pragma once

#include <cstdlib>

#include <memory>
#include <iostream>


/******************************** Header part ********************************/

/*
 * Why do we need this? See http://stackoverflow.com/questions/18792565/
 * declare-template-friend-function-of-template-class ...
 */
template <class Code, class Payload>
class Trie;

template <class Code, class Payload>
int reduce_trie(Trie<Code, Payload> const* const trie);

/** Simple trie class that supports a payloading mechanism. */
template <class Code, class Payload>
class Trie {
public:
  Trie(size_t branching, Payload* payload=nullptr);
  ~Trie();

  /**
   * Adds a new branch to this trie. Does nothing if the branch already exists.
   */
  Trie* add(const Code& code, Payload* payload=nullptr);
  
  template <class InputIterator>
  Trie* add_branch(InputIterator first, InputIterator last,
                   Payload* payload=nullptr);

  friend int reduce_trie<Code, Payload>(Trie<Code, Payload> const* const trie);

private:
  /** Sets the payload; used by the constructor & add methods. */
  void set_payload(Payload* payload=nullptr);

  size_t branching;
  std::unique_ptr<std::unique_ptr<Trie>[]> branches;  /* Array of pointers. */
  std::unique_ptr<Payload> payload;
};


template <class Code, class Payload>
int reduce_trie(Trie<Code, Payload> const* const trie);

/**************************** Implementation part ****************************/


template <class Code, class Payload>
Trie<Code, Payload>::Trie(size_t _branching, Payload* _payload) : branching{_branching} {
  branches.reset(new std::unique_ptr<Trie>[_branching]);
  set_payload(_payload);
}
template <class Code, class Payload>
Trie<Code, Payload>::~Trie() {
  std::cout << "Deleting Trie at " << this << "..." << std::endl;
}

template <class Code, class Payload>
Trie<Code, Payload>* Trie<Code, Payload>::add(
    const Code& _code, Payload* _payload) {
  Trie* next = branches.get()[_code].get();
  if (next == nullptr) {
    next = new Trie(branching, _payload);
    branches.get()[_code].reset(next);
  }
  return next;
}

template <class Code, class Payload>
template <class InputIterator>
Trie<Code, Payload>* Trie<Code, Payload>::add_branch(
    InputIterator first, InputIterator last, Payload* _payload) {
  Trie* curr = this;
  while (first != last) {
    curr = curr->add(*first);
    ++first;
  }
  curr->set_payload(_payload);
  return curr;
}

template <class Code, class Payload>
void Trie<Code, Payload>::set_payload(Payload* _payload) {
  payload.reset(_payload);
}

