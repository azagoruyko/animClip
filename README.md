# Anim Clip
Save animation to JSON and load it.

## Installation
Compile with Visual Studio and CMake.<br>
Load the plugin in Maya.

## Usage
Two commands are available in Maya when the plugin is loaded: `saveAnimClip` and `loadAnimClip`.

### Save animation.
1. Select animated controls (transforms).
2. Select range in time line.<br>
  If no range selected then just a pose will be saved.
3. Run `saveAnimClip -f "c:/clip.json"`<br>
  This saves animation into the file.

### Load animation.
1. Select controls you want to load an animation to.
2. Run `loadAnimClip -f "c:/clip.json"`<br>
  This restores animation from the file and applies it to controls from the current frame.
  
  You can execute `help saveAnimClip` or `help loadAnimClip` to see the additional flags.<br>
  Rotation order is always saved and restored. Namespaces are supported, of course.
