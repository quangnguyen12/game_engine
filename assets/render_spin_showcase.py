import bpy
import os
import math
from mathutils import Vector, Quaternion

blend_path = r"D:\Game_prj\game_engine\assets\models\Target_Vinashoot_Swivel_Fold.blend"
out_dir    = r"D:\Game_prj\game_engine\assets"
os.makedirs(out_dir, exist_ok=True)

bpy.ops.wm.open_mainfile(filepath=blend_path)

scene = bpy.context.scene
scene.render.engine = 'BLENDER_EEVEE'
scene.render.resolution_x = 512
scene.render.resolution_y = 640
scene.render.film_transparent = True

# World lighting
world = scene.world
if not world:
    world = bpy.data.worlds.new("World")
    scene.world = world
world.use_nodes = True
bg = world.node_tree.nodes.get("Background")
if bg:
    bg.inputs[0].default_value = (0.13, 0.14, 0.16, 1.0)
    bg.inputs[1].default_value = 1.3

# Remove existing lights/cameras
for obj in list(scene.objects):
    if obj.type in ('LIGHT', 'CAMERA'):
        bpy.data.objects.remove(obj, do_unlink=True)

def add_light(name, light_type, pos, energy, color=(1,1,1)):
    l_data = bpy.data.lights.new(name, light_type)
    l_data.energy = energy
    l_data.color = color
    l_obj = bpy.data.objects.new(name, l_data)
    l_obj.location = pos
    scene.collection.objects.link(l_obj)
    return l_obj

add_light("KeyLight",   'SUN',   (3.0, -4.0, 5.0), 3.2)
add_light("FillLight",  'SUN',  (-4.0, -2.0, 4.0), 1.6)
add_light("RimLight",   'SUN',   (0.0,  4.0, 3.5), 2.4)
add_light("BottomFill", 'POINT', (0.0, -1.0, 0.3), 60.0)

# Camera setup: Slightly angled 3/4 perspective to see depth & 3D features clearly
cam_data = bpy.data.cameras.new("SpinCam")
cam_obj  = bpy.data.objects.new("SpinCam", cam_data)
scene.collection.objects.link(cam_obj)
scene.camera = cam_obj

cam_data.type = 'PERSP'
cam_data.lens = 48
# Camera positioned in front, slightly elevated to clearly see both base logo badge and rotating target plate
cam_obj.location = Vector((0.0, -2.2, 0.62))

target_center = Vector((0.0, 0.0, 0.56))
dir_vec = target_center - cam_obj.location
cam_obj.rotation_euler = dir_vec.to_track_quat('-Z', 'Y').to_euler()

# Get Armature and Pose Bone
arm_obj = bpy.data.objects.get("Target_Armature_Rig")
if not arm_obj:
    raise RuntimeError("Target_Armature_Rig not found")

# MUTE ALL NLA TRACKS so they do not override manual or action evaluation!
if arm_obj.animation_data:
    for track in arm_obj.animation_data.nla_tracks:
        track.mute = True

# Also ensure no conflicting action is attached
arm_obj.animation_data.action = None

pb_swivel = arm_obj.pose.bones["Bone_Swivel_Y"]
pb_hinge  = arm_obj.pose.bones["Bone_Hinge_X"]
pb_base   = arm_obj.pose.bones["Bone_Base"]

pb_swivel.rotation_mode = 'QUATERNION'
pb_hinge.rotation_mode  = 'QUATERNION'
pb_base.rotation_mode   = 'QUATERNION'

pb_base.rotation_quaternion  = Quaternion((1.0, 0.0, 0.0, 0.0))
pb_hinge.rotation_quaternion = Quaternion((1.0, 0.0, 0.0, 0.0))  # Upright 0 deg

angles = [0, 45, 90, 135, 180, 225, 270, 315]
rendered_files = []

for ang in angles:
    # Rotate around vertical shaft axis (local Y axis of Bone_Swivel_Y)
    rad_s = math.radians(ang / 2.0)
    pb_swivel.rotation_quaternion = Quaternion((math.cos(rad_s), 0.0, math.sin(rad_s), 0.0))
    bpy.context.view_layer.update()
    
    out_file = os.path.join(out_dir, f"spin_frame_{ang:03d}.png")
    scene.render.filepath = out_file
    bpy.ops.render.render(write_still=True)
    rendered_files.append((ang, out_file))
    print(f"Rendered angle {ang} deg -> {out_file}")

print("All spin frames successfully rendered!")
