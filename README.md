# Polychora

💎 A 4D renderer and polychoron generator.

<p align="center">
  <img src="https://raw.githubusercontent.com/mwalczyk/four/master/screenshots/polychora_slicing.gif" alt="screenshot" width="500" height="auto"/>
</p>

## Description

### Projections

After seeing videos of [Miegakure](http://miegakure.com/) gameplay, I became very interested in the idea of 4-dimensional rendering. It turns out that there are several ways to visualize higher dimensional objects. Perhaps the simplest method involves a projection from 4D to 3D. This is similar to how "traditional" 3D engines render objects to a 2D display surface (your screen). A 4D-to-3D projection can either be a perspective or parallel (orthographic) projection. However, instead of projecting objects onto a flat plane (as is the case in a 3D-to-2D projection), we project our 4-dimensional objects "onto" a 3-dimensional cuboid (in this case, the unit cube centered at the origin). From here, we simply perform the 3D-to-2D projection that we are used to.

Most people are familiar with this form of visualization. When a 4D-to-3D perspective projection is applied to the 8-cell (also known as the hypercube or [tesseract](https://en.wikipedia.org/wiki/Tesseract)), the result is the familiar animation of an "outer" cube with a smaller, nested "inner" cube that appears to unfold itself via a series of 4-dimensional plane rotations. The reason why the inner cube appears smaller is because it is "further away" in the 4th dimension (the `w`-axis of our 4D coordinate system). If you are interested in this form of visualization, I highly recommend Steven Hollasch's [thesis](http://hollasch.github.io/ray4/Four-Space_Visualization_of_4D_Objects.html#chapter4), titled _Four-Space Visualization of 4D Objects_.

### Slicing

A different way to visualize 4-dimensional objects is via a "slicing" procedure, which produces a series of 3-dimensional cross-sections of the full 4-dimensional shape. This is analogous to "cutting" a 3D polyhedron with a 2-dimensional plane (think MRI scans). Luckily, much of the math carries over to 4D. In order to facilitate this process, meshes in `polychora` are represented by a "shell" of tetrahedra. 

Over the course of this research project, I have explored 2 different ways of generating polychora. Currently, shapes are generated in a fully procedural manner using combinatorics and [QHull](http://www.qhull.org/), a library for computing n-dimensional convex hulls. The coordinates of the vertices of all uniform + regular polychora are known and can be calculated via a set of even/odd permutations and changes-of-sign. For example, the 120-cell can be generated via the following "permutation table" (taken from the amazing catalog available at [Eusebeia](http://eusebeia.dyndns.org/4d/120-cell)):

```
All permutations and changes-of-sign of:

  (2, 2, 0, 0)
  (√5, 1, 1, 1)
  (φ, φ, φ, φ−2)
  (φ2, φ−1, φ−1, φ−1)

Together with all even permutations and changes-of-sign of:

  (φ2, φ−2, 1, 0)
  (√5, φ−1, φ, 0)
  (2, 1, φ, φ−1)

where φ is the Golden Ratio.

```

I wrote a C++ header file for enumerating such permutations, together with help from the C++ standard template library and a header file from Eusebeia (for generating _only_ the [even permutations](https://en.wikipedia.org/wiki/Parity_of_a_permutation)). Once the vertices are generated, we can pass them to QHull's convex hull algorithm. Enabling the `Qt` "triangulation" flag results in a list of simplical facets, which can be directly passed into the slicing pipeline.

Previously, polychora were generated in a more ad-hoc manner, using a variety of modified "shape files" that I found on the internet (mostly from Paul Bourke's [website](http://paulbourke.net/geometry/hyperspace/)). You can find the corresponding code in an [older, less polished version](https://github.com/mwalczyk/four) of this project, which was also written in Rust. The advantage of this approach was that it gave access to all of the connectivity information of each polychoron: vertices, edges, faces, and cells. However, because the resulting facets weren't necessarily tetrahedral, I had to algorithmically "tetrahedralize" each mesh before rendering.

Tetrahedralization in 4-space is similar to triangulation in 3-space. In particular, [any 3D convex polyhedron can be decomposed into tetrahedrons](https://mathoverflow.net/questions/7647/break-polyhedron-into-tetrahedron) by first subdividing its faces into triangles. Next, we pick a vertex from the polyhedron (any vertex will do). We connect all of the other face triangles to the chosen vertex to form a set of tetrahedra (obviously, ignoring faces that contain the chosen vertex). This is not necessarily the "minimal tetrahedral decomposition" of the polyhedron (which is an active area of research for many polytopes), but it always works. An example of this process for a regular, 3D cube can be found [here](https://www.ics.uci.edu/~eppstein/projects/tetra/).

So, I started with a 4D polychoron, whose "faces" (i.e. "cells") are themselves convex polyhedra embedded in 4-dimensions. One by one, I "tetrahedralized" each cell. Together, the sum of these tetrahedra formed the desired 4-dimensional mesh. For example, the hypercube has 8 cells, each of which is a cube (this is why the hypercube is called the 8-cell). Each cube produces 6 distinct tetrahedra, so all together, the tetrahedral decomposition of the hypercube results in 48 tetrahedra. For a visual example of this procedure in 3-space, see the following Stack Overflow post: ["How to Make an Icosahedron from 20 Tetrahedra"](https://math.stackexchange.com/questions/1340470/how-to-make-an-icosahedron-from-20-tetrahedra).

Regardless of the way in which the polychora are generated, one question remains: what is the importance of maintaining tetrahedral facets? It turns out that slicing a tetrahedron in 4-dimensions is much simpler than slicing the full cells that bound the polychoron. In particular, a sliced tetrahedron (embedded in 4-dimensions) will always produce zero, 3, or 4 vertices, which makes things quite a bit easier (particularly, when it comes to computing vertex indices for OpenGL rendering).

### Implementation of the Slicing Procedure

Each mesh in `polychora` maintains a GPU-side buffer that holds all of its tetrahedra (each of which is an array of 4 vertices). The slicing operation is performed via a compute shader that ultimately generates a new set of vertices (the "slice") for each tetrahedron. The same compute shader also generates draw commands on the GPU, which are later dispatched via `glMultiDrawArraysIndirect`. Essentially, each tetrahedron will generate its own unique draw command that renders either 0, 1, or 2 triangles, depending on whether the slicing operation returned an empty intersection (0), a single triangle (1), or a quad (2). 

In the case where a tetrahedron's slice is a quad, care needs to be taken in order to ensure a proper vertex winding order. This too is handled in the compute shader: the 4 vertices are sorted based on their signed angle with the polygon's normal. This is accomplished via a simple insertion sort. In GLSL, this looks something like:

```glsl
uint i = 1;
while(i < array.length())
{
    uint j = i;
    while(j > 0 && (array[j - 1] > array[j]))
    {
        vec2 temp = array[j];
        array[j] = array[j - 1];
        array[j - 1] = temp;

        j--;
    }
    i++;
}
```

After the slices are computed, the resulting object is rendered via a separate vertex / fragment shader that simply performs an orthographic 4D-to-3D projection. 

## Tested On

- Windows 10
- NVIDIA GeForce GTX 1660 Ti

NOTE: this project will only run on graphics cards that support OpenGL [Direct State Access](https://www.khronos.org/opengl/wiki/Direct_State_Access) (DSA) and C++17.

## To Build

1. Clone this repo and initialize submodules: 
```shell
git submodule init
git submodule update
```
2. From the root directory, run the following commands:
```shell
mkdir build
cd build
cmake ..
```
3. Open the project file for your IDE of choice (generated above)
4. Build and run the project

## To Use

`polychora` currently supports 5 regular polychora - 8-cell, 16-cell, 24-cell, 120-cell, 600-cell - as well as several uniform polychora.

To rotate the camera around the object in 3-space, press + drag the left mouse button. You can zoom the camera in or out using the scroll wheel.

There are 6 possible plane rotations in a 4-space (see `maths.h` for more details), and these are exposed to the user by 6 float sliders. There is also a toggle for changing between wireframe and filled modes.

Finally, you can toggle between 3 different projections / draw "modes" by repeatedly pressing `t`:
1. Slices: show the 3-dimensional slice of each polychoron, as dictated by the aforementioned "slicing hyperplane"
2. Tetrahedral wireframes: show the 3-dimensional projection of the 4-dimensional tetrahedral decomposition of each polychoron
3. Skeleton: show the 3-dimensional projection of the wireframe of the 4-dimensional polychoron 

All of the draw modes listed above will be affected by the 4-dimensional rotations mentioned prior.

## To Do

- [ ] Add 4-dimensional "extrusions" (i.e. things like spherinders)
- [ ] Add "hollow"-cell variants of each polytope (see Miegakure)
- [ ] Research 4-dimensional knots, un-tying, un-boxing, etc.
- [ ] Add simple lighting (calculate 3D normals after slicing procedure)
- [ ] Research Munsell color solids

## Future Directions

"T" (the author of Eusebeia, mentioned below) recommended the "double description method" for computing higher-dimensional convex hulls. This algorithm is implemented in the [cddlib](https://github.com/cddlib/cddlib) library and supposedly produces more geometrically accurate results. "T" also mentioned that many convex hull algorithms (including QHull) will "triangulate" the output, breaking it into n-dimensional simplices during execution. Since a 4-simplex is a tetrahedron, this makes the slicing procedure relatively straightforward. However, the resulting topology usually doesn't feel as "clean" as the output that I initially generated by manually tetrahedralizing the mesh. This might be because the resulting tetrahedra don't necessarily have uniform volume / size. Disabling the `Qt` triangulation flag in QHull would likely produce a set of non-simplical facet cells, which could be tetrahedralized manually using the aforementioned procedure, but this requires further investigation. 

Another idea I had exploits the fact that the dual of a regular polychoron is itself a regular polychoron. For example, the 120-cell and 600-cell are dual to each other. This means that the vertices of the 600-cell (of which there are 120) correspond to the cell centers of the 120-cell. Recall that the 120-cell is composed of 120 dodecahedral cells. Generating the vertices, edges, and faces of a dodecahedron is easy, so we could, in theory, create 120 dodecahedrons (embedded in 4-space) and center each of them at one of the vertices of the 600-cell. This construction would give us the complete 120-cell. One problem I could forsee is, each of the dodecahedral cells may require some sort of rotation in order to form a fully closed hyper-surface. Let's think about the analogous problem in one lower dimension: a dodecahedron is composed of 12 pentagonal faces. We can create 12 regular pentagons in the XY-plane and translate each so that its center point coincides with one of the 12 vertices of the dodecahedron's dual (the icosahedron). To form a closed surface, we must rotate each face so that its outward normal aligns with the vector pointing from the dodecahedron's center towards the facet center.

The software package [Stella4D](https://www.software3d.com/StellaManual.php?prod=stella4D) also has some interesting notes in the manual.

Marc Ten Bosch (the author of Miegakure) has written extensively on the topic of geometric algebra, particularly rotors and bivectors. These are alternate (more intuitive?) ways of representing orientation in n-dimensions. In the future, I would love to explore this topic, as well as [rigid body dynamics in 4-space](https://marctenbosch.com/news/2020/05/siggraph-2020-technical-paper-n-dimensional-rigid-body-dynamics/).

## Credits

The idea to use a convex hull algorithm came from "T," the author of [Eusebeia](http://eusebeia.dyndns.org/4d/). Their site is one of the most amazing resources on higher-dimensional rendering. Throughout the early stages of this project, "T" answered many of my (probably dumb) questions and provided a great deal of help and guidance. 

Thanks to [@GSBicalho](https://github.com/GSBicalho) and [@willnode](https://github.com/willnode) for their additional guidance throughout this project. Their responses to my questions were vital towards my understanding the 4D slicing procedure. 

Thanks to [Dr. Libor Vasa](http://meshcompression.org/) who also answered several of my questions early on about the 120-cell.

Other resources that I found helpful throughout the creation of this project include:

- [A General Purpose Animation System for 4D](https://scholarsarchive.byu.edu/cgi/viewcontent.cgi?article=7968&context=etd)
- [GL4D](http://www.cse.cuhk.edu.hk/~cwfu/papers/GL4D/paper/GL4D.pdf)
- [Generating and Rendering Four-Dimensional Polytopes](http://torus.math.uiuc.edu/jms/Papers/dodecaplex/new.pdf)
- [A Visualization Method of Four Dimensional Polytopes by Oval Display of Parallel Hyperplane Slices](https://arxiv.org/pdf/1607.01102.pdf)
- [Visualizing Higher-Dimensional Space-Time and Space-Scale Objects as Projections](https://peerj.com/articles/cs-123.pdf)
- [N-Dimensional Rigid Body Dynamics](https://marctenbosch.com/ndphysics/NDrigidbody.pdf)
- ["How Should I Handle (Morphing) 4D Objects in OpenGL?"](https://stackoverflow.com/questions/44939879/how-should-i-handle-morphing-4d-objects-in-opengl?noredirect=1&lq=1)
- ["Calculate 3D Cross-Section of 4D Shape"](https://stackoverflow.com/questions/50008610/calculate-3d-cross-section-of-4d-shape-tesseract/50016144#50016144)
- [Four Dimensional Dice Up To Twenty Sides](http://www.polytope.net/hedrondude/dice4.htm)
- [Normal Vectors in Higher Dimensions](https://ef.gy/linear-algebra:normal-vectors-in-higher-dimensional-spaces)

### License
[Creative Commons Attribution 4.0 International License](https://creativecommons.org/licenses/by/4.0/)
