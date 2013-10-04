// Multiple live iterators on the same Set can cope with removing entries.

load(libdir + "iteration.js");

// Make a set.
var set = Set();
var SIZE = 7;
for (var j = 0; j < SIZE; j++)
    set.add(j);

// Make lots of iterators pointing to entry 2 of the set.
var NITERS = 5;
var iters = [];
for (var i = 0; i < NITERS; i++) {
    var iter = set[std_iterator]();
    assertIteratorResult(iter.next(), 0, false);
    assertIteratorResult(iter.next(), 1, false);
    iters[i] = iter;
}

// Remove half of the set entries.
for (var j = 0; j < SIZE; j += 2)
    set.delete(j);

// Make sure all the iterators still work.
for (var i = 0; i < NITERS; i++) {
    var iter = iters[i];
    for (var j = 3; j < SIZE; j += 2)
        assertIteratorResult(iter.next(), j, false);
    assertIteratorResult(iter.next(), undefined, true);
}
