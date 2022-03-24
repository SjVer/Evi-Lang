# Evi

A statically typed programming language using the llvm project<br/><br/>
The implementation of this language's parser is based on [clox](https://craftinginterpreters.com/).

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
