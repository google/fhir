# Overview

This directory contains scripts to build binary and source distributions of the
`google-fhir` Python package.

## Building and testing the release

To generate `sdist` and `bdist_wheel` release artifacts, it is required that the
host operating system be a supported Linux distribution (Debian >= buster or
equivalent Ubuntu distro). While only _required_ for Darwin hosts, the usage of
Docker is encouraged to obtain the most hermetically-selead environment when
building and testing.

See the [official documentation](https://docs.docker.com/get-docker/) for more
on installing Docker for your host.

### Linux

Run `./distribution/build_distribution.sh` from the `//fhir/py/` directory:

```
un@host:/tmp/fhir/py$ ./distribution/build_distribution.sh
```

### Docker

The Docker image must be built from the **project root** (`//fhir/`), since we
include file outside of the immediate Docker [build context](https://docs.docker.com/engine/reference/commandline/build/#extended-description).

First, build the image:

```
un@host:/tmp/fhir$ docker build -f py/distribution/Dockerfile .
```

Next, create a directory where you want the resulting artifacts to be placed:

```
un@host:/tmp/fhir$ mkdir -p <output-directory>
```

Finally, run the container, and mount your `<output-directory>`:

```
un@host:/tmp/fhir$ docker run -it --rm -v <output-directory>:/tmp/google/fhir/release <image-hash>
```

Where:

* `-it`: Tells Docker that we want to run the container interactvely, by keeping
`STDIN` open even if not attached. It additional allocates a pseudo-TTY
* `--rm`: Instructs Docker to automatically remove the container once it exists
* `-v`: Mounts the host directory: `<output-dir>` at `/tmp/google/fhir/release`,
where the resulting artifacts are generated inside the container

### General

The `build_distribution.sh` will carry out the following steps:

1.  Install necessary system dependencies (at time of writing, `protoc`)
2.  Install necessary Python dependencies within a virtualenv
3.  Build `sdist` and `bdist_wheel` Python artifacts
4.  Instantiate a sandbox "workspace" with appropriate references to necessary
    runtime testdata
5.  Execute all tests against both `sdist` and `bdist_wheel` distributions

If all tests pass, the `sdist` and `bdist_wheel` release artifacts are placed in
`/tmp/google/fhir/release`.

## Installing the release

The output generated from `build_distribution.sh` can be directly installed.

To install the `sdist`, simply:

```
un@host:/tmp/fhir$ pip3 install <output-dir>/*.tar.gz
```

Similarly, to install the `bdist_wheel`:

```
un@host:/tmp/fhir$ pip3 install <output-dir>/*.whl
```

It is recommend that these steps be performed in a Python virtual environment
such as [virtualenv](https://pypi.org/project/virtualenv/) or [venv](https://docs.python.org/3/library/venv.html).

See more about Python packaging from the [official Python packaging
documentation](https://packaging.python.org/tutorials/packaging-projects/).
