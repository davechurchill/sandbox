# MUN AI & Games Lab Augmented Reality Sandbox

![Sandbox](https://davechurchill.ca/files/images/sandbox/sandbox_github.jpg)

An interactive augmented-reality sandbox that turns live or generated height maps into projected terrain, simulations, and composited visual effects.

[Sandbox image gallery](https://davechurchill.ca/files/images/sandbox/)

## How it works

An overhead depth camera continuously measures the shape of the sand and turns those measurements into a greyscale depth image. The sandbox applies the selected colors, water, weather, animals, and other effects to that shape before a projector maps the finished scene directly back onto the sand. As people reshape the sand, the camera measures the new surface and the whole process repeats, allowing the projected world to respond immediately.

[![AR Sandbox workflow](https://raw.githubusercontent.com/wiki/davechurchill/sandbox/images/diagram_simple.png)](https://raw.githubusercontent.com/wiki/davechurchill/sandbox/images/diagram_simple.png)

## Documentation

Complete setup and usage documentation is maintained in the [GitHub Wiki](https://github.com/davechurchill/sandbox/wiki):

- [Installation](https://github.com/davechurchill/sandbox/wiki/Installation)
- [Using the Sandbox](https://github.com/davechurchill/sandbox/wiki/Using-the-Sandbox)
- [Terrain Sources](https://github.com/davechurchill/sandbox/wiki/Sources)
- [Visualizer Reference](https://github.com/davechurchill/sandbox/wiki/Visualizers)
- [Projection and 3D View](https://github.com/davechurchill/sandbox/wiki/Projection-and-3D-View)
- [Settings and Snapshots](https://github.com/davechurchill/sandbox/wiki/Settings-and-Snapshots)
- [Architecture and Frame Flow](https://github.com/davechurchill/sandbox/wiki/Architecture-and-Frame-Flow)
