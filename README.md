# P2P File Sharing System (COE768)

A peer-to-peer (P2P) file distribution and index system implemented in C with POSIX sockets.

## Architecture

- **`server/`**: The centralized indexing server (UDP).
  - `pdu.h`: PDU structure and protocol constants.
  - `registry.h`: In-memory registry data structures and request handlers for registration (`R`), content search (`S`), online listing (`O`), and de-registration (`T`).
  - `main.c`: Clean index server socket initialization and main dispatch loop.
- **`client/`**: The peer client application (UDP for indexing, TCP for peer transfer).
  - `pdu.h`: Protocol Data Unit definition and buffer limits.
  - `registration.h`: Registration, de-registration, content listing, and local file tracking.
  - `getContent.h`: Search content, peer-to-peer file downloading, and local peer download server handler.
  - `main.c`: Main interactive CLI menu and event loop multiplexing peer connections and stdin via `select()`.

## Compilation

Compile using `gcc`:

```bash
# Compile server
gcc -o server/server server/main.c

# Compile client
gcc -o client/client client/main.c
```

## Running

1. **Start the Index Server**:
   ```bash
   ./server/server [optional_port] # defaults to 3000
   ```

2. **Start a Peer Client**:
   ```bash
   ./client/client
   ```
   Follow the interactive prompts to enter your peer name, index server IP, and port.
