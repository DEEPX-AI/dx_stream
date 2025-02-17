# DX-Stream

DX-Stream is a GStreamer-based Vision AI application development tool that allows you to easily and quickly implement Vision AI applications using the DEEPX NPU.

## Documentation

[DX-Stream User Guide](./docs/source/overview.md)

## For DEEPX Internal 

#### Build Docs

```
$ cd docs
$ mkdocs build
```
- `docs/DX-Stream vX.X.X.pdf` 

#### Docker Release

```
$ ./dk build <path to dx_rt archive tar.gz>
$ docker save -o dxstream_vX.X.X_amd64_ubuntu20.04.tar dxstream
```
- Release `dxstream_vX.X.X_amd64_ubuntu20.04.tar` & `dk` files

#### Source Release 

```
$ git archive -o dxstream_vX.X.X.zip HEAD;

```
- Release `dxstream_vX.X.X.zip` 

#### Debian Release (Deprecated)
