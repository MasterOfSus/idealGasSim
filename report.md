# idealGasSim
## Authors
Lorenzo Liam Baldisserri
The project was also built on the work of the previous group members:
Diego Quarantani
Niccolo Poli
Tomaso Tamburini

## Introduction
This project is a rudimentary event-based gas simulator operating in an euclidean three-dimensional space, with graphic data visualization and ROOT file output functionalities. It has the intent of allowing for clear visualization of the most basic phenomena described in kinetic theory of ideal gases, and the qualitative comparison of the theoretical results provided by said theory with their simulated counterparts.
It strives to achieve this by running a physical simulation of a physical system, representing a gas, and by subsequent processing of the raw simulation data into meaningful statistical datasets and thermodynamical variables, mainly stored through the formats provided by the ROOT framework, so that these can both be visualized and saved for later fruition.
The visualization is achieved through the output of a video feed (live or saved to mp4) which enables the user to view the system's progression through time, both through its statistical and thermodynamical characteristics and through a direct video rendering of the system itself.
The provided experience is customizable to a limited degree, in almost all of its simulation parameters and aesthetical characteristics.
Finally, the developed facilities are also made accessible through the project's library, which can be used to implement more extensive and precise data processing, if desired.

## Implementation choices
The concept of a gas has been abstracted to a set of identical rigid spherical particles, freely moving inside of a cubical container, subject only to reciprocal collisions and collision with the container's inner walls.
This system is then observed in its evolution through time, through the elaboration of the simulated events' data into statistical values and datasets, which are then trivially converted to the thermodynamical quantities of interest.
Finally, the resulting statistical and thermodynamical data is converted to both a graphical format (a video, which can be viewed in real time while the simulation runs, or stored in an mp4 file for repeated viewing) and a statistical format, through the ROOT framework's file output functionality, saved in a ROOT file.
To make the visualization of the phenomena more intuitive, a direct gas rendering and display functionality has been implemented and integrated with the graphical statistics visualization, providing a unified video output.

The project has also been structured so as to allow for free and customizable usage of the implemented facilities, as these allow for a wider range of data processing than that which has been implemented in the main executable.

The project can be seen as divided in a set of four interdependent components, reflected in the gasSim directory subdirectory structure:
 - a physics engine (PhysicsEngine subdirectory), which provides the implementation of the physical model and of the methods used to simulate its evolution through time
 - a graphics module (Graphics subdirectory), which provides a rudimentary three-dimensional rendering engine with facilities that allow to "take an almost perspectically correct picture" of the gas (doesn't take into account lateral stretching towards the edges of the screen
 - a data processing module (DataProcessing), which provides a set of facilities allowing for storage and analysis of data relative to the simulation and their processing into more meaningful data structures and quantities
 - a pipeline for the data being output from the simulation (found in DataProcessing as well), which allows for simultaneous (multithreaded-safe) storage of the raw simulation output, processing (both statistical and graphical), direct external access to the statistical and graphical processing results and/or composition of the processed data into a coherent video output
The last component has been developed under the necessity of avoiding either an unsustainable memory footprint or an excessive slowness of the program.
### PhysicsEngine
The physics engine provides the following set of components:
 - GSVector, a template floating point vector class, implementing the concept of three-dimensional vectors
This component allows for usage of the concept of a vector with the flexibility to choose the floating point data structure to use based on the necessities posed by the implementation. It provides the basic operations to perform on vectors (scalar multiplication, dot product, cross product).
 - Particle, a spherical uniform particle representation implementation.
This component allows for the representation of particles, sharing mass and radius (implemented for safe thread-access) across the implementation. It provides a minimal set of later-needed facilities for particle management and information access.
 - Collision, a set of three small structs providing the abstraction needed for management of particle-to-wall and particle-to-particle collision.
This component allows for free (and therefore lightweight) storage and comparison of collision information, and the method for resolution of a collision, 

## External libraries
The project depends on ROOT 6.36.00, which can be installed through the snap package manager or directly through its binary release, and on SFML 2.6.1, provided by the package libsfml-dev.
To install SFML, run:
```
sudo apt install libsfml-dev
```
## Execution instructions
The project provides a main executable, found under the name `idealGasSim` in the build directory. Execution allows for passing a .ini configuration file path, through the -c option.
### Customizing the parameters
The simulation tries to generate the gas as a set of particles with randomized speeds, with uniformly distributed module and direction, fitting them in a cubical lattice inside of the container; it then proceeds to simulate the system's evolution through a given number of events (collisions), and divides the collisions in sets of equal number, on which statistical analysis and "measurements" are performed. It either saves the video output or streams it as the simulation is run.
The user can customize the parameters determining the output through a .ini configuration file. A reference version, containing all of the customizable parameters and their accurate description, set to values that output a meaningful demo, can be found at `configs/gasSim_demo.ini`.
### Tweaking the ROOT input file
The configuration of the root data structures used during the simulation can be tweaked by regenerating the input file itself, from which they are loaded, using the macro mainInputFile.cpp from a ROOT prompt, which can be modified as desired. The requirements for the objects are as follows:
 - The objects that act as containers (TGraph, TH1D) must be empty
 - The objects must be saved under the same names and be of the same types as the ones in the default root file
As for the demo config file, the default file found at `inputs/input.root` provides a set of working and meaningful presets, and the macro used to generate it can be taken as a reference for what to feed the simulation.

## Generative AI usage
During developement ChatGPT has been used, almost exclusively as a way to get indicative information about the facilities provided by the used libraries, and in a couple of instances to write code snippets:
 - in the file found at `gasSim/DataProcessing/SDPGetVideo.cpp`, to write the RGBA32toRGBA8 auxiliary function
 - in the file found at `main.cpp`, to write the ffmpeg pipe opening command

## Writing period
This project has been written in the period between the 7th of May, 2024 and the 16th of January, 2026

