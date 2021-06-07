<h1> Advanced Graphics Progamming Engine </h1>

<h2>Deferred Engine </h2>

<p>Advanced Programming Project made by university students: Èric Canela and Jacobo Galofre</p>

![alt All](https://github.com/sherzock/Advanced-Graphics-Progamming-Engine/blob/main/webImgs/All.PNG)<br>

<h2>Githubs</h2>

[Jacobo Galofre](https://github.com/sherzock)
<br>
[Eric Canela](https://github.com/knela96)

<h2>Controls</h2>

<p>W/A/S/D : Move Camera (On Free Mode)</p>
<p>F: Toggle Orbital and Free Camera</p>
<p>Right mouse button: Rotate Camera</p>

<h2>Features</h2>

<p>Hierarchy of loaded models.</p>

![alt 1](https://github.com/sherzock/Advanced-Graphics-Progamming-Engine/blob/main/webImgs/hierarchy.PNG)

<br>

<p>Info window with opengl information and app info.</p>

![alt 2](https://github.com/sherzock/Advanced-Graphics-Progamming-Engine/blob/main/webImgs/info.PNG)

<br>

<p>Inspector to edit the selected entity.</p>

<p>Tranform to translate, scale, rotate and edit the different entities.</p>

![alt 3](https://github.com/sherzock/Advanced-Graphics-Progamming-Engine/blob/main/webImgs/inspector1.PNG)

<br>

![alt 4](https://github.com/sherzock/Advanced-Graphics-Progamming-Engine/blob/main/webImgs/inspector2.PNG)

<br>

<p>Game window with the render and two combo boxes to edit the render type (Color, Depth, Albedo, Normal, Position) and render modes (Forward or Deferred).
<br>
CheckBoxes to enable and disable bloom effect and Height map.</p>

![alt 5](https://github.com/sherzock/Advanced-Graphics-Progamming-Engine/blob/main/webImgs/renderbox.PNG)

<br>

![alt 6](https://github.com/sherzock/Advanced-Graphics-Progamming-Engine/blob/main/webImgs/modes.PNG)

<br>

<p>Parameters can be edited for bloom effect and Height map.</p>

![alt 7](https://github.com/sherzock/Advanced-Graphics-Progamming-Engine/blob/main/webImgs/BloomParams.PNG)

<br>

![alt 8](https://github.com/sherzock/Advanced-Graphics-Progamming-Engine/blob/main/webImgs/bumpParams.PNG)

<br>

<h2>How To Use Effects</h2>

All these effects are included in the same file named "shaders.glsl" inside the root folder.

<h3>Bloom</h3>

To use the bloom effect you can activate it by pressing the checkbox on the inspector which you will find at the right of the engine when opening it for the first time.
You can can modify the parameters to change the result of the effect. The parameters you can modify are: threshold, the kernel radius and the 5 LODs' intensity levels.

<br>

![Bloom Params](https://github.com/sherzock/Advanced-Graphics-Progamming-Engine/blob/main/webImgs/BloomParams.PNG)

<br>

<h3>Relief Mapping</h3>

To use the Relief Mapping effect you will have to toggle the checkbox on the inspector which you will find at the right of the engine when opening it for the first time.
You can modify the parameters to change the result of the effect. The parameters you can modify are: Bumpiness, Texture size and and the relief map steps performed.

<br>

![Bloom Params](https://github.com/sherzock/Advanced-Graphics-Progamming-Engine/blob/main/webImgs/bumpParams.PNG)

<br>

<h2>Effects Comparison Screenshots</h2>

<h3>Bloom</h3>

![alt 9](https://github.com/sherzock/Advanced-Graphics-Progamming-Engine/blob/main/webImgs/BloomOff.PNG)

<br>

![alt 10](https://github.com/sherzock/Advanced-Graphics-Progamming-Engine/blob/main/webImgs/BloomOn.PNG)

<br>

<h3>Relief Mapping</h3>

![alt 11](https://github.com/sherzock/Advanced-Graphics-Progamming-Engine/blob/main/webImgs/bumpOff.PNG)

<br>

![alt 12](https://github.com/sherzock/Advanced-Graphics-Progamming-Engine/blob/main/webImgs/bumpOn.PNG)

<br>
