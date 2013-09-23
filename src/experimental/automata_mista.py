# typedef int Letter
# typedef tuple(Letter) Word

from collections import namedtuple

class Trie(object):
    """A node in the trie."""
    def __init__(self, final=False, arcs={}):
        """
        @param final Final state?
        @param arcs The arcs: a {Letter : Trie} dict.
        """
        self._final = final
        self._arcs = arcs

    def mem(self, word):
        """
        Looks up @p word in this trie.
        @return @c True, if it could; @c False otherwise.
        """
        curr = self
        for letter in word:
            next = curr._arcs.get(letter)
            if next is not None:
                curr = next
            else:
                return False
        return curr._final


class Path(tuple):
    """Path is just a tuple, but it's better if we know it when we use one."""
    def __new__(cls, *args):
        return tuple.__new__(cls, args)

class Address(object):
    """
    Address in the automaton. There are two types of addresses: global and
    local; these are implemented by the subclasses. This object has two members:
    a number and a word. It is up to the subclasses to define how they interpret
    these fields.
    """
    def num(self):
        """Returns the number field."""
        pass
    def word(self):
        """Returns the word field."""
        pass

class Global(Address):
    def __init__(self, tree_index, path):
        """
        @param tree_index the index of the tree in the automaton forest
        @param path the path in the tree
        @see Automaton
        """
        self._tree_index = tree_index
        self._path       = path

    def num(self):
        return self._tree_index
    def word(self):
        return self._path

class Delta(object):
    """
    A differential word (|w1|, w2) between w and w', where w = p.w1, w' = p.w2.
    """
    def __init__(self, len_w1, w2):
        self._len_w1 = len_w1
        self._w2     = w2
    
    @staticmethod
    def diff(word1, word2):
        max_len = min(len(word1), len(word2))
        for i in xrange(max_len):
            if word1[i] != word2[i]:
                return Delta(len(word1) - i, word2[i:])
        else:
            if max_len == len(word1):
                return Delta(0, word2[max_len:])
            else:
                return Delta(len(word1) - max_len, [])

    def patch(self, word1):
        """Patches @p word1 so that it becomes word2."""
        return word1[:len(word1) - self._len_w1] + self._w2

    def __repr__(self):
        return "({0}, {1})".format(self._len_w1,
                                   ''.join(str(e) for e in self._w2))

class Local(Address, Delta):
    """
    Local addresses are really just Deltas; they move around in the current
    @b deterministic tree.
    """
    def __init__(self, len_w1, w2):
        Delta.__init__(self, len_w1, w2)

    def num(self):
        return self._len_w1
    def word(self):
        return self._w2
       
class State(object):
    def __init__(self, final=True, deter={}, choices={}):
        """
        @param final Final state?
        @param deter A {Letter : State} dict.
        @param choices A {Word : Address} dict.
        """
        self._final   = final
        self._deter   = deter
        self._choices = choices

class Automaton(object):
    """
    An automaton.

    Why do we need more than one trie? Quote:

    "The only problem is that virtual addresses must indeed point to locations
     accessible from the top node, whereas there exist non-deterministic
     automata which have no deterministic sub-automaton spanning the whole space
     set. In that case, we may still represent faithfully the non-deterministic
     automaton by considering a forest of trees, rather than a single tree."
    """
    def __init__(self, forest, init_root, init_path):
        """
        @param states The state forest of the automaton.
        @param init_root The index of the initial state in @c states.
        @param init_path The path of the initial state in @c forest[init_root].
        """
        self._forest    = forest
        self._init_root = init_root
        self._init_path = init_path

class PathError(Exception):
    """Thrown for illegal path operations."""
    pass

def access(state, letter):
    """
    Reads @p letter in @p state and returns the new state, or @c None, if no
    such transition exists.
    @todo To ExecutionState, if everything works?
    """
    return state._deter.get(letter)

def push(word, state, states):
    """
    Reads @p word and traverses the automaton accordingly.
    @param word the input word.
    @param state the current state.
    @param states the state stack.
    @return the new values of @p state and @p states as a tuple.
    @todo Returns (state, states) if @p word causes a loop AND if @p word is not
          a valid input; FIX IT.
    @todo To ExecutionState, if everything works.
    """
    new_stack = []
    for letter in word:
        new_state = access(state, letter)
        if new_state is None:
            return (state, states)
        else:
            new_stack.append(state)
            state = new_state
    else:
        return (state, states + new_stack)

