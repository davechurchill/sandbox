# MUN AI & Games Lab Augmented Reality Sandbox

![sandbox](https://davechurchill.ca/files/images/sandbox/sandbox_github.jpg)

[Sandbox Image Gallery](https://davechurchill.ca/files/images/sandbox/)

## Hardware and software

The hardware used for our sandbox is as follows:

- Any Windows computer with a decent CPU and GPU
- Intel RealSense Depth Camera D455
- BenQ 1080p DLP Gaming Projector TH575
- A sandbox configured so the projector fills the entire box and the camera sees the entire box

The software is written in C++20 and uses the following external libraries:

- SFML 3
- ImGui-SFML
- Intel RealSense SDK
- OpenCV 4.8.0

## Installation

SFML 3, the Intel RealSense SDK, and OpenCV must be installed separately.

Set the `SFML_DIR` environment variable to the SFML directory. For OpenCV 4.8.0, add the appropriate `opencv\build\x64\vc16\bin` directory to the system `PATH`. Installing the RealSense SDK normally provides the remaining runtime dependencies.

## Visualizers

A visualizer turns the current height map into an image or adds a visual effect to it. The **Visualizer** tab shows one collapsible row per visualizer, with an unlabeled checkbox that enables or disables it. Any number of different visualizers can be enabled together, with at most one instance of each type. Expanding one row closes the others and exposes that visualizer's settings.

Visualizers are drawn in their listed order. Full-terrain visualizers establish the scene image, while transparent visualizers add effects over it. Enabling more than one full-terrain visualizer is allowed, but a later opaque image can cover an earlier one. Canvas mouse input goes to source tools on the **Source** tab, the expanded enabled visualizer on the **Visualizer** tab, and projection controls on the **Projection** tab. Collapsing every visualizer prevents all visualizers from receiving canvas input.

Visualizer options and the enabled visualizer list are stored in `settings.json` when settings are saved.

### Colorizer

Maps normalized terrain height to one of five shader palettes: **Popsicle**, **Blue**, **Red**, **Terrain**, or **Animating Water**. Its built-in contour lines can be enabled independently and adjusted from 0 to 19 lines. The shader can be reloaded while the program is running.

### Fish Pond

Renders the terrain as animated water whose color changes with depth. Fish swim at individual depths, become smaller and darker as they swim deeper, avoid water that is too shallow for them, and school only with nearby fish in a similar depth band. Nearby members of the same depth band tend to share a color type so recognizable schools can form.

- **Fish Count** controls the population from 0 to 1000; the default is 100.
- **Swim Speed**, **Fish Size**, **School Radius**, **School Strength**, and **Separation Strength** tune movement and flocking.
- **Shallowest Fish Depth** and **Deepest Fish Depth** set the allowed swimming-depth range.
- Left-click the terrain to add a fish. **Reset Fish** recreates the configured population.

### Forest Fire

Uses the grassy-hills Nature terrain style, including ponds and rocky high ground. It starts with no trees: trees must be painted onto burnable terrain before fire can spread. Water and rock cannot hold trees or burn. Fire consumes tree fuel, leaves blended burnt ground, and is rendered with terrain-aligned flame particles rather than a scrolling fire texture.

- Left mouse paints trees, middle mouse removes trees, and right mouse ignites the forest.
- **Tree Brush Size**, **Tree Brush Blur**, and **Tree Paint Amount** control the tree brush.
- **Water Level** and **Rock Level** define non-burnable terrain.
- **Spread Rate**, **Burn Rate**, **Ignition Radius**, and **Wind X/Y** control the simulation.
- The visualizer reports burning cells, forested cells, and remaining fuel. It can be paused, randomly ignited, extinguished, or reset and cleared.

### Heat

Simulates heat diffusion across the terrain and colors the result as a heat map. Available implementations are **Average**, **Heat Equation**, **Heat Equation Kernel**, and **Heat Equation SIMD**.

- **Iterations Per Frame** controls continuous simulation speed; **Step** advances one iteration.
- **Reset** resets the temperature field, while **Clear Sources** removes all heat sources.
- The **Source** dropdown selects one existing hot or cold source. Hold the left mouse button and drag to move only that selected source.
- Optional contour lines can be displayed from 0 to 19 lines.

### Blockworld

Turns the height map into a block-based terrain visualization with quantized elevations, visible block edges and sides, and height-based water, sand, grass, rock, and snow materials. **Block Size** controls the visual cell size from 2 to 48 pixels.

### Nature

Provides natural terrain colorization with four selectable styles:

- **Grassy Hills** blends ponds, shore, grass, and high ground. **Pond Level** controls the waterline.
- **Rocky Cliffs** transitions gradually from grass to gray rock as height and slope increase.
- **Desert Sand** renders a dry sand and stone palette.
- **Alpine** renders high-altitude mountain terrain.

### TerrainLighting

Applies directional lighting and terrain relief to one of four palettes: **Terrain**, **Grayscale**, **Desert**, or **Ice**. **Light Azimuth**, **Light Elevation**, **Ambient Light**, **Shadow Strength**, and **Height Strength** control the result. Left-drag on the terrain to reposition the light direction interactively.

### Adjust Terrain Color

Applies shader-based color correction to the projected terrain while preserving visualizers drawn before it. **Brightness**, **Contrast**, **Exposure**, **Saturation**, **Hue**, **Gamma**, and **Temperature** can be adjusted independently or returned to neutral with **Reset Adjustments**.

### Animals

Adds a top-down wildlife simulation. Two sheep and one wolf are created by default when suitable terrain is available. Sheep wander with smooth steering, slow down uphill, avoid obstacles and nearby threats, and turn to face their movement direction. The slightly faster and larger wolf steers toward the nearest sheep; nearby sheep flee, and a sheep is removed when caught.

- Left-click suitable terrain to add a sheep.
- **Sheep Speed** and **Sheep Size** control the flock's appearance and motion.
- **Reset Animals** recreates the default population.

### BFS

Builds a height-weighted path field toward the right edge and sends particles into it from random positions along the left edge. Particles are spawned at a fixed rate and destroyed after reaching the right edge. Increasing **Height Penalty** makes higher terrain more expensive to cross.

Controls include **Spawn Rate**, **Trail Length**, **Particle Speed**, **Particle Alpha**, **Cell Size**, and **Height Penalty**, plus a live active-particle count and a reset button.

### Balls

Simulates shaded balls rolling downhill under gravity. Balls change apparent size with terrain height, cast shadows, show a center-to-motion direction mark, and collide with one another using configurable restitution. The first ball starts near the middle; later balls receive randomized colors.

- Left-click to add a ball. **Reset Balls** clears the scene and places one ball at a new random location.
- **Gravity**, **Ball Speed Multiplier**, **Rolling Resistance**, **Ball Size**, and **Ball Bounciness** tune the physics.
- **Trail Length** adds fading tracks. **Lava Appearance** switches the balls and trails to a glowing lava style.

### Cloth Sheet

Simulates an unpinned spring-mesh sheet falling and deforming over the terrain. The wireframe remains opaque while the fabric fill can be made nearly transparent, and lower parts of the grid are darkened slightly to make depth easier to read.

- Left-click to place a new sheet.
- **Cells X/Y** independently set mesh resolution from 2 to 64 cells per axis.
- **Cloth Size**, **Sheet Transparency**, **Spring Stiffness**, **Damping**, **Gravity**, and **Wind X/Y** control its shape and motion.

### Contour Lines

Adds topographic contour lines without replacing visualizers drawn before it. **Contour Lines** controls the line count from 0 to 64, while **Line Color** and **Line Opacity** control their appearance.

### Pathfinding (A*)

Calculates an eight-directional A* path over the current terrain every frame. Before each search, signed slopes are cached for every cell and movement direction using a five-tile directional average, reducing sensitivity to single-cell depth noise. The first left click sets the green start point, the second sets the red goal, and the next click begins a new pair. **Movement Length** makes each action advance from 1 to 32 cells and uses the same value as the goal-reached radius; every intermediate cell, corner, and slope is still validated. **Uphill Slope Penalty** and **Downhill Slope Penalty** independently make steep elevation changes more expensive so the search prefers flatter routes when available. **Maximum Legal Slope** completely rejects moves whose absolute averaged slope exceeds its threshold. **Path Thickness** controls the rendered path width. The UI reports geometric distance, slope-weighted cost, nodes expanded, open- and closed-list sizes, and total search time.

### Smoke and Fire

Adds temporary fires to burnable terrain without consuming the terrain or tree state. Each fire emits randomized, flickering square flame particles and rising smoke, can spread to nearby valid terrain, and eventually burns out.

- Left-click to ignite terrain; **Extinguish All** clears every active fire and particle.
- **Fire Size**, **Fire Lifetime**, **Spread Rate**, **Smoke Amount**, **Smoke Buoyancy**, and **Wind X/Y** control the effect.

### Vectors

Visualizes steering wind as white rectangular particles moving from left to right. Every particle receives a random vertical spawn position, height, and speed. Particles steer around terrain above their height; if no clear route exists, they cross the obstruction more slowly instead of pooling against it. They wrap from the right edge back to the left. Lower particles remain more opaque, while alpha falls sharply with increasing height.

Controls include **Spawn Rate**, **Trail Length**, **Base Particle Speed**, **Particle Alpha**, **Steering Distance**, **Minimum Wind Height**, and **Maximum Wind Height**, plus an active-particle count and reset button.

### Weather

Adds one of four atmospheric effects: **Rain**, **Snow**, **Fog**, or **Clouds**. **Intensity**, **Wind X/Y**, and **Element Size** apply to every mode; rain and snow also expose **Fall Speed**. Precipitation falls toward the terrain, while fog and cloud particles drift across it.

### WaterFlow

Simulates rainwater spreading downhill, accumulating in low areas, and fading through evaporation. **Rain Brush** is the default mode and releases rain only around a left-clicked or dragged point; **Uniform Rain** applies rainfall over the full terrain.

Controls include **Rainfall Rate**, brush **Rain Radius**, **Flow Speed**, **Evaporation**, **Water Depth Scale**, **Simulation Steps**, **Trail Persistence**, **Water Visibility**, **Water Opacity**, and **Water Color**. **Reset Water** clears the accumulated water.
