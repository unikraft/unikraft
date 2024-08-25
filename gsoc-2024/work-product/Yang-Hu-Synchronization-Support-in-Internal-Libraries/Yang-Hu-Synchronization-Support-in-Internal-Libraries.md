# Unikraft GSoC'24: Synchronization Support In Internal Libraries

<img src="https://unikraft.org/images/unikraft-gsoc24.png"/>

## Summary

Memory allocators have a significant impact on application performance.
There have been serveral [research papers](https://dl.acm.org/doi/10.1145/378795.378821) which compared different memory allocators.
According to their work, switching to the appropiate memory allocator may improve the application performance by 60%.
Unikraft currently supports only one non-synchronized memory allocator, the [*binary buddy* allocator](https://github.com/unikraft/unikraft/tree/staging/lib/ukallocbbuddy).
Therefore, this project aims to enhance synchronization support in Unikraft's internal libraries and port [*mimalloc*](https://github.com/microsoft/mimalloc) (pronounced "me-malloc"), a high-performance general-purpose memory allocator developed by Microsoft, to it.

## GSoC Contributor

Name: Yang Hu

Email: yanghuu531@gmail.com

Github profile: [huyang531](https://github.com/huyang531)

## Mentors

* [Răzvan Vîrtan](https://github.com/razvanvirtan)
* [Radu Nichita](https://github.com/RaduNichita)
* [Sergiu Moga](https://github.com/mogasergiu)
* [Răzvan Deaconescu](https://github.com/razvand)

## Contributions

### Porting *mimalloc*

I started my work based on [Hugo Lefeuvre's](https://github.com/hlef) initial attempt to port mimalloc to Unikraft back in 2020 (see [this repo](https://github.com/unikraft/lib-mimalloc)).
As Unikraft has evolved significantly over the years, I've had to adapt the port to work with the latest Unikraft core (v0.17.0), which includes:

- Switch *mimalloc* from using `newlib` to `musl`:
    - [PR: Add the `stdatomic.h` header to `lib-musl`](https://github.com/unikraft/lib-musl/pull/80)
    - [PR: Adapt `lib-mimalloc` to using `lib-musl` and the new threading interface](https://github.com/unikraft/lib-mimalloc/pull/12)

- [PR: Adapt Unikraft's heap initialization routine to support *mimalloc*](https://github.com/unikraft/unikraft/pull/1480)

### Benchmarking *mimalloc*

I benchmarked the *mimalloc* allocator using the `cache-scratch` and `cache-thrash` benchmarks *without SMP support enabled*.
The details of how the benchmarks work can be found in [my second blog post](https://unikraft.org/blog/2024-07-12-unikraft-gsoc-test-mimalloc-on-unikraft).

In debugging the benchmarks, I have also ran into a confusing bug where the TLS's magic value would be corrupted when the user declares a thread-local array larger than 15 bytes.

The product of this part of my work includes:

- [Gist: The benchmark application ported to Unikraft](https://gist.github.com/huyang531/28a31fc2d9f348fafb5b9d8e6c9493d5)
- [Issue: Cannot handle thread-local arrays larger than 15 bytes](https://github.com/unikraft/unikraft/issues/1478)

### Other Works

During the course of this project, I have also fixed a few minor bugs and amended Unikraft's documentation:

- [PR: lib/ukvmem: Fix dependency and test scripts](https://github.com/unikraft/unikraft/pull/1447)
- [PR: Fix typos and add notes about the hardware acceleration with KVM for random number generation](https://github.com/unikraft/docs/pull/452)
- [PR: Add tips for disabling compiler optimization in debug guide](https://github.com/unikraft/docs/pull/442)

## Blog Posts

 - [Blog post #1: Porting *mimalloc*](https://unikraft.org/blog/2024-06-16-unikraft-gsoc-port-mimalloc-to-unikraft)
 - [Blog post #2: Testing *mimalloc*](https://unikraft.org/blog/2024-07-12-unikraft-gsoc-test-mimalloc-on-unikraft)
 - [Blog post #3: Debugging *mimalloc*](https://github.com/unikraft/docs/pull/448)
 - [Blog post #4: Benchmarking *mimalloc* in an SMP environment](https://github.com/unikraft/docs/pull/455)
 
## Future Work

### Running *mimalloc* in an SMP Environment

After running the benchmarks under multiple configurations, we noticed that *mimalloc* did not perform significantly better than the existing *binary buddy* allocator.
This is because we are not running the virtual machine with SMP enabled, so *mimalloc* would have wasted much time on unnecessary synchronization.
Therefore, to really benchmark the performance of *mimalloc* (and to justify Unikraft's potential for full SMP support), we need to get it running with real parallelism.

I have outlined some key considerations and future steps in [this blog post](https://github.com/unikraft/docs/pull/455).

### Further Testing of *mimalloc*

So far, we have only tested *mimalloc* on two multi-threaded benchmarks.
To further validate its functionality, portability, and performance, we need to test *mimalloc* on [some more complicated benchmarks](https://unikraft.org/blog/2024-07-12-unikraft-gsoc-test-mimalloc-on-unikraft#next-steps) and on real-world applications like Redis.

### Porting the Latest *mimalloc* to Unikraft

So far, we have been working on *mimalloc* v1.6.3.
We can further improve our work by porting a newer version of the memory allocator to Unikraft.

### Towards Full Synchronization Support in Internal Libraries

As the project title suggests, more work needs to be done for Unikraft to provide general synchronization support for user applications, which goes far beyond just providing a synchronous memory allocator.
For example, we may need to synchronize the [Virtual Memory interface (`ukvmem`)](https://github.com/unikraft/unikraft/tree/staging/lib/ukvmem), the [cooperative scheduler (`ukschedcoop`)](https://github.com/unikraft/unikraft/tree/staging/lib/ukschedcoop), and rethink how every component interacts with each other in an SMP environment.

## Acknowledgements

This has been an incredibly fun (and challenging) journey and I am grateful to every one in the Unikraft community who made this GSoC project possible.
Special thanks to Răzvan Vîrtan, Radu Nichita, Sergiu Moga, and Răzvan Deaconescu, for all your support along the way!
