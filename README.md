# A-Minimalistic-Websocket-Server
A work-in-progess software of an absent-minded idiot, written entirely in C, with occasional instances of plagiarizing code and use of open-source libraries.<br><br>
Imported Libraries Are: base64.c, sha1.c, and sha1.h.

## Compiling From Source (Linux):
*(You have no other choice [insert evil face emoji])*

1. Download/Clone this Repository onto your local machine (or piece of metal).
2. Change Directory (cd) into the root of this repository.
3. Run `gcc -o wsserver sha1.c base64.c wsserver.c` &nbsp; (I know, I know, this is not the proper way to do it)
4. Now you should have a dynamicaly-linked Linux executable file named **"wsserver"**.
5. Have fun with it! (`./wsserver`)

## Features:
*Features?! What features??*
<br>
- **1:** Listens on Port 50100 by Default, (this is due to the fact that these ports are usually left to die in the corner)
- However, this can be customized by setting the `--port` flag followed by an unsigned integer from 1 to 65535 (inclusive).
- Example: `./wsserver --port 50101` (It will now listen on port 50101 instead)
- **2:** Logs useless information to Standard Input/Output. e.g. The port that it is listening on, Client Request Packet, and a whole bunch of other trash.
- **3:** Only accepts 4096 bytes of Request Header Data, returns HTTP Error 413 otherwise. *(Is this even a feature??)*
- **4:** Responds politely with "Hi", if Client Request is HTTP "GET" without "Upgrade".
- **5:** Source Code includes ~incomprehensible ramblings of an idiot~ helpful comments.

## Using test/index.html:
...<br>
0. Start the Server. (And perhaps compile it if you haven't.)<br>
1. Open this file in a Web Browser.
2. This HTML page runs JavaScript, so please make sure that JS Execution is enabled.
3. The script will automatically attempt to connect to `ws://127.0.0.1:50100` by default.
4. You may modify this script in any way you wish. (Because it was simply copied it from [this page in Mozilla.org](https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_client_applications).)

<br>

### Just Something to Know:
Like all other softwares under the GNU General Public License, 
This program comes **WITH ABSOLUTELY NO WARRA—** Okay, that's enough, you get the idea. (Because I am a hopeless failure, I can not guarantee that this piece of code will actually work without causing you or the people around you to experience a severe instance of full existantial crisis.)
