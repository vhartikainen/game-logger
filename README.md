GameLogger
==========

GameLogger is a system to monitor game play activity on players' computers and publishes that information on a website. The system comprises of three conceptual component:
- **Client**: Records the open game and counts the number of mouse button and key presses per seconds (APM), and sends this information to the server.
- **Server**: The server listens to clients' updates and stores the active game and APM information to a database. It also servers browsers this data.
- **Browser**: View to the players' game and APM data. Actively polls the server for updated data and dynamically updates the web page to show the most latest gme and APM information.

The system is synchronized with the server clock, so that all clients and browsers push and pull data virtually synchronously. With a 5 second APM buffer, and one second upload times, the total latency of the APM information from the players' computer to the browser component is around 7 seconds. With a smaller buffer, this can be shortened.


System
------

# Server

The server acts as a controller to
* Provide settings to client and browsers
* Listen to clients for data upload
* Cache and store the data to a MySQL database
* Serve the cached and stored data to browsers

## Setup

The server is built with PHP and MySQL. To setup, follow these steps:
1. Use the createTables.sql to create the required MySQL tables.
2. Edit the database.php with appropriate database access.
3. Store the server files to under a URL accessible from the web.
4. Customize your own web look. 

[**Documentation**]() for customizing the web pages.

# Client

You can either download the most recent binaries

[**Download GameLogger client v1.3**](http://www.motify.fi/gamelogger/GameLogger_v1.3.zip)

or compile them yourself.

## Compiling

**Requirements**

* [Qt 6.x](https://www.qt.io/download) — install the `msvc2022_64` component
* [Visual Studio 2022](https://visualstudio.microsoft.com/) (Community edition works) — install the *Desktop development with C++* workload
* Windows SDK (installed automatically with the Visual Studio workload above)

**Using the build script**

Edit the two paths at the top of [client/build.cmd](client/build.cmd) to match your Qt and Visual Studio installations:

```
set QT_DIR=C:\Qt\6.8.0\msvc2022_64
set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
```

Then run from a plain Command Prompt (no need to open a Developer prompt first):

```
client\build.cmd
```

The executable is written to `client\GameLogger\build-tmp\release\GameLogger.exe`.

If [jom](https://wiki.qt.io/Jom) is on your `PATH` it will be used instead of `nmake` for a parallel build.

**Manual build**

```bat
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set PATH=C:\Qt\6.8.0\msvc2022_64\bin;%PATH%
mkdir client\GameLogger\build-tmp
cd client\GameLogger\build-tmp
qmake ..\GameLogger.pro -spec win32-msvc "CONFIG+=release"
nmake release
```

## Running

The client takes two required arguments:

1. **Player name** — case-sensitive; used to identify the player on the website and in the HTML.
2. **Server URL** — the base URL of the game logger server where `clientConnect.php` is located.

```bat
GameLogger.exe <PlayerName> <ServerURL>
```

**Examples:**

```bat
GameLogger.exe Alice http://example.com/gamelogger
```

```bat
GameLogger.exe "Player One" http://192.168.1.10/gamelogger
```

If the player name contains spaces, wrap it in quotes. Starting the client without arguments will show an error dialog.

