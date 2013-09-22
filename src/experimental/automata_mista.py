# typedef int Letter
# typedef tuple(Letter) Word

# A node in the trie.
class Trie(object):
    # @param final Final state?
    # @param arcs The arcs: a {Letter : Trie} dict.
    def __init__(self, final=False, arcs={}):
        self._final = final
        self._arcs = arcs

    # Looks up @p word in this trie.
    # @return @c True, if it could; @c False otherwise.
    def mem(self, word):
        curr = self
        for letter in word:
            next = curr._arcs.get(letter)
            if next is not None:
                curr = next
            else:
                return False
        return curr._final


# Path is really just a tuple, but it's better if we know it when we use one.
class Path(tuple):
    def __new__(cls, *args):
        return tuple.__new__(cls, args)

# Address of a trie.
class Address(object):
    # @param tree_index the index of the tree in the automaton forest
    # @param path the path in the tree
    # @see Automaton
    def __init__(self, tree_index, path):
        self._tree_index = tree_index
        self._path       = path

class State(object):
    # @param final Final state?
    # @param deter A {Letter : State} dict.
    # @param choices A {Word : Address} dict.
    def __init__(self, final=True, deter={}, choices={}):
        self._final   = final
        self._deter   = deter
        self._choices = choices

# An automaton.
#
# Why do we need more than one trie? Quote:
#
# "The only problem is that virtual addresses must indeed point to locations
# accessible from the top node, whereas there exist non-deterministic
# automata which have no deterministic sub-automaton spanning the whole space
# set. In that case, we may still represent faithfully the non-deterministic
# automaton by considering a forest of trees, rather than a single tree."
class Automaton(object):
    # @param states The state forest of the automaton.
    # @param initial The index of the initial state in @c states.
    def __init__(self, states, initial):
        self._states  = states
        self._initial = initial

if __name__ == '__main__':
    trie = Trie(False, {1: Trie(False, {2: Trie(True)}),
                        2: Trie(True,  {2: Trie(True),
                                        3: Trie(True)})})
    for w in [[1, 2], [2, 2], [2, 1], [1, 2, 3]]:
        print "{0} in trie: {1}".format(''.join(str(i) for i in w), trie.mem(w))

    state = State(False, {1: State(True, {}, {(3,): Address(0, Path(1))}),
                          2: State(True, {}, {(3,): Address(0, Path(2))})})
                                                   # should be (1,) for sharing
