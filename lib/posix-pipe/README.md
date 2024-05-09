# posix-pipe

`posix-pipe` is an internal library of Unikraft that enables the system calls that allows interacting with pipes.

## Configuring applications to use `posix-pipe`

You can select `posix-pipe` under the `Library Configuration` screen of the `make menuconfig` command.
After that, you can use the functions exposed by the internal library just like you would for a Linux system,

An example of a simple application that uses the `posix-pipe` library is the following:

```c
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main() {
    int pipe_fd[2]; // File descriptors for the pipe

    // Create a pipe
    if (pipe(pipe_fd) == -1) {
        return -1;
    }

    // Write data to the pipe
    const char* message = "Hello, Pipe!";
    write(pipe_fd[1], message, strlen(message));

    // Read data from the pipe
    char buffer[100] = {};
    read(pipe_fd[0], buffer, sizeof(buffer));

    // Close the pipe
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    // Print the received data
    printf("Received from pipe: %s\n", buffer);

    return 0;
}
```