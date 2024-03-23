# Directory Synchronizer
A small app for Windows, that replicates the files between 2 computers in the local network.

The backend of the app is written in C++. The frontend is an [Electron JS](https://www.electronjs.org/) app.

Here is how the app works on a very abstract level. Let's say, that 2 instances of the it run on 2 computers in the same LAN.
I'll call one of the computers the Server and the other - the Client.
The Server with be the one, that will replicate the files of some of its folders to the Client.
So here is what happens:

Both the Server and the Client select a path to a directory.
The server reads all the files in its directory and hashes their data to 64-bit unsigned integers.
The algorithm, that is used for this operation is called [xxHash](https://xxhash.com/).
More specifically, the Stephan Brumme's C++ implementation from [here](https://create.stephan-brumme.com/xxhash/).

After the data hashing process is completed, the Client establishes a TCP connection to the server and requests a list of its files together with their hashes.
Then, the Client on its own turn checks which of the files from the list exist on its side.
Calculates their hashes and compares them with the server ones.
Then, prepares a list of missing or changed files, that need to be downloaded from the Server.
The file downloading process is done over the UDP protocol by the C++ backend of the app.
For better efficiency, the app can stream several files in parallel over the network.

The frontend displays in real time the progress of the file transfers.
