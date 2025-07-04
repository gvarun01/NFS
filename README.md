# OSN-Project: Distributed File System

## Overview

This project implements a distributed file system in C. It allows users to store, retrieve, and manage files and directories across multiple servers. The system is designed with a client-server architecture and consists of three main components:

*   **Client:** Provides a user interface for interacting with the file system, allowing users to issue commands for file and directory operations.
*   **Naming Server (NM):** Acts as the central metadata manager. It maintains the file system namespace, tracks the location of files and directories on Storage Servers, and handles file access control (locking) and metadata caching.
*   **Storage Server (SS):** Responsible for the actual storage of file data. Multiple Storage Servers can be part of the system, providing distributed storage capabilities and data redundancy features like backup.

## Features

*   **Core File Operations:**
    *   Read, write (synchronous and asynchronous), and get information (size, permissions) for files.
    *   Create, delete, copy, and paste files.
*   **Core Directory Operations:**
    *   Create, delete (recursively), copy, and paste directories.
    *   List contents of directories or all accessible paths.
*   **Distributed Architecture:**
    *   Client-server model with distinct Naming Server and multiple Storage Servers.
*   **Naming Server (Metadata Management):**
    *   **Trie-based Namespace:** Efficiently manages the hierarchical file system paths.
    *   **LRU Caching:** Caches frequently accessed file path lookups to improve performance.
    *   **Storage Server Registration & Health Monitoring:** Manages active storage servers and their availability.
    *   **File Locking:** Basic support for read/write locks to manage concurrent access.
    *   **Logging:** Records server operations for monitoring and debugging.
*   **Storage Server (Data Management):**
    *   Executes low-level file and directory manipulation commands.
    *   Handles data transfer for read and write operations.
    *   Supports data backup and replication between storage servers.
*   **Network Communication:**
    *   Utilizes TCP/IP sockets for inter-component communication.
    *   Employs a defined request/response protocol.
*   **Audio Streaming:** Includes functionality for streaming audio files.

## Tech Stack

*   **Programming Language:** C
    *   *Why C?* Chosen for its performance, low-level control over system resources (memory, network sockets), and suitability for system programming tasks like building a distributed file system.
*   **Core Libraries/APIs:**
    *   Standard C Libraries (`stdio.h`, `stdlib.h`, `string.h`, etc.)
    *   POSIX APIs for networking (`sys/socket.h`), concurrency (`pthread.h`), and file system operations (`sys/stat.h`, `dirent.h`).
*   **Custom Data Structures:**
    *   **Trie:** For efficient namespace management in the Naming Server.
    *   **LRU Cache:** For optimizing metadata lookups in the Naming Server.
    *   **Linked Lists:** Used for various dynamic data management tasks.
*   **Build System:**
    *   **Make:** `Makefile`s are used to automate the compilation process.

## Setup and Installation

Ensure you have a C compiler (like GCC) and `make` installed on your system.

**1. Compilation:**

Open your terminal and navigate to the respective directories to compile each component:

*   **Client:**
    ```bash
    cd Client
    make
    ```
    This creates an executable named `client` in the `Client` directory.

*   **Naming Server (NM):**
    ```bash
    cd NM
    make -f NMMakefile
    ```
    This creates an executable named `NS` in the `NM` directory.

*   **Storage Server (SS):**
    ```bash
    cd SS
    make
    ```
    This creates an executable named `a.out` in the `SS` directory.

## Running the Project

After compiling, the components should be started in the following order:
1.  Naming Server (NM)
2.  Storage Server(s) (SS)
3.  Client

**1. Start the Naming Server (NM):**

```bash
cd NM
./NS <NM_PORT_FOR_CLIENTS_AND_SS>
```
*   Replace `<NM_PORT_FOR_CLIENTS_AND_SS>` with the port number the Naming Server will listen on for connections from clients and storage servers (e.g., `8080`).

**2. Start Storage Server(s) (SS):**

Each Storage Server requires several arguments to connect to the Naming Server and define its own operational parameters.

```bash
cd SS
./a.out <NS_IP> <NS_PORT> <SS_PORT_FOR_CLIENT_COMM> <SS_PORT_FOR_NM_COMM> <PATHS_FILE>
```
*   `<NS_IP>`: IP address of the machine running the Naming Server (e.g., `127.0.0.1`).
*   `<NS_PORT>`: The port number the Naming Server is listening on (must match `<NM_PORT_FOR_CLIENTS_AND_SS>` from the NM step).
*   `<SS_PORT_FOR_CLIENT_COMM>`: Port number this Storage Server will use to listen for direct client connections (e.g., `9001`).
*   `<SS_PORT_FOR_NM_COMM>`: Port number this Storage Server will use to listen for Naming Server connections (e.g., `9002`).
*   `<PATHS_FILE>`: Path to a text file listing the absolute paths or directories this Storage Server will manage (e.g., `acc_paths.txt` in the `SS` directory, which should be configured with actual paths).

**Example for SS:**
```bash
./a.out 127.0.0.1 8080 9001 9002 acc_paths.txt
```
You can run multiple Storage Server instances, each with its own set of ports and managed paths.

**3. Start the Client:**

```bash
cd Client
./client <NM_IP> <NM_PORT>
```
*   `<NM_IP>`: The IP address of the Naming Server.
*   `<NM_PORT>`: The port number the Naming Server is using for client communication (must match `<NM_PORT_FOR_CLIENTS_AND_SS>` from the NM step).

Once started, the client will provide an interface to interact with the distributed file system.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details. Copyright (c) 2025 Varun Gupta.
