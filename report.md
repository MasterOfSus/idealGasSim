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
The concept of a gas is represented as a set of identical rigid spherical particles, freely moving inside of a cubical container, subject only to elastic collisions both with each other and with the container's walls.
This system is then observed in its evolution through time, through the elaboration of the simulated events' data into statistical values and datasets, trivially convertible to the thermodynamical quantities of interest.
Finally, the resulting statistical and thermodynamical data is converted to both a graphical format (a video, which can be viewed in real time while the simulation runs, or stored in an mp4 file for repeated viewing) and a statistical format, through the ROOT framework's file output functionality, saved in a ROOT file.
To make the visualization of the phenomena more intuitive, a direct gas rendering and display functionality has been implemented and integrated with the graphical statistics visualization, providing a unified video output.

The project has also been structured so as to allow for free and customizable usage of the implemented facilities, as these allow for a wider range of data processing than that which has been implemented in the main executable.

The project can be seen as divided in a set of three major interdependent components and a minor one, reflected in the gasSim directory's subdirectory structure:
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
This component allows for the representation of particles, sharing mass and radius (implemented for thread-safe access).
 - Collision, a set of three small structs providing the abstraction needed for management of particle-to-wall and particle-to-particle collision.
The dual nature of a collision has been dealt with by making use of dynamic polymorphism, so as to provide an uniform interface (implemented in the pure virtual Collision struct) for the "collision solving" method and collision completion time class member, used to compare collisions to choose the one with the smallest time.
These structs have been made with speed in mind, as they are extensively used in the main computational bulk of the simulation, and have therefore been implemented without ensuring correct usage of the provided methods (requiring additional overhead), which has been instead delegated to the Gas class itself.
The collision solving methods are implemented according to the following formulas:
For particle-to-particle collisions, the resulting speeds can be calculated by imposing three conditions: conservation of kinetic energy (elastic collision), conservation of momentum (principle of conservation of momentum) and finally, for the momentum exchange between the particles to be a vector linearly dependent with the vector connecting the two spheres' centers, as is the case for the case of repulsive forces being normal to the contacting surfaces. These conditions result in the following general solution to the problem:
~insert formula~
For particle-to-wall collisions, the coordinate relative to the wall's perpendicular axis is flipped, as per the limit of a collision between an object with finite mass and one with mass approaching infinity (which corresponds to our containers' walls, which are fixed in place)
 - Gas, the class implementing the concept of an ideal gas.
This Class provides two main facilities:
 - Constructors allowing the user to have full control over the desired starting conditions of the particles.
 - Methods allowing the user to simulate a given number of interactions, one just to make the system progress, oneoutputting the collision data to the simulation output pipeline.
The simulation methods rely on the process of getting all possible collision and comparing the time they would take to happen through the getTime() method, selecting the one with the smallest time and moving the entire gas by that time, then calling the solve() method over said collision.
The collision finding process, implemented in the firstPPColl() and firstPWColl() methods, is implemented as follows:
first the first particle-to-wall collision is found by computing the collision time over the whole container of particles, substituting a fixed collision's value every time one with smaller time is found. The final result is stored in a "best collision".
then the first particle-to-particle collision is found by computing the collision time for all couples of particles. The time computation is divided in two steps, the first checks that the relative distance of the two particles has negative dot product with the relative speed of the two particles, a cheap computation which provides the state of a condition necessary to the existance of a finite collision time, then if this step succeeds the actual collision time is computed, through the following formula, which results from imposing the distance of the two particles being equal to the sum of their radiuses:
this quadratic formula usually yields two values, of which the one with smallest modulus is selected.
this is done across the whole set of particles with multiple threads, using triangular indexing to biject the set of all couples of particles with a set of indexes, through the following formula:

once the two "best" collisions are found, they are compared and the one with the smallest time is selected, then the gas particles are shifted through their speeds by that time, and the collision, once "contact" has been achieved, is solved. This whole process simulates one event, and is repeated for the amount of times specified by the user. In the case that the overload providing data recording is used, the simulation data is recorded inside of a given instance of the pipeline class after the resolution of the collision has been completed.
### Graphics
This module provides two components:
 - RenderStyle, a simple collection of rendering parameters determining the aesthetical characteristics of a drawn gas 
 - Camera, a class implementing the concept of perspective, allowing for a rudimentary visually intuitive (almost perspectically correct) 3D rendering of a gas.

The camera class provides a fundamental point projection method, turning a point in 3D space into its projection, with its pixel coordinates, which can be used to draw it onto an image, and a depth field, which contains the information about its distance from the camera plane, which among other things can be used for scaling purposes.
This point projection method is then used to add particle drawing functionalities, by drawing a circle where the particle should be and then scaling it according to the depth field. These are used to provide two methods allowing for the drawing of a set of particles in space, which requires only the additional ordering, based on the depth field, of the projections so as to then iterate over the projections container and draw them one over another, having the closest ones seen above the farthest, as is the case for convex objects such as spheres are.
Finally, the point projection and particle drawing methods are used to add a set of helper functions which make it possible to draw a Gas, through drawing its walls and its particles. Since the Gas class guarantees the enclosure of the particles inside of their container, correctly drawing the gas is easily achieved by drawing the walls facing away from the camera, then the particles over them, then the walls facing the camera over them, as the geometry of a convex object guarantees that looking at it from one direction will have the surfaces hidden from view possess normal vectors facing the same way as the observer's sight, therefore facing away from the observer. The cube happens to be a convex object. The information about the gas's normal vectors has been computed by "brute-forcing"the six cases of the wall, as there was no need for the additional implementation which would have been required to implement it in a more implicit way. SILLYYYYY

### DataProcessing
This module provides three components, the third of which will be explained in its own section:
 - GasData, which is essentially a Gas snapshot with collision information associated. It has been developed for storage of solved collisions, meaning that the contact between the colliding objects is still present, but the speeds have already been changed according to the collision solution.
 - TdStats, a class providing the facilities to process GasData instances into meaningful information. This is the class that turns the simulation data into "measurements"
 This class supports the bulking together of any number of subsequent GasData instances from the same Gas instance, and their processing into information of interest:
 * elapsed time
 * "temperature" (again, just average energy over a degree of freedom)
 * side of the box -> walls area, box volume
 * cumulated wall pulses (necessary for average pulse/time -> average force -> average pressure)
 * last collision positions -> the possibility to calculate the free paths for each particle at the time of their collision -> the mean free path
 * Histogram filled with the norm of the speed of every particle -> possibility to compare with a Maxwell-Boltzmann distribution
The "insertion" of a new GasData instance is checked for some minimal compatibility requirements
The copy and move constructors/assignment operators had to be implemented as non-standard to ensure that ROOT's internal memory management would not cause segmentation faults by calling the TH1 method SetDirectory(nullptr).

#### SimDataPipeline
The last component of the DataProcessing module, providing a complete, thread safe-ish simulation data pipeline, with two exit points: one for raw processed data in the form of TdStats and/or sf::Textures, and another for a video output, in the form of sf::Textures, containing a customizable assortment of the previous raw results in a graphical form.

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

