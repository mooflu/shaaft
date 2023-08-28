# Introduction
Shaaft is a three-dimensional block stacking game. The blocks fall into a shaft (also known as the pit). The aim of the game is to complete planes (instead of rows). Incomplete layers cause the blocks to build up and reduce the available space. The game ends when some blocks no longer fit into the shaft.

![image](https://github.com/mooflu/shaaft/assets/693717/87e89d27-6571-47bc-b8e0-ed95847796e2)

# Scoring

Points are given when a block is locked. Its visual representation changes from the outline (which you can move and rotate) to the solid elements that accumulate at the bottom of the shaft.

The number of points is dependent on the number of elements in a block, the block's complexity, as well as the current level. Dropping a block earlier (using SPACE) results in  more points as well.

When a plane is completed (and disappears) additional points are given out. Completing multiple planes at a time gives more points. 

The 20 seconds following a Moo-Hachoo all points scored are doubled.

# Menu
The menu hierarchy looks like this:
```
New Game
Scores
Setup
  Controls
  Display
  Sound
Help
About
  Credits
Quit
```

## New Game

This starts a new game and will terminate a game in progress.

## Scores
You can view the top-10 for each shaft dimension. Use the left/right arrow keys to switch between the different dimensions. It defaults to the shaft dimension set in the Setup preferences. Hovering the mouse over a top-10 entry will show the time the score was achieved at the bottom of the screen.

## Setup
Within this menu entry you can set various preferences. Some of these setting will only take effect the next time a new game is started.

### Start Level

  There are 9 levels. Higher levels cause the blocks to drop faster. 

### Shaft width/height/depth

Sets the shaft dimension for the next new game.

### Blockset

There are a number of sets to choose from ranging from a simple to a complex set of blocks. Complex blocks will make it much harder to complete planes.

### Rotation/Move speed

Sets the speed at which the block to be placed is rotated and moved. Note that this setting is only used for the display of a smooth transition. The block moves/rotates to its next orientation/position as soon as you press the corresponding key.

### Show Next

When checked shows the next block that will appear after the current block is locked. 

### Show Indicator

When checked shows an indicator (target icon) where the block will land.

### Shaft Tilting

When checked the shaft can be tilted using the mouse. To return to its normal position press Command-0 (zero) on the Mac or Control-0 on Windows.

### Practice Mode

When checked the next new game will be run in practice mode. No score is kept and blocks will not drop automatically.

## Controls

To set up custom key mappings:

1. Select the control you want to change (red highlight)
2. Press Enter or mouse button 1 (highlight will turn yellow)
3. Press the key you want assign or Escape to cancel

You can also change the mouse sensitivity and smoothing factor. 

## Display

### Fullscreen

Check this to switch to fullscreen. Make sure that the resolution you selected is supported by your monitor. If things go wrong you can press SHIFT-` (backtick) to return to a resolution of 800x600.

### Resolution

Click on the resolution numbers to change and then click on the check box to activate the resolution you selected.

### Grab mouse

When checked and in windowed mode the mouse cursor will not be able to leave the window.

### Show FPS

When checked shows the number of frames per second (top right) the game is displaying. It's a crude way to measure how fast your computer and graphics card are.

### Draw Solid Shaft Tiles

When checked draws the shaft tiles solid.

### Draw Shaft Tile Frame

When checked draws the frame of a shaft tile.

### Antialiased Lines

When checked draws smooth lines. 

### Mouse Cursor Animation

When checked animates the mouse cursor.

### Show Score Updates

When checked displays score updates within the shaft.

## Sound

### Play music

When checked plays the default soundtrack. You may want to turn this off and simply listen to your iTunes library.

### Music Volume

Allows you to set the music volume.

### Effects Volume

Allows you to set the effects volume.

## Help

This page displays some simple reminders of how the game and the scoring works.

## About/Credits

These pages display the copyright notice and credits.

## Quit

Exits the game.
