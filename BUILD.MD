# TINET build instructions

1. Install [TI Connect CE](https://education.ti.com/en/products/computer-software/ti-connect-ce-sw) (Or [TiLP](http://lpg.ticalc.org/prj_tilp/) if you are on Linux)
2. Install the [CE Toolchain](https://ce-programming.github.io/toolchain/static/getting-started.html) and make sure it's added to your PATH environment variable
3. Build the project:

    ```shell
    git clone https://github.com/tkbstudios/tinet-calc.git
    cd tinet-calc
    make gfx
    make
    ```

4. Install the generated TINET.8xp file
5. Install the [clibs.8xg](https://github.com/CE-Programming/libraries/releases/latest) libraries if you haven't already
