1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

The client knows it has received everything when it detects a special end marker like RDSH_EOF_CHAR (0x04). Techniques that can be used are: 
- Keep reading (recv()) until you see the EOF marker.
- Use a fixed-length header to tell the client how much data to expect.
- Set timeouts so the client doesn’t wait forever if something goes wrong.


2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

TCP is like a long scroll of text with no punctuation—it doesn’t separate messages. The shell must mark where each command starts and ends so they don’t mix. This can be done by adding a null character (\0) or an EOF marker (0x04) at the end of messages. Another method is sending a header with the message length. If boundaries aren’t handled, commands could merge together, get cut off, or make the client wait forever for more data.

3. Describe the general differences between stateful and stateless protocols.

A stateful protocol remembers things from past messages. It’s like a phone call where both sides stay connected and keep track of the conversation. Example: SSH (your session stays open). A stateless protocol treats every request separately, like sending emails where each message stands alone. Example: HTTP (loading a webpage doesn’t remember your last visit).

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

UDP is faster than TCP because it doesn’t check if data arrives. It’s good when speed matters more than accuracy. It’s used for live streaming, online gaming, and video calls, where missing a small piece doesn’t ruin everything. It’s also great for DNS lookups, where you just need a quick answer. But for things like file downloads, where every piece must be correct, TCP is better.

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

The operating system provides sockets, which are like digital mailboxes for sending and receiving data. A program creates a socket, connects to another computer, sends data, and waits for a response, just like texting. There are TCP sockets (reliable like a phone call) and UDP sockets (fast like shouting across a room).
