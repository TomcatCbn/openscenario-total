odrviewer is a simple application for viewing OpenDRIVE files.

Optionally it can populate the road lanes with randomized dummy vehicles, which will go until end of road then starting over from initial position. If reaching a junction with multiple options, it will randomly chose its way out.

Application is launched from command line (or batch file). 

Usage : [options]
Options:
  --odr <odr_filename>
      OpenDRIVE filename
  --model <model_filename>
      3D Model filename
  --density <density>
      density (cars / 100 m)
  --speed_factor <speed_factor>
      speed_factor <number>
  --osi_lines
      Show OSI road lines (toggle during simulation by press 'u')
  --osi_points
      Show OSI road points (toggle during simulation by press 'y')
  --road_features
      Show OpenDRIVE road features (toggle during simulation by press 'o')
  --path <path>
      Search path prefix for assets, e.g. car and sign model files
  --logfile_path <path>
      logfile path/filename, e.g. "../esmini.log" (default: log.txt)
  --disable_log
      Prevent logfile from being created
  --disable_stdout
      Prevent messages to stdout
  --help
      Show this help message

Additional options forwarded to OpenSceneGraph:
  --window <x y w h>
          Set the position (x,y) and size (w,h) of the viewer window.		
  --screen <num>
          Set the screen to use when multiple screens are present.
  --clear-color <color>
          Set the background color of the viewer in the form "r,g,b[,a]".		  

Example 1 - View the ODR file and some random traffic on a 3D model, window mode 1000 x 500:

   odrviewer --odr xodr\e6mini.xodr --model models\e6mini.osgb --window 60 60 1000 500

Example 2 - just ODR, fullscreen

   odrviewer --odr xodr\e6mini.xodr

Example 3 - remove traffic

   odrviewer --odr xodr\e6mini.xodr --model models\e6mini.osgb --density 0 --window 60 60 1000 500

Example 4 - sparse traffic (about 0.5 vehicle per 100 meter = 1 per 200 m)

   odrviewer --odr xodr\e6mini.xodr --model models\e6mini.osgb --density 0.5 --window 60 60 1000 500


Key shortcuts

    H (shift h): This help text
    TAB:         Move camera to next vehicle
    Shift - TAB: Move camera to previoius vehicle
    o:           Toggle show/hide OpenDRIVE road feature lines
    u:           Toggle show / hide OSI road lines
    y:           Toggle show / hide OSI road points
    p:           Toggle show / hide environment 3D model
    , (comma):   Toggle entity view : Model / None
    ESC:         quit

    1 - 9: Camera models acording to :
        1: Custom camera model
        2: Flight
        3: Drive
        4: Terrain
        5: Orbit
        6: FirstPerson
        7: Spherical
        8: NodeTracker
        9: Trackball

    When custom camera model(1) is activated
        k: Switch between the following sub models:
           - Orbit        (camera facing vehicle, rotating around it)
           - Fixed        (fix rotation, always straight behind vehicle)
           - Flex         (imagine the camera attached to vehicle via an elastic string)
           - Flex - orbit (Like flex but allows for roatation around vehicle)
           - Top          (top view, fixed rotation, always straight above vehicle)

    Viewer options
        f: Toggle full screen mode
        t: Toggle textures
        s: Rendering statistics
        l: Toggle light
        w: Toggle geometry mode(shading, wireframe, dots)
        c: Save screenshot in JPEG format - in the folder where the application was started from
        h: Help

Mouse control

    Left:   Rotate
    Right:  Zoom
    Middle: Pan

    This is typical, exact behaviour depends on active camera model.
