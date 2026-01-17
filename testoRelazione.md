# idealGasSim
## Authors
Lorenzo Liam Baldisserri
The project was also built on the work of the previous group members:
Diego Quarantani
Niccolo Poli
Tomaso Tamburini

## Introduction
This project is a rudimentary event-based gas simulator operating in an euclidean three-dimensional space, with graphic data visualization and ROOT file output functionalities. It has the intent of allowing for clear visualization of the most basic phenomena described in kinetic ideal gas theory, and the qualitative comparison of the theoretical results provided by said theory with their simulated counterparts.
It strives to achieve this by running a physical simulation of a physical system, representing a gas, and by subsequent processing of the raw simulation data into meaningful statistical datasets and thermodynamical variables, mainly stored through the formats provided by the ROOT framework, so that these can both be visualized and saved for later fruition.
The visualization is achieved through the output of a video feed (live or saved to mp4) which enables the user to view the system's progression through time, both through its statistical and thermodynamical characteristics and through direct visualization of the system itself.
The provided experience is customizable to a limited degree, in almost all of its aesthetical characteristics and simulation parameters.
Finally, the developed facilities are also made accessible through the project's library, which can be used to implement more extensive and precise data processing, if desired.

## Implementation choices
The concept of a gas has been abstracted to a set of identical rigid spherical particles, freely moving inside of a cubical container, subject only to reciprocal collisions and collision with the container's inner walls.
This system is then observed in its evolution through time, through the elaboration of the simulated events' data into statistical values and datasets, which are then trivially converted to the thermodynamical quantities of interest.
Finally, the resulting statistical and thermodynamical data is converted to both a graphical format (a video, which can be viewed in real time while the simulation runs, or stored in an mp4 file for repeated viewing) and a statistical format, through the ROOT framework's file output functionality, saved in a ROOT file.
To make the visualization of the phenomena more intuitive, a direct gas rendering and display functionality has been implemented and integrated with the graphical statistics visualization, providing a unified video output.

The project has also been structured so as to allow for free and customizable usage of the implemented facilities, as these allow for a wider range of data processing than that which has been 

## External libraries
The project depends on ROOT 6.36.00, which can be installed through the snap package manager or directly through its binary release, and on SFML 2.6.1, provided by the package libsfml-dev.
To install SFML, run:
```
sudo apt install libsfml-dev
```
## Execution instructions
The project provides a main executable, found under the name idealGasSim in the build directory. Execution is done through the -c option, which allows to pass a .ini config file path. A demo config file, named gasSim_demo.ini, is provided inside the configs directory, it contains all of the input parameters supported by the simulation, set to defaults which run smoothly even on my 2013 potato laptop.
### Parameters explanation
The simulation tries to generate the gas as a set of particles with randomized speeds, with uniformly distributed module and direction, fitting them in a cubical lattice inside of the container, according to the following parameters:
 - nParticles, the number of particles to simulate
 - pMass, the common particle mass
 - pRadius, the common particle radius
 - targetT, the temperature to reach
 - boxSide, the side of the container
The simulation is run according to the following parameters:
 - nIters, the number of events to simulate
 - nStats, the number of events that constitute one "measurement" of the desired quantities
 - ROOTInputPath, specifies the name given to the ROOT input file, which will be looked for at inputs/name.ROOT
The video output is composed according to the specified values, and is either viewed live while the simulation is running, or saved to an mp4 file:
 - videoOpt, the video option to use for the output (all, justStats, justGas or gasPlusCoords)
 - saveVideo, true saves the video to an mp4 file, false views it live
 - targetBuffer, number of viewing seconds to reach before displaying a live video portion
 - videoOutputName, the name to save the video as: it will be saved at outputs/videos/name.mp4
 - ROOTOutputName, same as the ROOTInputPath variable, saves the ROOT data at outputs/name.mp4
 - videoResx, video width in pixels
 - videoResy, video heigth in pixels
 - framerate, video framerate
 - mfpMemory, keep mean free path information or not
 - bufferingWheel, name of the path for the buffering wheel image, will be looked for at assets/name.png
The rendering of the gas is done according to the following parameters:
 - wallsColor, hex color, alpha field last
 - wallsOpts, can include, in any order, u (up), d (down), l (left), r (right), f (front), b (back), referring to how the box walls are seen looking in the direction of the y axis.
 - particleTexName, the name of the particle texture, which will be looked for at assets/name.png
 - fontPath, the name of the font file, which will be looked for at assets/name.ttf
 - camPos, written as a 3-element vector: {x, y, z}
 - camLength, the camera "length", the bigger it is, the bigger the camera
 - fov, camera fov, in degrees
 - gasBGColor, gas background color
 - placeHolderName the name of the placeHolder texture, which will be looked for at assets/name.png
### ROOT input file tweaking
The configuration of the root data structures used during the simulation can be tweaked by regenerating the input file itself, from which they are loaded, using the macro mainInputFile.cpp from a ROOT prompt, which can be modified as desired. The requirements for the objects are as follows:
 - The objects must be empty
 - The objects must be saved under the same names and types

## Generative AI instruments usage
During developement ChatGPT has been used, almost exclusively as a way to get indicative information about the facilities provided by the used libraries, and in a couple of instances to write code snippets:
 - in the file found at `gasSim/DataProcessing/SDPGetVideo.cpp`, to write the RGBA32toRGBA8 auxiliary function
 - in the file found at `main.cpp`, to write the ffmpeg pipe opening command

## Writing period
This project has been written in the period between the 7th of May, 2024 and the 16th of January, 2026

