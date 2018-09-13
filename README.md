# mozquic_example
This is going to be a simple client-server implementation using the QUIC-protocol as transport protocol. 

This example uses the mozquic-library as base. The used version can be found [here](https://github.com/jakobod/mozquic) in the 'trunk' branch.
It is a fork of https://github.com/mcmanus/mozquic, but slightly changed. 

Also you will need nss, which can be found [here](https://github.com/nss-dev/nss). You will need version 3.33 or higher.

# build
```sh
git clone git@github.org:jakobod/mozquic.git
cmake . 
make
cd ..
git clone git@github.com:jakobod/mozquic_example.git
mkdir build && cd build
cmake ..
make
```

# running
```sh
./server # starts the server on port 4434
./client # starts client_1
./client # starts client_2
```
The client will ask you for a host to connect to. Use 'localhost' for testing purposes.
After that it wants a port to connect to. Use '4434' to connect to the server.

# NOTE
This project needs to be compiled with clang/clang++. Otherwise you will probably get runtime errors.
