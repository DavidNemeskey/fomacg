#pragma once

/**
 * A C++ implementation of mixed automata (Automata Mista), as described in
 * Huet, Gérard. "Automata mista." Verification: Theory and Practice (2004),
 * pp. 271-273.
 *
 * @author David Mark Nemeskey
 */

#include <vector>
#include <map>

typedef int Letter;
typedef std::vector<Letter> Word;

class Trie;

typedef std::map<Letter, Trie*> ArcMap;

/** A node in the trie. */
struct Trie {
  /** Final state? */
  bool final;

  /** Arcs. */
  ArcMap arcs;

  Trie(bool final, const ArcMap& arcs);

  /** Default constructor. */
  Trie(bool final_=false);
  /**
   * Deletes the whole trie under this.
   * @warning This operation might run out of stack if the trie is huge.
   * @todo Fix it.
   */
  ~Trie();

  /** Checks for membership in the trie. */
  bool mem(Word w);
};

/** Address of a trie. */
typedef Word Address;

class State;

typedef std::map<Letter, State*> DeterMap;

/** A ... state of the FSA? */
struct State {
  bool final;
  DeterMap deter;
  // TODO: add choices; I haven't the faintest idea what should come here.
};

/** Automaton: an array (forest) of addresses. */
struct Automaton {
  /**
   * The state forest of the automaton.
   *
   * Why do we need more than one trie? Quote:
   *
   * "The only problem is that virtual addresses must indeed point to locations
   * accessible from the top node, whereas there exist non-deterministic
   * automata which have no deterministic sub-automaton spanning the whole space
   * set. In that case, we may still represent faithfully the non-deterministic
   * automaton by considering a forest of trees, rather than a single tree."
   */
  std::vector<State> states;
  /** The address of the initial state. */
  Address initial;
};

