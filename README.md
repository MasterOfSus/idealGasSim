# idealGasSim


**idealGasSim** is a rudimentary ideal‑gas simulator implementing an event‑based three‑dimensional system of freely moving identical particles inside a cubic container. The project includes visualization, data processing, and a fully configurable workflow.

## Features

* Event‑based simulation of a 3D system with an arbitrary number of equal particles in a cubic box
* Visualization via real‑time rendering or MP4 video output, including:

  * Particle renders
  * Graphs of thermodynamic variables and coordinates
* Data export to ROOT files
* Full configuration via `.ini` files
* A library providing:

  * A physics engine supporting the simulation
  * A 3D graphics engine based on SFML
  * Data‑processing utilities generating raw statistical datasets
  * A video‑composition pipeline with support for custom fitting, additional processing, and multithreading

## Supported Environment

The project was developed on **Ubuntu 24.04.2 LTS**.
A floating window manager is recommended; strict tiling managers are not supported.

### Dependencies

Developed and tested with:

* `libsfml-dev` 2.6.1
* `ROOT` 6.36.00

## Compilation

1. Ensure that your ROOT include path is exported through your environment variable (e.g. `CPLUS_INCLUDE_PATH`) if ROOT is not installed via a package manager.
2. Update the linker flags in `CMakeLists.txt` to match your local ROOT installation.
3. Update the target_link_libraries() field in `CMakeLists.txt` to match the multithreading facilities provided by your environment. (the project was developed on an intel machine, amd machines might differ)
3. Configure and build using CMake from the project root:

```
cmake -S . -B build -G"Ninja Multi-Config"
```
then, depending on the desired build type (Debug or Release): 
```
cmake --build build --config ^desired build type^
```
Unit tests are executed by appending --target test, as follows:
```
cmake --build build --config ^desired build type^ --target test
```
The main binaries can be found in build/^desired build type^

## Running the Main Binary

### Editing the Input File

Graph appearance and parameter settings are controlled via an input macro whose path is specified in the configuration `.ini` file. Modify the macro as needed and execute it with ROOT to regenerate the input file. The program will automatically load it on startup according to the path specified in the configuration file.

### Execution

Run the main binary from the project root. Example: the official demo can be executed with

```bash
./build/idealGasSim -c configs/gasSim_demo.ini
```

The demo configuration file serves as a reference for most available runtime parameters.

## Additional "tests" and manual tests execution
The proper unit tests are provided in the same build directory as the main executable, but they rely on the execution environment to provide an `assets` folder equal to that found in the unitTesting directory, it is best to execute them from the unitTesting directory itself.

Additional demos for the getVideo method are provided in the `unitTests/getVideoTest.t` binary.
The binary provides parameters validation and function call safety checks, along two different demos and a stress test for the thread safety.
The `getVideoTest.t` binary depends on the `assets` directory in the same way as the unit tests.

* Demos can be replayed but not re-executed on a single run of the testing binary
* The stress test can be repeated indefinitely

## Acknowledgments of External Projects

* **ROOT Framework**
* **SFML**

## Credits

Thanks to Giulia for providing photos of Jesse, the silly car used as the default buffering wheel.

Finally, some overdue thanks to Diego Quarantani, Niccolò Poli, and Tomaso Tamburini for their essential work on the project, their support throughout its development, their patience in enduring the effects on their life caused by my obsession with it, but most importantly for becoming good friends.
Thank you.

## BLEP!!!!
![hopefully this silly car image is still available](https://static.wikia.nocookie.net/silly-cat/images/5/59/Milly.png/revision/latest?cb=20231001194804)
