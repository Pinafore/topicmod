# Introduction #

Running these commands (assuming you have root) will install the necessary packages to your system.

# NLTK #

If you don't already have NLTK, follow the instructions here on how to install it:

http://www.nltk.org/download

# Scripts #

While not strictly necessary, adding topicmod/bin to your path will add some scripts to your path that will allow you to run examples and to check code for style (by default in the Makefile, so lacking this will cause builds to fail).

# Example #

Doing the following on a clean Ubuntu install will create an environment to compile this code.

```
sudo apt-get install g++ unzip libgsl0-dev libboost-dev python-setuptools subversion
sudo easy_install http://nltk.googlecode.com/files/nltk-2.0b9-py2.6.egg
svn checkout http://topicmod.googlecode.com/svn/trunk/ topicmod-read-only
mv topicmod-read-only topicmod
wget http://protobuf.googlecode.com/files/protobuf-2.4.0a.tar.gz
tar xvfz protobuf-2.4.0a.tar.gz
cd protobuf-2.4.0a/
./configure
make
sudo make install
cd python
python setup.py install
awk '{ printf "%s\n", $0 }END{ printf "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib\n" }'  ~/.bashrc > ~/.bashrc
```

# Debugging #

Are you getting errors related to gsl (e.g. gsl\_vector\_get not found)?  Make sure that:

gsl-config

is accesible and gives reasonable output.