# Noid File Format

This section describes the on-disk file format of noid databases. The data and indexes from a database are
contained in separate B+trees in a single file, preferably named `<database_name>.ndb`.
The associated database structure is contained in the noid main database which resides in a file
called `noid_main.ndb` in the noid data directory.
Although there doesn't seem to be a single paper describing the definitive B+tree algorithm, there are multiple
resources available on this topic. A few of those are referenced next. A general explanation can be obtained from
[Wikipedia](https://en.wikipedia.org/wiki/B%2B_tree). Another description is offered in a lecture on
[GitHub](https://thodrek.github.io/cs564-fall17/lectures/lecture-13/Lecture_13_Btree.pdf). Finally, a visualisation
of B+trees with different properties can be played around with at the
[University of San Fransisco](https://www.cs.usfca.edu/~galles/visualization/BPlusTree.html).

### Database File Locking

The available locking strategies are documented in `file_locking.md`.

## File Header Page

The first 100 bytes of a database file consist of the file header, which contains metadata about the B+tree.
All pages have a size of 4096 bytes, except for the header page which is 100 bytes.
The first tree header page number exists here to enable storage of the database structure within the database file.
This allows for reconstruction of the database without requiring the noid main database.

| offset | size (bytes) | description                                              |
|--------|--------------|----------------------------------------------------------|
| 0      | 8            | Header string `"noid v1\0"`                              |
| 8      | 2            | LE `uint16_t` page size in bytes (min 512, default 4096) |
| 10     | 1            | `uint8_t` key size in bytes (multiple of 8, fixed at 16) |
| 11     | 4            | LE `uint32_t` first tree header page number              |
| 15     | 4            | LE `uint32_t` first freelist page number                 |
| 19     | 4            | LE `uint32_t` checksum (fnv-1a of all preceding bytes)   |
| 23     | 77           | Reserved for later use (zeroed)                          |

## Freelist Page

Freelist pages are pages which list pages previously in use by the database, but are now free for reuse.
Freelist pages are implemented as a doubly linked list, and are cleaned up when the database is compacted.
If the page is full, the value of the next free page slot number is one past the maximum list size.
With a page size of 4096 bytes, this value is 1022.

| offset | size (bytes) | description                                      |
|--------|--------------|--------------------------------------------------|
| 0      | 2            | page type `'FL'` (free list)                     |
| 2      | 4            | LE `uint32_t` previous freelist page number      |
| 6      | 4            | LE `uint32_t` next freelist page number          |
| 10     | 2            | LE `uint16_t` next free page 'slot' in this page |
| 12     | 1021*4       | LE `uint32_t` list of free page numbers          |

## Tree Header Page

The tree header page is the entry point for a single B+tree containing a table or an index. Table trees contain
the actual data, whereas index tables contain no data in their leaf nodes. The last 4088 bytes of this page are
sacrificed so that all pages in a database file have a consistent size, allowing for fast sequential reading.

| offset | size (bytes) | description                                                               |
|--------|--------------|---------------------------------------------------------------------------|
| 0      | 2            | The tree type: `'TT'` (table) or `'TI'` (index)                           |
| 2      | 2            | `uint16_t` max entries in an internal node page (page size-24)/entry size |
| 4      | 2            | `uint16_t` max records in a leaf node page (page size-24)/record size     |
| 6      | 4            | LE `uint32_t` root node page number                                       |
| 10     | 4            | LE `uint32_t` amount of pages in this tree                                |
| 14     | 4082         | Reserved for later use (zeroed)                                           |

## Internal Node Page

The internal node pages contain no data but only keys and references to child nodes.
If the page is full, the value of the next free page slot number is one past the maximum list size.
With a page size of 4096 bytes, this value is 204.

| offset | size (bytes)           | description                              |
|--------|------------------------|------------------------------------------|
| 0      | 2                      | `'IN'` (internal node)                   |
| 2      | 2                      | LE `uint16_t` next free entry slot       |
| 4      | 4                      | LE `uint32_t` leftmost child page number |
| 8      | 16                     | Padding (zeroed)                         |
| 24     | key count * entry size | The node entries                         |

## Internal Node Entry

This is the format of the entries in an internal node.

| offset | size (bytes) | description                           |
|--------|--------------|---------------------------------------|
| 0      | 16           | The key                               |
| 16     | 4            | LE `uint32_t` right child page number |

## Leaf Node Page

The leaf node pages contain the records that contain the actual data.

| offset | size (bytes)               | description                             |
|--------|----------------------------|-----------------------------------------|
| 0      | 2                          | `'LN'` (leaf node)                      |
| 2      | 2                          | `uint16_t` record count                 |
| 4      | 4                          | LE `uint32_t` left sibling page number  |
| 8      | 4                          | LE `uint32_t` right sibling page number |
| 12     | 12                         | Padding (zeroed)                        |
| 24     | record count * record size | The node records                        |

## Leaf Node Record

A leaf node record contains at least the first 3 bytes of the data payload. If the total data size is
at most 7 bytes, the inline indicator is set to the payload size.
Otherwise, the inline indicator is set to zero and the last 4 bytes of the record contain the first overflow
page number.

| offset | size (bytes) | description                                                  |
|--------|--------------|--------------------------------------------------------------|
| 0      | 16           | The key                                                      |
| 16     | 1            | `uint8_t` inline indicator                                   |
| 17     | 3            | 3x `uint8_t` 1st 3 bytes of payload                          |
| 20     | 4            | LE `uint32_t` 2nd 4 bytes of payload or overflow page number |

## Overflow Page

| offset | size (bytes)        | description                             |
|--------|---------------------|-----------------------------------------|
| 0      | 2                   | LE `uint16_t` data size in this page    |
| 2      | 4                   | LE `uint32_t` next overflow page number |
| 6      | data size + padding | The data plus padding (zeroes)          |