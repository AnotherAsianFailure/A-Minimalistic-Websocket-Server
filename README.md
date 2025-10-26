# A-Minimalistic-Websocket-Server
A work-in-progress software of an absent-minded idiot, written entirely in C, with occasional instances of plagiarizing code and use of open-source libraries.

## Compiling From Source (Linux):
*(You have no other choice [insert evil face emoji])*

1. Download/Clone this Repository onto your local machine (or piece of metal).
2. Change Directory (cd) into the root of this repository.
3. Run `gcc -o wsserver sha1.c base64.c wsserver.c` &nbsp; (I know, I know, this is not the proper way to do it)
4. Now you should have a dynamicaly-linked Linux executable file named **"wsserver"**.
5. Have fun running it! (`./wsserver`)

## Features
*Features?! What features??*
<br>
- **1:** Listens on Port 50100 by Default,
- However, this can be customized by setting the `--port` flag followed by an unsigned integer from 1 to 65535 (inclusive).
- Example: `./wsserver --port 50101` (It will now listen on port 50101 instead)
- **2:** Only accepts 4096 bytes of Request Header Data, returns HTTP Error 413 otherwise. *(Is this even a feature??)*
- **3:** Responds politely with "Hi" if Client Request is HTTP "GET" without "Upgrade".
- **4:** Source Code includes ~incomprehensible ramblings of an idiot~helpful comments.
