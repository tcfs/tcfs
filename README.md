# TCFS

TCFS, a Lightweight Network File System.

## How to use?

- Server(assume IP address is 192.168.0.100):

```
go get github.com/hmgle/tcfs-go/tcfsd
tcfsd -dir .
# can use absolute path:
# tcfsd -dir "/tmp"
# tcfsd -dir "c:" # Windows
```

Alternatively, you could get the [Erlang version](https://github.com/hmgle/tcfs-erl) tcfs server, but that version not support data encrypting.

- Client:

```
sudo apt-get install libfuse-dev
git clone https://github.com/hmgle/tcfs.git
cd tcfs
mkdir mountpoint
make
# 192.168.0.100 is the server IP address
./tcfs --server 192.168.0.100 mountpoint
ls -shal mountpoint
# access mountpoint
# ...
# unmount tcfs
fusermount -u mountpoint
# or sudo umount mountpoint
```

