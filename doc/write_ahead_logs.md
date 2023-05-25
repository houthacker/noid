# Write-Ahead Logging
Noid uses a Write-Ahead Log (WAL) to support atomic commits and rollbacks. The most important advantages of using
a WAL are:
* In write-heavy applications, a WAL is significantly faster;
* A WAL allows readers and writer to execute in parallel without blocking;
* Disk i/o is often faster since WAL writes tend to be sequential.

The original database file is unmodified and changes are written to the WAL. This allows readers to read from the
original database file, while writers append to the WAL. At a certain point in time, the database must of course
be updated with the WAL contents, but this is a relatively simple operation since the database content is not altered
from a reader/writer point of view. The process of moving WAL pages to the database is called Checkpointing.

## Checkpointing
Noid supports automatic checkpointing only, though this might become configurable in the future. Currently, a
checkpoint is done when the WAL contains 1000 pages. This keeps the WAL relatively small and performant.

## WAL access
Readers and writers do not access the WAL directly, but every `Pager` uses the WAL to provide readers and writers
with a consistent view of the database content. When a reader requests a page from the `Pager`, the `Pager` first
checks the WAL if it contains the requested page. If the page exists in the WAL, and it is followed by a commit page,
that page is returned to the reader. If the WAL contains multiple copies of the requested page, the reader gets
the latest version of that page.

### WAL File Locking
The available locking strategies are documented in `file_locking.md`.

## WAL Index
The WAL index exists to enable rapid retrieval of WAL log frames. This index is a regular `mmap`-ed file in the
same directory as the database file. The WAL index is transient and is rebuilt from the WAL log if so required.

## WAL File Header
A WAL log is created when the first connection to a database is opened. The checkpoint numbers are updated after
every checkpoint. This enables easy identification of 'old' WAL log frames which can be overwritten.

| offset | size (bytes) | description                             |
|--------|--------------|-----------------------------------------|
| 0      | 12           | Header magic `"noid wal v1\0"`          |
| 12     | 2            | `uint16_t` Database page size in bytes  |
| 14     | 4            | `uint32_t` Checkpoint Sequential Number |
| 18     | 4            | `uint32_t` Checkpoint Random Number     |
| 22     | 4            | `uint32_t` fnv-1a header checksum       |

## WAL Log Frame
| offset | size (bytes) | description                             |
|--------|--------------|-----------------------------------------|
| 0      | 4            | `uint32_t` Database page number         |
| 4      | 4            | `uint32_t` Checkpoint Sequential Number |
| 8      | 4            | `uint32_t` Checkpoint Random Number     |
| 12     | 4            | `uint32_t` fnv-1a frame checksum        |
| 16     | page size    | The updated database page               |