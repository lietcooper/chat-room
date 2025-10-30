<!--
  Auto-generated / edited by AI assistant to help contributors and AI agents.
  Keep this short and specific to the codebase. If you update project structure
  or protocols, please update this file.
-->

# Copilot / AI agent instructions for chatroom-C

Summary
- Small Windows-based TCP chat server implemented in a single `main.c` file.
- Uses Winsock2, blocking sockets multiplexed with `select()` and a fixed-size client table.
- Project is Windows-specific (includes <winsock2.h>, <windows.h>, and links `ws2_32.lib`).

Big picture (what to know quickly)
- Single-process, synchronous server listening on PORT (defined as `8080` in `main.c`).
- On new connection the server expects the client to send a username as the first message.
- After that, any received text is broadcast to other connected clients. The server treats `quit` or `exit` as client disconnect commands.
- Client state is held in simple arrays: `SOCKET client_sockets[MAX_CLIENTS]` and `char client_names[MAX_CLIENTS][50]`.

Key files
- `main.c` — entire server implementation. Look here for protocol and constants: `PORT`, `BUFFER_SIZE`, `MAX_CLIENTS`.

Build & run notes (discoverable from code)
- This is a Windows Winsock program and must be built/linked with ws2_32. Example commands (common, adjust to your environment):
  - MSVC (Developer Command Prompt):
    cl /EHsc main.c ws2_32.lib
  - MinGW / GCC (mingw-w64 on Windows):
    gcc -o chatroom.exe main.c -lws2_32

Runtime & protocol specifics to preserve
- Upon `accept()` the server immediately calls `recv()` to read the username. Agents editing the server must preserve this handshake.
- The server uses a simple text protocol; messages are newline-terminated strings. It strips a final `\n` before processing.
- Special commands: `quit` and `exit` (exact string comparison) trigger a disconnect and a broadcasted leave message.

Important implementation patterns and conventions
- Encoding: `SetupConsole()` configures console codepage and locale for Chinese (GBK) using `SetConsoleOutputCP(936)` and `setlocale(LC_ALL, "chs")`. Be careful touching console/encoding code — it's intentionally Windows/Chinese-oriented.
- Concurrency: no threads — single loop with `select()` and a 1-second timeout. Avoid adding blocking operations inside the loop.
- Limits are hard-coded: `MAX_CLIENTS` and `BUFFER_SIZE`. If you change them, update all related logic (array sizes, loops).
- Error handling is immediate, prints to console with `WSAGetLastError()` and often exits on fatal failure.

Examples (references from `main.c` to copy/paste or inspect):
- Client table and name arrays: `SOCKET client_sockets[MAX_CLIENTS]` and `char client_names[MAX_CLIENTS][50]`.
- Username receive on connect: server does `recv(client_socket, client_names[client_index], sizeof(...)-1, 0)` then null-terminates.
- Broadcast loop: iterate `for (int j = 0; j < MAX_CLIENTS; j++)` and `send()` to other valid sockets.

Editing guidance for AI agents
- Preserve Windows APIs and link flags — this is not a POSIX program. If asked to port to POSIX or macOS, explicitly note the many required changes (sockets APIs, console handling, Sleep vs usleep, includes). Do not silently convert.
- When refactoring, keep these contracts intact:
  - First message after accept() is username.
  - Broadcast format: "%s: %s\n" where first placeholder is `client_names[i]`.
  - Commands `quit`/`exit` must remain recognized exactly.
- Prefer minimal, isolated edits. The codebase is one-file and small; large refactors should be proposed to the maintainer first.

Tests & debugging tips (practical)
- Quick manual test: run the server on Windows and connect with two simple TCP clients (telnet or netcat) to exercise username handshake and message broadcast.
- For debugging, use Visual Studio or gdb for MinGW builds. Watch the `client_sockets` array and `client_count` when adding/removing clients.

If anything in this file looks incorrect or the project has additional components (clients, docs, CI), tell me where they live and I will update these instructions.

— end of file —
