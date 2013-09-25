"""
Implementation of the then-unnamed Automata Mista from another of Huet's paper:
A Functional Toolkit for Morphological and Phonological Processing, Application
to a Sanskrit Tagger, 2004.
"""

from collections import namedtuple

# typedef int Letter
# typedef Word list(Letter)

# arcs = list/map(Letter x Trie)
#Trie = namedtuple('Trie', ['final', 'arcs'])

############ Start trie

class Trie(object):
    num_tries = 0
    """A node in the trie."""
    def __init__(self, final=False, arcs={}):
        """
        @param final Final state?
        @param arcs The arcs: a {Letter : Trie} dict.
        """
        self.final = final
        self.arcs = arcs
        self.trid = Trie.num_tries
        self.__hash = id(self)
        Trie.num_tries += 1

    def compute_hash(self):
        """
        Computes the hash for this Trie (and recursively all tries under it).
        Execute once before the hash values are needed; the values will be
        valid until @c arcs is modified.

        @todo This method only works for tries with no cycles in them, so it
              won't work on a regular FSA!
        """
        self.__hash = hash(tuple(sorted((k, v.compute_hash())
                           for k, v in self.arcs.iteritems())))
        self.__hash = 3 * self.__hash + self.final
        return self.__hash

    def __repr__(self):
        return "Trie[{2}]({0}, {1})".format(self.final, self.arcs, self.trid)

    def __eq__(self, other):
        return self.final == other.final and self.arcs == other.arcs

    def __hash__(self):
        return self.__hash

def trie_of(w):
    """Returns the singleton trie containing @p c."""
    t = Trie(True, {})
    for c in reversed(w):
        t = Trie(False, {c: t})
    return t

# Shame or not, I didn't understand the Zipper part; the ML syntax is confusing
# and the explanation of what the code does is subpar. So here is my own enter
# function.

def enter(t, w):
    """Adds word @p w to trie @p t."""
    if t is None:
        return trie_of(w)
    else:
        curr = t
        for c in w:
            try:
                curr = curr.arcs[c]
            except KeyError:
                next = Trie(False, {})
                curr.arcs[c] = next
                curr = next
        curr.final = True
        return t

def make_lex(words):
    """
    Creates a trie from a list of words.
    """
    t = None
    for word in words:
        t = enter(t, [ord(c) for c in word])
    return t

def contents(trie):
    """Lists the contents of a trie in lexicographic order."""
    ret = []
    def traverse(trie, word):
        if trie.final:
            ret.append(''.join(chr(c) for c in word))
        for letter, next in sorted(trie.arcs.iteritems()):
            traverse(next, word + [letter])
    traverse(trie, [])
    return ret

def mem(trie, word):
    curr = trie
    for letter in word:
        next = curr.arcs.get(letter)
        if next is not None:
            curr = next
        else:
            return False
    return curr.final

############ End trie

############ Start sharing

def minimize(trie):
    """
    A much faster minimization method than the one in the paper that uses the
    built-in hash map.
    """
    trie.compute_hash()
    # A { key : bucket } dict
    memo = {}
    def __minimize(trie):
        if trie not in memo:
            min_trie = Trie(trie.final)
            min_trie.arcs = dict((k, __minimize(v)) for k, v in trie.arcs.iteritems())
            memo[trie] = min_trie
        else:
            min_trie = memo[trie]
        return min_trie
    return __minimize(trie)

############ End sharing

def count_unique(trie):
    """Counts the unique Trie objects in @p trie."""
    visited = set()
    def __count_unique(trie):
        visited.add(id(trie))
        for next in trie.arcs.values():
            if id(next) not in visited:
                __count_unique(next)
    __count_unique(trie)
    return len(visited)

def test_trie():
    trie = Trie(False, {1: Trie(False, {2: Trie(True, {})}),
                        2: Trie(True,  {2: Trie(True, {}),
                                        3: Trie(True, {})})})
    print trie
    trie2 = enter(enter(enter(enter(enter(trie_of([2, 2]), [1, 2]), [2, 3]), [2]), [2, 2, 2]), [1, 2, 2])
    print trie2, count_unique(trie2)

    trie_min = minimize(trie2)
    print "min trie\n{0}".format(trie_min), count_unique(trie_min)

    t = make_lex(['abc', 'def', 'ab', 'defg'])
    print t
    print contents(t)
    for w in [[1, 2], [2, 2], [2, 1], [1, 2, 3]]:
        print "{0} in trie: {1}".format(''.join(str(i) for i in w), mem(trie, w))

def test_word_list(word_file):
    print "Reading file..."
    with open(word_file, 'r') as inf:
        words = [word.strip() for word in inf.readlines()]
    print "Making trie..."
    trie = make_lex(words)
    print "Minimizing..."
    min_trie = minimize(trie)
    print "Number of words: {0}, {1} raw: {2}, minimized: {3}\n".format(
            len(contents(trie)), len(contents(min_trie)),
            count_unique(trie), count_unique(min_trie))
#    for word in contents(min_trie):
#        print word

if __name__ == '__main__':
#    test_trie()
    import sys
    test_word_list(sys.argv[1])
