#pragma once

#include <cstdlib>

#include <memory>
#include <set>
#include <vector>
#include <iostream>


/******************************** Header part ********************************/

/*
 * Why do we need this? See http://stackoverflow.com/questions/18792565/
 * declare-template-friend-function-of-template-class ...
 */
template <class Code, class Payload>
class Trie;

template <class Code, class Payload>
class TrieCollector;

/**
 * Simple trie class that supports a payloading mechanism.
 *
 * @note The trie takes ownership of the payloads passed to it; the user must
 *       not try to delete them.
 */
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
  
  template <class Container>
  inline Trie* add_branch(Container c, Payload* payload=nullptr);

  inline Trie* get(const Code& code) const;

  /**
   * Runs the @c Code sequence from @p first to @p last through the trie and
   * returns the payload at the end of it.
   */
  template <class InputIterator>
  Payload* match_all(InputIterator first, InputIterator last) const;

  /**
   * Runs the @c Code sequence in container @p c through the trie and
   * returns the payload at the end of it.
   */
  template <class Container>
  inline Payload* match_all(Container c) const;

  friend class TrieCollector<Code, Payload>;

private:
  /** Sets the payload; used by the constructor & add methods. */
  void set_payload(Payload* payload=nullptr);
  Payload* get_payload() { return payload.get(); }

  size_t branching;
  std::unique_ptr<std::unique_ptr<Trie>[]> branches;  /* Array of pointers. */
  std::unique_ptr<Payload> payload;
};

template <class Code, class Payload>
class TrieCollector {
public:
  template <class InputIterator, class OutputIterator>
  void collect(Trie<Code, Payload>* trie, InputIterator first,
               InputIterator last, OutputIterator out) {
    std::set<Trie<Code, Payload>*> tries = {trie};
    for (auto code = first; code != last; ++code) {
      std::vector<Trie<Code, Payload>*> tries_to_add;
      for (auto t = begin(tries); t != end(tries); ++t) {
        auto new_trie = (*t)->get(*code);
        if (new_trie != nullptr) tries_to_add.push_back(new_trie);
      }
      tries.insert(begin(tries_to_add), end(tries_to_add));
    }
    for (auto t = begin(tries); t != end(tries); ++t) {
      Payload* payload = (*t)->get_payload();
      if (payload != nullptr) {
        *out = payload;
        ++out;
      }
    }
  }
};


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
inline Trie<Code, Payload>* Trie<Code, Payload>::get(const Code& code) const {
  return branches.get()[code].get();
}

template <class Code, class Payload>
Trie<Code, Payload>* Trie<Code, Payload>::add(
    const Code& _code, Payload* _payload) {
  Trie* next = get(_code);
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
template <class Container>
inline Trie<Code, Payload>* Trie<Code, Payload>::add_branch(
    Container c, Payload* payload) {
  return add_branch(begin(c), end(c), payload);
}

template <class Code, class Payload>
template <class InputIterator>
Payload* Trie<Code, Payload>::match_all(
    InputIterator first, InputIterator last) const {
  const Trie* curr = this;
  while (first != last && curr != nullptr) {
    curr = curr->branches.get()[*first].get();
    ++first;
  }
  return curr != nullptr ? curr->payload.get() : nullptr;
}

template <class Code, class Payload>
template <class Container>
inline Payload* Trie<Code, Payload>::match_all(Container c) const {
  return match_all(begin(c), end(c));
}

template <class Code, class Payload>
void Trie<Code, Payload>::set_payload(Payload* _payload) {
  payload.reset(_payload);
}

