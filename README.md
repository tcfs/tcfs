# TCFS

TCFS, a Toy Cloud File System.

## How to use?

- Server:

```
git clone https://github.com/hmgle/tcfs-erl.git
cd tcfs-erl
make
make run
```

- Client:

```
git clone https://github.com/hmgle/tcfs.git
cd tcfs
mkdir mountpoint
make
./tcfs mountpoint
ls -shal mountpoint
```
