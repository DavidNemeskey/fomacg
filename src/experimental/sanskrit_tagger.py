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
        self._final = final
        self._arcs = arcs
        self._trid = Trie.num_tries
        Trie.num_tries += 1

    def __repr__(self):
        return "Trie[{2}]({0}, {1})".format(self._final, self._arcs, self._trid)

    def __eq__(self, other):
        return self._final == other._final and self._arcs == other._arcs

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

############ End trie

############ Start sharing

# A { key : bucket } dict
memo = {}

def share(element, key):
    """
    Looks in k-th bucket and returns y in it such that y = x if it exists,
    otherwise returns x memorized in the new k-th bucket [x :: e].
    """
    bucket = memo.get(key)
    if bucket is None:
        bucket = []
        memo[key] = bucket
    try:
        return bucket[bucket.index(element)]
    except ValueError:
        bucket.append(element)
        return element

hash0 = 1
hash_max = 150

# The hash functions.
def hash1(letter, key, sum):
    return sum + letter * key

def hash(final, num_arcs):
    return (num_arcs + 1 if final else 0) % hash_max

def traverse(lookup, trie):
    def travel(trie):
#        print "travel({0}) start".format(trie)
        final, arcs = trie._final, trie._arcs
        def f(tries_span, n_t):  # WTF?
#            print "AHA"
            tries, span = tries_span
            n, t        = n_t
#            print "f1([tries={0}, span={1}], [n={2}, t={3}])".format(
#                    tries, span, n, t)
            t0, k       = travel(t)
#            print "f2([tries={0}, span={1}], [n={2}, t={3}]): t0={4}, k={5}".format(
#                    tries, span, n, t, t0, k)
#            print "f3([tries={0}, span={1}], [n={2}, t={3}]): t0={4}, k={5}, f={6}".format(
#                    tries, span, n, t, t0, k, ([(n, t0)] + tries, hash1(n, k, span)))
            return ([(n, t0)] + tries, hash1(n, k, span))
#        print "reducing with {0}, {1}".format(([], hash0), list(arcs.iteritems()))
        arcs0, span = reduce(f, arcs.iteritems(), ([], hash0))
#        print "After reduce: {0}/{1}, {2}/{3}".format(type(arcs0), arcs0,
#                                                      type(span), span)
        key = hash(final, span)
#        print "Key: {0}".format(key)
#        print "travel({0}) end".format(trie)
        return ( lookup(Trie(final, dict(arcs0)), key), key )
    return travel(trie)

def minimize(trie):
    dag, _ = traverse(share, trie)
    return dag

def memo_size():
    """Returns the size of @c memo."""
    return sum(len(bucket) for bucket in memo.values())

############ End sharing

def count_unique(trie):
    """Counts the unique Trie objects in @p trie."""
    visited = set()
    def __count_unique(trie):
        visited.add(trie)
        for next in trie._arcs.values():
            if next not in visited:
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

if __name__ == '__main__':
#    test_trie()
    import sys
    test_word_list(sys.argv[1])
