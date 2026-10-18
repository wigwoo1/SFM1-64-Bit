from pathlib import Path
report=Path('outputs/sfm64-vulkan-posed-session-validation.txt')
with report.open('a') as f:
 f.write('\nScope: saved session model2552 Mamagen, first camera2481, 92 saved local bones, saved model placement, candidate engine-axis camera basis. 66146 triangles,108108 non-background pixels,512x512 readback. PNG visually inspected: posed upper-body/hand view. Native camera projection and rigid inverse have separate numeric reference coverage; full bone pose/view parity is unverified. No scene background, flex deformation, Source lighting, procedural animation, active clip/time camera selection, interactive viewport or full renderer parity. Skinning uses four CPU workers; no performance claim. Installed files unchanged.\n')
