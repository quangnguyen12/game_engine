import bpy
import os
import math
from mathutils import Vector, Quaternion

blend_path = r"D:\Game_prj\game_engine\assets\models\Target_Vinashoot_Swivel_Fold.blend"
out_dir    = r"D:\Game_prj\game_engine\assets"

bpy.ops.wm.open_mainfile(filepath=blend_path)

scene = bpy.context.scene
scene.render.engine = 'BLENDER_EEVEE'
scene.render.resolution_x = 360
scene.render.resolution_y = 450
scene.render.film_transparent = False

# World background color
world = scene.world
if world and world.node_tree:
    bg = world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs[0].default_value = (0.10, 0.11, 0.13, 1.0)
        bg.inputs[1].default_value = 1.0

# Setup lights and camera
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

cam_data = bpy.data.cameras.new("GifCam")
cam_obj  = bpy.data.objects.new("GifCam", cam_data)
scene.collection.objects.link(cam_obj)
scene.camera = cam_obj

cam_data.type = 'PERSP'
cam_data.lens = 48
cam_obj.location = Vector((0.45, -2.1, 0.65))

target_center = Vector((0.0, 0.0, 0.56))
dir_vec = target_center - cam_obj.location
cam_obj.rotation_euler = dir_vec.to_track_quat('-Z', 'Y').to_euler()

arm_obj = bpy.data.objects.get("Target_Armature_Rig")
if arm_obj and arm_obj.animation_data:
    for t in arm_obj.animation_data.nla_tracks:
        t.mute = True
    arm_obj.animation_data.action = None

pb_swivel = arm_obj.pose.bones["Bone_Swivel_Y"]
pb_hinge  = arm_obj.pose.bones["Bone_Hinge_X"]
pb_base   = arm_obj.pose.bones["Bone_Base"]

pb_swivel.rotation_mode = 'QUATERNION'
pb_hinge.rotation_mode  = 'QUATERNION'
pb_base.rotation_mode   = 'QUATERNION'

pb_base.rotation_quaternion  = Quaternion((1.0, 0.0, 0.0, 0.0))
pb_hinge.rotation_quaternion = Quaternion((1.0, 0.0, 0.0, 0.0))

num_frames = 24

for idx in range(num_frames):
    ang = (idx / num_frames) * 360.0
    rad_s = math.radians(ang / 2.0)
    pb_swivel.rotation_quaternion = Quaternion((math.cos(rad_s), 0.0, math.sin(rad_s), 0.0))
    bpy.context.view_layer.update()
    
    t_file = os.path.join(out_dir, f"_temp_gif_{idx:02d}.png")
    scene.render.filepath = t_file
    bpy.ops.render.render(write_still=True)

print("Rendered 24 frames for GIF!")
