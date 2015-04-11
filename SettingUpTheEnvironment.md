# Introduction #

The various files here rely on some scripts to work.  This will:
**Download the source code** Rename it so it's consistent with what's expected
**Add executables to your path** Make them executable
**Unzip data**

# Details #

To set up
```
svn checkout http://topicmod.googlecode.com/svn/trunk/ topicmod-read-only
mv topicmod-read-only topicmod
awk '{ printf "%s\n", $0 }END{ printf "export PATH=$PATH:$HOME/topicmod/bin\n" }'  ~/.bashrc > ~/.bashrc
chmod +x ~/topicmod/bin
cd ~/topicmod/data
chmod +x *.sh
./unzip_data.sh
```

# Virtual Machine #

If you're having problems, this virtual machine is all ready to go:
http://dl.dropbox.com/u/27635323/Topicmod.ova
http://dl.dropbox.com/u/27635323/Topicmod.txt

(Be sure to update the repo.)