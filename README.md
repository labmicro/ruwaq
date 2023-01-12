# RUWAQ

-----

RUWAQ is a multi-board firmware to interface with hardware for run automated tests.

**Table of Contents**

- [How to use](#How-to-use)
- [License](#license)

## How to use

RUWAQ is a multi-board firmware to allow [SIRU](https://github.com/labmicro/siru) tool to run automated tests on hardware, designed to be used in the development of embedded systems.

To compile it is required to have cloned this repository and also the base structure provided by the framework [MUJU](https://github.com/labmicro/muju). Eventually, the `MUJU` path must be corrected in the `makefile` of this project and then it can be compiled and flash to the board with the command

```console
make download
```

**At this moment this project is in development, and it only has support for the EDU-CIAA-NXP boards.**

## License

`RUWAQ` is distributed under the terms of the [MIT](https://spdx.org/licenses/MIT.html) license.
