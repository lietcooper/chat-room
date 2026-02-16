# Simple C Chatroom

A multi-user chatroom system implemented in C using TCP sockets. This project provides separate implementations for Windows and macOS/Linux to handle platform-specific networking and threading APIs.

## Project Structure

- `macOS-linux/`: Contains source code for Unix-like systems.
- `windows/`: Contains source code for Windows systems.
- Each directory includes:
  - `chatServer.c`: The server that manages connections and broadcasts messages.
  - `client.c`: The client application with a multi-threaded UI for sending and receiving messages simultaneously.

---

## macOS / Linux

### Prerequisites
- **Compiler**: `gcc` or `clang`
- **Library**: `pthread` (usually pre-installed)

### Compilation
Open your terminal and run the following commands in the `macOS-linux` directory:

1. **Compile Server**:
   ```bash
   gcc chatServer.c -o server
   ```

2. **Compile Client**:
   ```bash
   gcc client.c -o client -lpthread
   ```

### Running
1. Start the server first:
   ```bash
   ./server
   ```
2. In separate terminal windows, start one or more clients:
   ```bash
   ./client
   ```

---

## Windows

### Prerequisites
- **Compiler**: MinGW-w64 (GCC) or Microsoft Visual C++ (MSVC).
- **Library**: `ws2_32` (Windows Socket library).

### Compilation (using MinGW/GCC)
Open your Command Prompt or PowerShell in the `windows` directory:

1. **Compile Server**:
   ```bash
   gcc chatServer.c -o server.exe -lws2_32
   ```

2. **Compile Client**:
   ```bash
   gcc client.c -o client.exe -lws2_32
   ```

### Running
1. Start the server:
   ```bash
   server.exe
   ```
2. Start one or more clients:
   ```bash
   client.exe
   ```

---

## Key Differences

| Feature | macOS / Linux | Windows |
| :--- | :--- | :--- |
| **Networking API** | POSIX Sockets (`sys/socket.h`) | Winsock (`winsock2.h`) |
| **Threading** | `pthread` (`pthread.h`) | Windows Threads (`process.h` / `windows.h`) |
| **Initialization** | Not required | Requires `WSAStartup` & `WSACleanup` |
| **Linking** | `-lpthread` | `-lws2_32` |
| **File Handles** | Uses standard `int` as descriptors | Uses `SOCKET` type |

## Usage
1. Once the client starts, enter your preferred **username**.
2. Type your message and press **Enter** to broadcast it to everyone in the chatroom.
3. Type `quit` to exit the application.
