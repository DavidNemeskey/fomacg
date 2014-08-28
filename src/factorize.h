#pragma once

/**
 * Bimachine factorization for CG rules. The implementation is based on
 * Roche, E. and Schabes, Y. Finite State Language Processing. 1997.
 *
 * @author Dávid Márk Nemeskey
 */
#include <map>
#include <set>
#include <iostream>
#include <vector>

/** The state type in foma. */
typedef int State;
/** The Sigma symbol type in foma. */
typedef short int SigmaSymbol;

/** A full transition tuple in a transducer: source, target, in, out. */
struct Transition {
  State source;
  State target;
  SigmaSymbol in;
  SigmaSymbol out;

  Transition(State source_, State target_, SigmaSymbol in_, SigmaSymbol out_)
    : source(source_), target(target_), in(in_), out(out_) {}

  inline bool operator<(const Transition& other) const {
    if (source < other.source) {
      return true;
    } else if (source == other.source) {
      if (target < other.target) {
        return true;
      } else if (target == other.target) {
        if (out < other.out) {
          return true;
        } else if (out == other.out) {
          if (in < other.in) {
            return true;
          }
        }
      }
    }
    return false;
  }

  friend std::ostream& operator<<(std::ostream& os, const Transition& t);
};

inline std::ostream& operator<<(std::ostream& os, const Transition& t) {
  os << "Transition(" << t.source << ", "  << t.target << ", " << t.in
     << ", " << t.out << ")";
  return os;
}

/** One half of a transition: the source state and the input symbol. */
struct Trans {
  State  source;
  SigmaSymbol in;

  Trans(State source_, SigmaSymbol in_) : source(source_), in(in_) {}

  inline bool operator<(const Trans& other) const {
    return source < other.source ? true :
           source == other.source && in < other.in ? true : false;
  }
};

/** Used as key in the bimachine's transition table. */
struct BiTrans {
  State  q_1;
  SigmaSymbol a;
  State  q_2;

  BiTrans(State q_1_, SigmaSymbol a_, State q_2_) : q_1(q_1_), a(a_), q_2(q_2_) {}

  inline bool operator<(const BiTrans& other) const {
    if (q_1 < other.q_1) {
      return true;
    } else if (q_1 == other.q_1) {
      if (a < other.a) {
        return true;
      } else if (a == other.a) {
        return q_2 < other.q_2;
      }
    }
    return false;
  }

  friend std::ostream& operator<<(std::ostream& os, const BiTrans& bit);
};

inline std::ostream& operator<<(std::ostream& os, const BiTrans& bit) {
  os << "BiTrans(" << bit.q_1 << ", "  << bit.a << ", " << bit.q_2 << ")";
  return os;
}

/** Represents a bimachine as a 3-tuple <tt>(A_1, A_2, delta)</tt>. */
struct BiMachine {
  BiMachine(struct fsm* fst);
  ~BiMachine();

  /**
   * Apply down through the bimachines.
   * @todo We won't need this...
   */
  std::vector<SigmaSymbol> bi_apply_down(std::vector<SigmaSymbol> input);
  /** Finds the cell in delta that fits @p bit1, and if there is none, @p bit2. */
  std::map<BiTrans, SigmaSymbol>::const_iterator
    find_delta_cell(const BiTrans& bit1, const BiTrans& bit2);

  struct fsm* A_1;
  struct fsm* A_2;
  std::map<BiTrans, SigmaSymbol> delta;

private:
  /**
   * Computes A_1 (see page 66 of the book).
   * @return C_1
   */
  std::vector<std::set<State> > compute_A_1(const struct fsm* fst);
  /**
   * Computes A_2 (see page 66 of the book). Almost a verbatim copy of
   * compute_A_1().
   * @return C_2
   */
  std::vector<std::set<State> > compute_A_2(const struct fsm* fst);
  /** Computes the transition mapping of the bimachine. */
  void compute_delta(const struct fsm* fst, std::vector<std::set<State> > C_1,
                                            std::vector<std::set<State> > C_2);

  /**
   * Adds @p x if it is not containted in @p C.
   * 
   * @return the index of @p x in @p C.
   */
  int add_set(std::vector<std::set<State> >& C, const std::set<State>& x);
  /**
   * Creates a foma FSA object from
   * @param[in] name the FSA's name,
   * @param[in] d_A its transitions,
   * @param[in] C its states (as a new->old state mapping),
   * @param[in] A_sigma and sigma;
   * @param[in] new_finals the set of final states in A,
   * @param[in] eof_states the set of end-of-path states (i.e. those that have
   *                       no outedges). Needed so that we can correctly pre-
   *                       allocate the space.
   * @param[in] fst the original FST.
   */
  struct fsm* create_fsa(const std::string& name,
                         std::map<Trans, State> d_A,
                         std::vector<std::set<State> > C,
                         std::set<SigmaSymbol> A_sigma,
                         std::set<State> new_finals,
                         std::set<State> eof_states,
                         const struct fsm* fst);
};

/** Left- and right-sequential transducer representation of a bimachine. */
struct LeftRightSequential {
  /** Constructs the left- and right-sequential FST pair. */
  LeftRightSequential(const struct fsm* fst, const BiMachine& bimachine);
  /**
   * Loads the left- and right-sequential FST pair. This constructor takes
   * ownership for @p T_1 and @p T_2, and deletes them in the destructor.
   */
  LeftRightSequential(struct fsm* T_1, struct fsm* T_2);
  ~LeftRightSequential();

  /** Apply down through the transducers T_1 and T_2. */
  std::vector<SigmaSymbol> ts_apply_down(std::vector<SigmaSymbol> input);

  struct fsm* T_1;
  struct fsm* T_2;

private:
  /**
   * Computes the transducers from the automata and @c delta of @p bimachine.
   * @p fst is required so that we have access to the sigma alphabet.
   *
   * The size of the temporary alphabet between T_1 and T_2 is the size of the
   * sparse representation of the Sigma_A_1 x Q_A_1 matrix -- possibly larger
   * than the number of non-zero cells in @c delta.
   */
  void compute_ts(const struct fsm* fst, const BiMachine& bimachine);
};

/** Converts a transducer to left-right sequential transducer. Deletes @p fst. */
LeftRightSequential* fst_to_left_right(struct fsm* fst);
