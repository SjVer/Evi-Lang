# Evi

A statically typed programming language using the llvm project. \
 \
Personally I've lost interest in this project. It works as a minimum viable product and I'm genuinly proud of what it has become and what I've learned from making it, but it's time to move to the next project. \
 \
Documentation is unfinished but available at this github page's wiki.


### Building
In order to build the evi compiler and its standard library run the following commands in the root folder of this repository (after you've cloned it):

```sh
make deb target=amd64 # generate debian package
```

and

```sh
sudo apt install ./bin/evi*.deb # install the debian package
```

PS: llvm is required

(This README is a w.i.p. obviously)
