## noid
`noid` is a key-value store. Its name is pronounced like /nəʊ aɪˈdɪə/, or *no idea*.
That is because we have no idea if `noid` will ever outgrow the pet-project-stage.

### Design Patterns
#### Builder pattern
The on-disk data structures are represented at runtime by types in the `noid::backend::page` namespace. 
Instances of these types can only be created using the [Builder](https://en.wikipedia.org/wiki/Builder_pattern) 
pattern. Using this pattern has advantages which outweigh its possible disadvantages:
* It helps guarantee the immutability of these types;
* It allows for batch updates while not having to copy the structure for every update in between;
* Although currently not implemented, it supports different building strategies without the user having to care. 

### Noid File Format
This paragraph describes the on-disk file format of noid databases. The data and indexes from a database are 
contained in separate B+trees in a single file, preferably named `<database_name>.ndb`. 
The associated database structure is contained in the noid main database which resides in a file 
called `noid_main.ndb` in the noid data directory.
Although there doesn't seem to be a single paper describing the definitive B+tree algorithm, there are multiple 
resources available on this topic. A few of those are referenced next. A general explanation can be obtained from 
[Wikipedia](https://en.wikipedia.org/wiki/B%2B_tree). Another description is offered in a lecture on 
[GitHub](https://thodrek.github.io/cs564-fall17/lectures/lecture-13/Lecture_13_Btree.pdf). Finally, a visualisation
of B+trees with different properties can be played around with at the 
[University of San Fransisco](https://www.cs.usfca.edu/~galles/visualization/BPlusTree.html).

#### File Header Page
The first 100 bytes of a database file consist of the file header, which contains metadata about the B+tree.
All pages have a size of 4096 bytes, except for the header page which is 100 bytes.
The first tree header page number exists here to enable storage of the database structure within the database file. 
This allows for reconstruction of the database without requiring the noid main database.

| offset | size (bytes)  | description                                              |
|--------|---------------|----------------------------------------------------------|
| 0      | 8             | Header string `"noid v1\0"`                              |
| 8      | 2             | LE `uint16_t` page size in bytes (min 512, default 4096) |
| 10     | 1             | `uint8_t` key size in bytes (multiple of 8, default 16)  |
| 11     | 4             | LE `uint32_t` signature (fnv-1a of all preceding bytes)  |
| 15     | 4             | LE `uint32_t` first tree header page number              |
| 19     | 4             | LE `uint32_t` first freelist page number                 |
| 23     | 77            | Reserved for later use (zeroed)                          |

#### Freelist Page
Freelist pages are pages which list pages previously in use by the database, but are now free for reuse. 
Freelist pages are implemented as a doubly linked list, and are cleaned up when the database is compacted.

| offset | size (bytes)  | description                                  |
|--------|---------------|----------------------------------------------|
| 0      | 2             | page type `'FL'` (free list)                 |
| 2      | 4             | LE `uint32_t` previous free list page number |
| 6      | 4             | LE `uint32_t` next free list page number     |
| 10     | 2             | Padding (zeroed)                             |
| 12     | 1021*4        | LE `uint32_t` list of free page numbers      |

#### Tree Header Page
The tree header page is the entry point for a single B+tree containing a table or an index. Table trees contain
the actual data, whereas index tables contain no data in their leaf nodes. The last 4089 bytes of this page are
sacrificed so that all pages in a database file have a consistent size, allowing for fast sequential reading.

| offset | size (bytes) | description                                                 |
|--------|--------------|-------------------------------------------------------------|
| 0      | 2            | The tree type: `'TT'` (table) or `'TI'` (index)             |
| 2      | 1            | `uint8_t` max entries in internal (page size-24)/entry size |
| 3      | 1            | `uint8_t` max entries in leaf (page size-24)/record size    |
| 3      | 4            | LE `uint32_t` root node page number                         |
| 7      | 4089         | Reserved for later use (zeroed)                             |

#### Internal Node Page
The internal node pages contain no data but only keys and references to child nodes.

| offset | size (bytes)            | description                              |
|--------|-------------------------|------------------------------------------|
| 0      | 2                       | `'IP'` (internal page)                   |
| 2      | 1                       | `uint8_t` key count                      |
| 3      | 4                       | LE `uint32_t` leftmost child page number |
| 7      | 17                      | Padding (zeroed)                         |
| 24     | key count * entry size  | The node entries                         |

#### Internal Node Entry
This is the format of the entries in an internal node.

| offset   | size (bytes)  | description                          |
|----------|---------------|--------------------------------------|
| 0        | Node key size | The key                              |
| key size | 4             | LE uint32_t right child page number  |

#### Leaf Node Page
The leaf node pages contain the records that contain the actual data.

| offset | size (bytes)                | description                             |
|--------|-----------------------------|-----------------------------------------|
| 0      | 1                           | `'LP'` (leaf page)                      |
| 2      | 1                           | `uint8_t` record count                  |
| 3      | 4                           | LE `uint32_t` left sibling page number  |
| 7      | 4                           | LE `uint32_t` right sibling page number |
| 11     | 13                          | Padding (zeroed)                        |
| 24     | record count * record size  | The node records                        |

#### Leaf Node Record
A leaf node record contains at least the first 3 bytes of the data payload. If the total data size is 
at most 8 bytes, the inline indicator is set to the payload size. 
Otherwise, the inline indicator is set to zero and the last 4 bytes of the record contain the first overflow
page number.

| offset   | size (bytes)  | description                                                  |
|----------|---------------|--------------------------------------------------------------|
| 0        | Node key size | The key                                                      |
| key sz   | 1             | `uint8_t` inline indicator                                   |
| key sz+1 | 3             | LE `uint32_t` 1st 3 bytes of payload                         |
| key sz+5 | 4             | LE `uint32_t` 2nd 4 bytes of payload or overflow page number |

#### Overflow Page
| offset | size (bytes)         | description                             |
|--------|----------------------|-----------------------------------------|
| 0      | 2                    | LE `uint16_t` payload size in this page |
| 2      | 4                    | LE `uint32_t` next overflow page number |
| 6      | data size + padding  | The data plus padding (zeroes)          |

### Write-Ahead Logging
Noid uses a Write-Ahead Log (WAL) to support atomic commits and rollbacks. The most important advantages of using
a WAL are:
* In write-heavy applications, a WAL is significantly faster;
* A WAL allows readers and writer to execute in parallel without blocking;
* Disk i/o is often faster since WAL writes tend to be sequential.

The original database file is unmodified and changes are written to the WAL. This allows readers to read from the
original database file, while writers append to the WAL. At a certain point in time, the database must of course
be updated with the WAL contents, but this is a relatively simple operation since the database content is not altered
from a reader/writer point of view. The process of moving WAL pages to the database is called Checkpointing.

#### Checkpointing
Noid supports automatic checkpointing only, though this might become configurable in the future. Currently, a 
checkpoint is done when the WAL contains 1000 pages. This keeps the WAL relatively small and performant.

#### WAL access
Readers and writers do not access the WAL directly, but every `Pager` uses the WAL to provide readers and writers
with a consistent view of the database content. When a reader requests a page from the `Pager`, the `Pager` first
checks the WAL if it contains the requested page. If the page exists in the WAL, and it is followed by a commit page,
that page is returned to the reader. If the WAL contains multiple copies of the requested page, the reader gets
the latest version of that page.

#### WAL Index
The WAL index exists to enable rapid retrieval of WAL log frames. This index is a regular `mmap`-ed file in the
same directory as the database file. The WAL index is transient and is rebuilt from the WAL log if so required. 

#### WAL File Header
A WAL log is created when the first connection to a database is opened. The checkpoint numbers are updated after 
every checkpoint. This enables easy identification of 'old' WAL log frames which can be overwritten.

| offset | size (bytes) | description                             |
|--------|--------------|-----------------------------------------|
| 0      | 12           | Header magic `"noid wal v1\0"`          |
| 12     | 2            | `uint16_t` Database page size in bytes  |
| 14     | 4            | `uint32_t` Checkpoint Sequential Number |
| 18     | 4            | `uint32_t` Checkpoint Random Number     |
| 22     | 4            | `uint32_t` fnv-1a header checksum       |

#### WAL Log Frame
| offset | size (bytes) | description                             |
|--------|--------------|-----------------------------------------|
| 0      | 4            | `uint32_t` Database page number         |
| 4      | 4            | `uint32_t` Checkpoint Sequential Number |
| 8      | 4            | `uint32_t` Checkpoint Random Number     |
| 12     | 4            | `uint32_t` fnv-1a frame checksum        |
| 16     | page size    | The updated database page               |