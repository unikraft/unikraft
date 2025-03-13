# ukpod: Pages on Demand

Pages on Demand (PoD) is a library that allows callers to obtain contiguous read-write virtual memory that:
 * may or may not be mapped by physical RAM at any time
 * behaves consistently with it always being mapped
 * has this consistency ensured by caller-provided callbacks

If this sounds familiar and similar to demand-paging, file mmaps, or page caching, is because PoD is an abstraction over, and generalization of these concepts.
PoD is, in essence, Unikraft-internal file-like mmap, where the caller implements the file semantics, and PoD handles the rest.

Furthermore, PoD allows us to decouple policy -- the requirement for callers to obtain memory with the desired semantics -- from mechanism -- the precise fashion that memory is acquired and managed -- by:
* providing a unified abstract API to callers
* allowing for selection of PoD implementation backends tailored for the use case

PoD backends can either be provided by ukpod itself, or implemented in external libraries.

For more information on using PoD consult the source documentation in `include/uk/pod.h`.
