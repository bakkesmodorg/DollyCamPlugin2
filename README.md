# DollyCamPlugin2 (A dollycamera implementation for Rocket League)
Open source implementation of a dolly camera for Rocket League using the BakkesMod SDK. This plugin is included by default with BakkesMod. It allows video creators to set certain waypoints in a replay and the dollycam will automatically interpolate a path between these points.

Requirements:
- BakkesMod (With SDK)
- nlohmann json library (https://github.com/nlohmann/json)

Features:
- Setting camera location/rotation to given values
- Separate interpolation of camera location and rotation
- Preview rendering of the path in 3D space
- Linear interpolation
- Nth order bezier curve interpolation
- Cosine interpolation
- Hermite interpolation (broken)
- Catmull Rom interpolation
- Applying Chaikins algorithm to existing paths
- Saving/loading paths to and from a file.


Full documentation on how to use the plugin can be found here: https://docs.google.com/document/d/18MUmF7qsFZQdKZQOJvlWqzIxgGMyDm58uy9ivAnzFk4/edit
Feel free to add to this. Also, if you implement any features, feel free to submit a pull request!

If you have any questions regarding the BakkesMod SDK/implementation of the plugin send me a PM on Discord (Bakkes#8746) or open an issue on GitHub!
