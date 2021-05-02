# Assignment 6: Socket Programming

## Instructions to run the code

1. Firstly, open two different terminals (to execute both client and server).
2. After extracting the zip file. Enter the directory `2019111009` in both terminal windows, with the help of command:
`cd 2019111009`
3. Firstly, In the first terminal window, enter the server directory, namely `Server`, with the help of command:
`cd Server`
After entering server directory, run the following command to compile and run the server.c program:
`gcc -o server server.c; ./server`
4. In the second terminal window, enter the client directory, namely `Client`, with the help of command:
`cd Client`
After entering client directory, run the following command to compile and run the client.c program:
`gcc -o client client.c; ./client`
5. In the second window (client side), run queries of clients on the Command Line Interface. Valid commands are:
`get <fileName>` (downloads file with name fileName on client side, by obtaining from server),
`​get <file1> <file2> <file3> ..... <fileN>` (downloads multiple files on client side with their corresponding names, by obtaining from server),
`​exit` (closes connection with server).
6. Different files can be created in directories `Client` and `Server` for testing purposes.

## Assumptions 

1. The file name of file (asked to download) won't contain more than 1024 characters.
2. The number of files asked to be downloaded in a single command (second valid command on CLI, explained above), won't exceed 127.
3. Atmost one client queries the server at an instant.
4. On the client side, if there already exists a file with the same name that is currently requested to the server to download, the pre-existing file on client side will be overwritten while downloading.
5. (Trivial) client and server executables are never in the same directory.
6. The server is started (run) first, followed by the client.
7. File size won't exceed 2147483647 bytes (`INT_MAX`).

## Important points to note

1. SIGINT and SIGPIPE signals are handled for both client and server. The client exits / closes connection with server only when `exit` command is entered, and is unaffected when 'CTRL-C' is entered. The server shuts down when it receives the SIGINT signal, and if any client is currently connected with it, will also exit when the next command is entered on client-side CLI.
2. Download progress is printed on the client side. Server side prints prompts only when it is up and running, and when it exits successfully with SIGINT signal, and appropriate error messages.
3. Errors are handled appropriately, and the code is properly commented.
