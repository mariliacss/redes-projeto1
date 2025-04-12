# Movie Streaming Service

This project implements a movie streaming service using Server-Client architeture and TCP for network comunication.

## How to Run the Code

First you have to have installed or install the following packages:

* CMake
* SQLite3
* SQLite3 development package

Then, after cloning the repository, run:

```sh
 mkdir build
 cd build
 cmake ..
 make
```
This will create the executables in the build folder. To compile again, simply run `make`. To run `server` and `client` applications:

```sh
./server
./cliente 127.0.0.1
```

Replace `127.0.0.1` for the IP address of the machine you are running the server.