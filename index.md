## Project 4

### Implementation

I started implementing walls, the floor, and the ceiling at the same time. Initially, the floor and ceiling were rendered in by going through for loops for map width and height, and doing glm::translate(model, glm::vec3(i,j, z)) where z was either -1 or 2. I changed this in the end to make the floor and ceiling each just a singular modified cube, and then just modifying the texture scale accordingly. I used that same translate setup for format the whole map, including walls, doors, keys, the user, etc. To achieve this, along with additional object functionality, I made structs for doors, keys, walls, and 'players'. Each of them had the same base elements of an x and y coordinate, which were used to place them on the map using translate. 

The doors and keys both had a texture element (int), an unlocked or picked_up element (bool), and a name element (char). When reading these from the map file, the texture elements were set according to the name (i.e. A or a is always texture 2, B is texture 3, etc), and the bool element was set to false. When going through these objects in drawGeometry() then, I was able to tell whether or not that an object should be rendered, and where. For example, if a door was unlocked, it wasn't rendered anymore. If a key was picked up, the location was changed and continously updated to be "in the user's hand." When the bool values of associated doors and keys were both true, the key that was in the players hand would stop being rendered (they picked up the key and used it on the right door).

Loading .obj files was a little tricky for me. I referred back to HW2 (I think), for parsing a file in c++. I was getting the objects loaded, but they were missing triangles. I tried to account for quads when reading the file, but I kept getting the same problem. I ended up using the suggestion on Slack to just export a file from Wings3D and triangulate it. After doing this, the object files loaded just fine.

### Required Features:
 - continuous movement
 - walls & doors
 - keys
 - user input
 - collision detection
 - floors & ceilings
 - lighting
 - new maps
 - Continuous	Movement

### Additional Features:
 - integrated keyboard and mouse control
 - texture map the walls and floors
 - load existing obj files
 - video

\
&nbsp;
**sample file - textured walls/floor/ceiling - on the right very slight example of collision detection**

![image](https://user-images.githubusercontent.com/59031606/114970187-89fb2d00-9e3f-11eb-8479-0c377d68fc66.png)

\
&nbsp;
**load existing obj files** *(these .obj files are included under 'door' and 'key' folders*

![image1](https://user-images.githubusercontent.com/59031606/114966328-f2dea700-9e37-11eb-8210-24e71baa5579.png)

\
&nbsp;
**walls & doors** *(the right key for the door is the same color)*

![image](https://user-images.githubusercontent.com/59031606/114969707-92069d00-9e3e-11eb-87cb-e7c064bac047.png)

![image](https://user-images.githubusercontent.com/59031606/114969484-16a4eb80-9e3e-11eb-98e0-b3a4988e2f5f.png)

\
&nbsp;
*one wall and five doors, each looks different - using the following map*
```
6 3
WABCDE
000000
00S000
```
![image](https://user-images.githubusercontent.com/59031606/114971244-91bbd100-9e41-11eb-9219-825d81a0bb87.png)


\
&nbsp;
**keys - after picking up the key for the orange door**

![image](https://user-images.githubusercontent.com/59031606/114970902-f0347f80-9e40-11eb-9b85-acc01a59519d.png)


\
&nbsp;
**New Maps**

*used in the "walls & doors" and "keys" pictures*
```
6 6
00G000
000000
000000
AWBWDW
0d0a00
00S00b
```

```
7 7
0000000
CW0W0Wd
0WWWDWW
c00W00G
WWAWWWW
S000a00
0000000
```
