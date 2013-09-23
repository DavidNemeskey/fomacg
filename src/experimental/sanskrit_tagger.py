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

class Trie(object):
    """A node in the trie."""
    def __init__(self, final=False, arcs={}):
        """
        @param final Final state?
        @param arcs The arcs: a {Letter : Trie} dict.
        """
        self._final = final
        self._arcs = arcs

    def __repr__(self):
        return "Trie({0}, {1})".format(self._final, self._arcs)

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
                curr = curr._arcs[c]
            except KeyError:
                next = Trie(False, {})
                curr._arcs[c] = next
                curr = next
        curr._final = True
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
        if trie._final:
            ret.append(''.join(chr(c) for c in word))
        for letter, next in trie._arcs.iteritems():
            traverse(next, word + [letter])
    traverse(trie, [])
    return ret

def mem(trie, word):
    curr = trie
    for letter in word:
        next = curr._arcs.get(letter)
        if next is not None:
            curr = next
        else:
            return False
    return curr._final

def test_trie():
    trie = Trie(False, {1: Trie(False, {2: Trie(True, {})}),
                        2: Trie(True,  {2: Trie(True, {}),
                                        3: Trie(True, {})})})
    print trie
    print enter(enter(enter(enter(trie_of([2, 2]), [1, 2]), [2, 3]), [2]), [2])

    t = make_lex(['abc', 'def', 'ab', 'defg'])
    print t
    print contents(t)
    for w in [[1, 2], [2, 2], [2, 1], [1, 2, 3]]:
        print "{0} in trie: {1}".format(''.join(str(i) for i in w), mem(trie, w))

if __name__ == '__main__':
    test_trie()
