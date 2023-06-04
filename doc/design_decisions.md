# Design Decisions

## Design Patterns

### Builder Pattern

The on-disk data structures are represented at runtime by types in the `noid::backend::page` namespace.
Instances of these types can only be created using the
[Builder](https://en.wikipedia.org/wiki/Builder_pattern) pattern.
Using this pattern has advantages which outweigh its possible disadvantages:

* It helps guarantee the immutability of these types;
* It allows for batch updates while not having to copy the structure for every update in between;
* Although currently not implemented, it supports different building strategies without the user
  having to care.

## C++ Concepts and Pure Virtual Classes
