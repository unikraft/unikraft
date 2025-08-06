# `ukfs-devfs`: A filesystem for pseudofiles under `/dev`

This library provides a pseudo-filesystem registered as "devfs" under `ukfs` where libraries can publish special files intended to appear under `/dev`.

Consult the `uk/devfs.h` header for the bespoke API of devfs, and `ukfs` and above layers for the general filesystem API.
