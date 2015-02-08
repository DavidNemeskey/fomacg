#pragma once

#include <cstdlib>

#include <memory>
#include <set>
#include <vector>

/******************************** Header part ********************************/

/*
 * Why do we need this? See http://stackoverflow.com/questions/18792565/
 * declare-template-friend-function-of-template-class ...
 */
template <class Code, class Payload>
class Trie;

template <class Payload>
struct NoOpPayloadTransformer;

/**
 * Simple trie class that supports a payloading mechanism.
 *
 * @note The trie takes ownership of the payloads passed to it; the user must
 *       not try to delete them.
 */
template <class Code, class Payload>
class Trie {
  class TrieIterator;

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
  inline size_t get_branching() const { return branching; }

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
  inline Payload* match_all(Container c) const {
    return match_all(std::begin(c), std::end(c));
  }

  /**
   * Runs a @c Code sequence through the trie; similar to match_all(), with
   * three important differences:
   * - the sequence does not have to match completely: if a subsequence is
   *   not found in the trie, matching will resume at a later point in the
   *   sequence. E.g. the input {1, 2, 3, 4} will match the branch {1, 4};
   * - all such branches are considered, not just a single one;
   * - all payloads are collected along the matching paths, not just those
   *   at the endpoints of the paths.
   *
   * @tparam PayloadTransformer A class that transforms the payloads found
   *                            and writes them to the output iterator. For
   *                            an example, check out
   *                            ElementPayloadTransformer.
   */
  template <class InputIterator, class OutputIterator,
            class PayloadTransformer>
  void collect(InputIterator first, InputIterator last, OutputIterator out,
               const PayloadTransformer& transformer);

  /**
   * Same as the other collect() method, but defaults to
   * NoOpPayloadTransformer.
   */
  template <class InputIterator, class OutputIterator>
  inline void collect(InputIterator first, InputIterator last,
                      OutputIterator out) {
    collect(first, last, out, NoOpPayloadTransformer<Payload>());
  }

  inline TrieIterator begin() { return TrieIterator(this); }
  inline TrieIterator end() { return TrieIterator(this, branching); }

private:
  /** Sets the payload; used by the constructor & add methods. */
  void set_payload(Payload* payload=nullptr);
  Payload* get_payload() { return payload.get(); }

  size_t branching;
  std::unique_ptr<std::unique_ptr<Trie>[]> branches;  /* Array of pointers. */
  std::unique_ptr<Payload> payload;

  /** Iterates through all child nodes. */
  class TrieIterator : public std::iterator<std::input_iterator_tag, Trie> {
    Trie* trie;
    Code code;
    bool next_called;

  public:
    /** Iterator from a specific branch. */
    TrieIterator(Trie* trie, Code code);
    /** Auto-initialized constructor. */
    TrieIterator(Trie* trie);
    TrieIterator& operator++();
    TrieIterator operator++(int);
    inline bool operator==(const TrieIterator& other) {
      return trie == other.trie && code == other.code;
    }
    inline bool operator!=(const TrieIterator& other) {
      return !(*this == other);
    }
    inline Trie* operator*() { return trie->get(code); }
  };
};

/** Payload transformer that simply puts the payload into the iterator.  */
template <class Payload>
struct NoOpPayloadTransformer {
  template <class OutputIterator>
  inline void transform(Payload* payload, OutputIterator out) const {
    *out = payload;
    ++out;
  }
};

/**
 * Payload transformer that writes all elements of the payload (which must
 * be of a container type) to the output iterator.
 */
template <class Payload>
struct ElementPayloadTransformer {
  template <class OutputIterator>
  inline void transform(Payload* payload, OutputIterator out) const {
    for (auto it = std::begin(*payload); it != std::end(*payload); ++it) {
      *out = *it;
      ++out;
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
Trie<Code, Payload>::~Trie() {}

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
  return add_branch(std::begin(c), std::end(c), payload);
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
template <class InputIterator, class OutputIterator, class PayloadTransformer>
void Trie<Code, Payload>::collect(
    InputIterator first, InputIterator last, OutputIterator out,
    const PayloadTransformer& transformer) {
  std::set<Trie<Code, Payload>*> tries = {this};
  for (auto code = first; code != last; ++code) {
    std::vector<Trie<Code, Payload>*> tries_to_add;
    for (auto t = std::begin(tries); t != std::end(tries); ++t) {
      auto new_trie = (*t)->get(*code);
      if (new_trie != nullptr) tries_to_add.push_back(new_trie);
    }
    tries.insert(std::begin(tries_to_add), std::end(tries_to_add));
  }
  for (auto t = std::begin(tries); t != std::end(tries); ++t) {
    Payload* payload = (*t)->get_payload();
    if (payload != nullptr) {
      transformer.transform(payload, out);
    }
  }
}

template <class Code, class Payload>
void Trie<Code, Payload>::set_payload(Payload* _payload) {
  payload.reset(_payload);
}

template <class Code, class Payload>
Trie<Code, Payload>::TrieIterator::TrieIterator(
    Trie<Code, Payload>* _trie, Code _code)
  : trie(_trie), code(_code), next_called(true) {}

template <class Code, class Payload>
Trie<Code, Payload>::TrieIterator::TrieIterator(
    Trie<Code, Payload>* _trie) : trie(_trie), code(0), next_called(false) {
    operator++();  // Find the first existing branch
}

template <class Code, class Payload>
typename Trie<Code, Payload>::TrieIterator& Trie<Code, Payload>::TrieIterator::operator++() {
  if (next_called) code++;  // Leave the last element found
  while (trie->get(code) == nullptr && code < trie->get_branching()) code++;
  next_called = true;
  return *this;
}

template <class Code, class Payload>
typename Trie<Code, Payload>::TrieIterator Trie<Code, Payload>::TrieIterator::operator++(int) {
  Trie<Code, Payload>::TrieIterator tmp(*this);
  operator++();
  return tmp;
}
