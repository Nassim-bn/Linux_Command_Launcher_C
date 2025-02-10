# Command Launcher Project

This project involves the implementation of a command launcher. It is divided into three main components:

1. A library implementing a synchronized queue.
2. The program `command_launcher` which uses the synchronized queue to retrieve commands to execute.
3. A `client` program that interacts with the command launcher to place command execution requests into the synchronized queue.

## Synchronized Queue

### Overview

A synchronized queue is an abstract data type that provides (at least) two operations: enqueue and dequeue. In addition to the typical semantics of queues, these operations are blocking when the queue is full (for enqueue) or empty (for dequeue). Moreover, the operations on synchronized queues must be atomic: if multiple tasks are concurrently using the queue, the result should be equivalent to a sequential use of the queue.


### Constraints

- The synchronized queue will use a shared memory segment to allow different processes to insert and retrieve execution requests.
- Each execution request will include:
  - The name of the command to execute along with its optional parameters.
  - Information about named pipes (FIFOs) to be used as the command's standard output and standard error.
  - A second named pipe to be used as the command’s standard input.
- Each request received by the launcher will be processed by a dedicated thread.
- Commands sent by clients will follow the format: `cmd1 | cmd2 | ... | cmdN` (with `N ≥ 1`).
- The result of the command execution, along with any error messages, will be returned to the client via two distinct output streams.

### Launcher Requirements

- The launcher must handle zombie processes and termination requests via signals.
- Proper resource cleanup should be implemented when processes terminate.
- You are free to enhance the application with any improvements you see fit.

## Architecture

The client and daemon architecture is as follows:

- The client sends execution requests to the launcher via a synchronized queue.
- The launcher executes the command in separate threads, managing input/output via pipes.

## Files and Directories

- `command_launcher.c`: The main launcher program.
- `queue.c`: The implementation of the synchronized queue.
- `client.c`: The client program that interacts with the launcher.
- `Makefile`: Build automation for compiling the project.
- `README.md`: This file.

## User Manual

To use the program, follow these steps:

1. Open the terminal in the main project folder and run the command `make` to build the project.

2. - Execute the "command launcher" with the command:
     ```bash
     ./demon XXX
     ```
     (XXX is an integer representing the queue size).
   - If you do not enter a valid parameter, the program will display an error message.
   - If no parameter is entered, once launched, the program will prompt you to choose your preferred queue size.
   - The queue size is limited to 1000 for the protection of the command launcher.
   - To quit the command launcher, use the command `Ctrl+C`. The program will notify you of the exit.

3. - To execute one or more commands, open another terminal and type the command:
     ```bash
     ./client "XXX"
     ```
     (XXX is the list of commands you want to execute, separated by a vertical bar "|").
   - The results will be displayed in the current terminal.
   - Note that multiple requests can be made, either in one or in multiple terminals. This will not cause any issues because the command launcher ensures synchronization of requests and data.

**Note:** A request cannot be made if the command launcher is not running. An error message will notify you in that case.


## Copyright

This project was developed by **Nassim BENCHIKH** and **Rayane Arache**. All rights reserved.