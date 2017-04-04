Heist immutable/functional data structure library
Copyright (C) 2016-2017 by Stephen Blackheath
Released under a BSD3 licence

Immutable data structures, used for functional programming, or whenever you want
your code to better, cleaner and more reliable.

Passing the whole data structure by value is cheap (unlike std::set, etc).

It's all based on the 2-3 which is self-balancing. Most operations are O(log N)
just like mutable data structures.

When you insert an item, both the old structure (before the insert) and the new
one (after the insert) are valid. The memory consumed is only the difference
between the two. Whichever you don't ultimately want gets cleaned up when you
don't reference it any more.

One major advantage is that you can pass these between threads without having to
care at all about synchronizing between the threads. The producer can race
ahead modifying its own copy secure in the knowledge that the consumer has only
the version it was sent. Race conditions are absolutely impossible.

The interface is pretty hard to stuff things up in. For example, an iterator
that points nowhere is a concept that doesn't exist (unlike std::set, etc).

I hope you enjoy writing better code!