def pop(n, state, states):
    """
    Pops @p n elements from the @p states stack.
    @param n the number of elements to pop.
    @param state unused.
    @param states the state stack.
    @return the new values of @p state and @p states as a tuple.
    @todo Remove @p state from the list of arguments.
    @todo To ExecutionState, if everything works.
    """
    if len(states) < n:
        raise PathError
    return (states[-n], states[:-n])

class Finished(Exception):
    """Thrown when we don't accept the input."""
    pass

class ReactiveEngine(object):
    """The Reactive Engine that executes the matching process."""

    # Backtrack type
    BackTrack = namedtuple('BackTrack', ['input', 'state', 'states', 'choices'])

    def __init__(self, automaton):
        self._automaton  = automaton

    def react(self, input, res, state, states):
        """
        Does the main work in reacting to an input.
        @param input the current input
        @param res the resumption stack.
        @param state the current state.
        @param states the state stack.
        @return the new resumption stack.
        """
        b, det, choices = state._final, state._deter, state._choices

        def deter(cont):
            if len(input) == 0:
                return self.backtrack(cont)
            else:
                letter, rest = input[0], input[1:]
                try:
                    new_state  = det[letter]
                    # TODO: append?
                    new_states = [state] + states
                    return self.react(rest, cont, new_state, new_states)
                except KeyError:
                    return self.backtrack(cont)

        if len(choices) == 0:
            new_res = res
        else:
            # TODO: append?
            new_res = [BackTrack(input, state, states, choices)] + res

        if b and len(input) == 0:
            return new_res  # Solution
        else:
            return deter(new_res)

    def backtrack(self, cont):
        """
        As its name implies. Backtracks to the last decision point.
        @param cont res.
        @return the new resumption stack.
        """
        if len(cont) == 0:
            raise Finished()
        else:
            # TODO: -append?
            bt, res = cont[0], cont[1:]
            return self.choose(bt.input, res, bt.state, bt.states, bt.choices)

    def choose(self, input, res, state, states, choices):
        if len(choices) == 0:
            return self.backtrack(res)
        else:
            # TODO: -append?
            w, addr = choices[0]
            # TODO: What is choices anyway?
            ch      = choices[1:]

            # TODO: append?
            new_res = [BackTrack(input, state, states, ch)] + res
            if prefix(w, input):
                new_input = input[len(w):]
                new_state, new_states = self.transition(state, states, addr)
                return self.react(new_input, new_res, new_state, new_states)
            else:
                return self.backtrack(new_res)

    def transition(self, state, states, address):
        """
        Executes a transition defined by its address argument.
        @return The new value of state and states.
        @todo What happens if @p address is not valid?
        """
        if isinstance(address, Global):
            # Jump to the n'th tree in the forest, and apply the path on it
            return push(address.word(), self._automaton._forest[address.num()],
                        [])
        else:
            # Move around in the current tree
            s, ls = pop(address.num(), state, states)
            return push(address.word(), s, ls)

    def recognize(self, w):
        """
        The main method of the engine: tells whether the automaton recognized
        the input @p w.
        """
        init_state, init_states = push(
                self._automaton._init_path, self._automaton._init_root, [])
        try:
            self.react(w, [], init_state, init_states)
            return True
        except Finished:
            return False

def prefix(u, v):
    """
    Service routine that checks the prefix relation between words.
    @return @c True if @p u is a prefix of (or IS) @p v; @c False otherwise.
    """
    return u == v[:len(u)]

def advance(n, word):
    """Service routine that advances the input tape by @p n characters."""
    if n >= len(word):
        return []
    else:
        return word[n:]

def test_trie():
    trie = Trie(False, {1: Trie(False, {2: Trie(True)}),
                        2: Trie(True,  {2: Trie(True),
                                        3: Trie(True)})})
    for w in [[1, 2], [2, 2], [2, 1], [1, 2, 3]]:
        print "{0} in trie: {1}".format(''.join(str(i) for i in w), trie.mem(w))

def test_state():
    state = State(False, {1: State(True, {}, {(3,): Address(0, Path(1))}),
                          2: State(True, {}, {(3,): Address(0, Path(2))})})
                                                   # should be (1,) for sharing

def test_delta():
    import itertools
    ls = [ [1, 2, 3], [1, 3], [1, 2], [2, 3] ]
    for l1, l2 in itertools.product(ls, repeat=2):
        diff = Delta.diff(l1, l2)
        print "diff ({0}, {1}): {2} -> {3}".format(
                ''.join(str(e) for e in l1), ''.join(str(e) for e in l2),
                diff, ''.join(str(e) for e in diff.patch(l1)))


if __name__ == '__main__':
    test_trie()
