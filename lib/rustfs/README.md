# Rust on Unikraft
A simple ram FS for the unikraft kernel

## Usage
Clone this into your Unikraft `~/.unikraft/unikraft/lib` folder and register the library in `~/.unikraft/unikraft/lib/Makefile.uk`
and in the vfscore `Config.uk`.
At last create a test app and choose Rustfs as the root file system in the vfscore meunconfig.

Currently it is required to use the nightly toolchain.
simply run `make` to execute (after configuration)
