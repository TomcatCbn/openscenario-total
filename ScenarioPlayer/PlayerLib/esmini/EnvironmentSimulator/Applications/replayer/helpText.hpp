/*
 * esmini - Environment Simulator Minimalistic
 * https://github.com/esmini/esmini
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) partners of Simulation Scenarios
 * https://sites.google.com/view/simulationscenarios
 */

static const char* helpText =
"\n\
Key shortcuts \n\
    \n\
    H (shift h):   This help text \n\
    TAB:           Move camera to next vehicle \n\
    Shift - TAB:   Move camera to previoius vehicle \n\
    Space:         Toggle pause / play \n\
    o:             Toggle show / hide OpenDRIVE road feature lines \n\
    u:             Toggle show / hide OSI road lines \n\
    y:             Toggle show / hide OSI road points \n\
    p:             Toggle show / hide environment 3D model \n\
    i:             Toggle info text showing time and speed \n\
    , (comma):     Switch entity view : Model only / Bounding box / Model + Bounding box / None \n\
    ESC:           quit \n\
    \n\
    Arrow keys \n\
        Left:          Pause and move to previous frame(+Shift to skip 10 frames) \n\
        Right:         Pause and move to next frame(+Shift to skip 10 frames) \n\
        Shift + Left:  Pause and jump 10 frames back \n\
        Shift + Right: Pause and jump 10 frames forward \n\
        Ctrl + Left:   Jump to beginning \n\
        Ctrl + Right:  Jump to end \n\
        Up:            Increase timeScale(play faster) \n\
        Down:          Decrease timeScale(play slower) \n\
    \n\
    1 - 9: Camera models acording to : \n\
        1: Custom camera model \n\
        2: Flight \n\
        3: Drive \n\
        4: Terrain \n\
        5: Orbit \n\
        6: FirstPerson \n\
        7: Spherical \n\
        8: NodeTracker \n\
        9: Trackball \n\
    \n\
    When custom camera model(1) is activated \n\
        k: Switch between the following sub models: \n\
           - Orbit        (camera facing vehicle, rotating around it) \n\
           - Fixed        (fix rotation, always straight behind vehicle) \n\
           - Flex         (imagine the camera attached to vehicle via an elastic string) \n\
           - Flex - orbit (Like flex but allows for roatation around vehicle) \n\
           - Top          (top view, fixed rotation, always straight above vehicle) \n\
    \n\
    Viewer options \n\
        f: Toggle full screen mode \n\
        t: Toggle textures \n\
        s: Rendering statistics \n\
        l: Toggle light \n\
        w: Toggle geometry mode(shading, wireframe, dots) \n\
        c: Save screenshot in JPEG format - in the folder where the application was started from \n\
        h: Help \n\
    \n\
Mouse control \n\
    \n\
    Left:   Rotate \n\
    Right:  Zoom \n\
    Middle: Pan \n\
    \n\
    This is typical, exact behaviour depends on active camera model. \n\
";

