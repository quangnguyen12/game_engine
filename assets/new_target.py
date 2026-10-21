"""
================================================================================
VINASHOOT SWIVELING & POP-UP COMBAT SHOOTING TARGET PIPELINE
Calibrated 1:1 against technical blueprint 'new_target.png' (2048x2048).
Features:
- Accurate metric dimensions: Width 0.457m, Depth 0.350m, Height 1.064m
- Completely simulated 3D target plate: genuine 3D Molle segmented armor blocks,
  3D vertical channels, 3D rear X-ribs, 3D center hub, 3D bolts and clamp knobs
- Pure procedural PBR materials (zero screenshot/texture crop from blueprint)
- 3-Bone Armature rig: Bone_Base -> Bone_Swivel_Y (Yaw) -> Bone_Hinge_X (Pitch)
- 12 Baked Animations on NLA tracks including complete Y-axis rotations
- Multi-format export: .glb, .fbx (embedded textures), .obj, .blend
- Automatic workspace sync across game_engine, dist, and Jme_scene
================================================================================
"""

import bpy
import bmesh
import mathutils
import math
import os
import sys
import shutil
import json

def clear_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    for block in bpy.data.meshes: bpy.data.meshes.remove(block)
    for block in bpy.data.materials: bpy.data.materials.remove(block)
    for block in bpy.data.textures: bpy.data.textures.remove(block)
    for block in bpy.data.images: bpy.data.images.remove(block)
    for block in bpy.data.armatures: bpy.data.armatures.remove(block)
    for block in bpy.data.actions: bpy.data.actions.remove(block)

def create_pbr_material(name, base_color=(0.5, 0.5, 0.5, 1.0), metallic=0.0, roughness=0.5, image_path=None):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    output_node = nodes.new(type='ShaderNodeOutputMaterial')
    output_node.location = (400, 0)
    bsdf_node = nodes.new(type='ShaderNodeBsdfPrincipled')
    bsdf_node.location = (0, 0)

    bsdf_node.inputs['Base Color'].default_value = base_color
    bsdf_node.inputs['Metallic'].default_value = metallic
    bsdf_node.inputs['Roughness'].default_value = roughness

    if image_path and os.path.exists(image_path):
        tex_node = nodes.new(type='ShaderNodeTexImage')
        tex_node.location = (-400, 0)
        img = bpy.data.images.load(image_path, check_existing=True)
        tex_node.image = img
        links.new(tex_node.outputs['Color'], bsdf_node.inputs['Base Color'])

    links.new(bsdf_node.outputs['BSDF'], output_node.inputs['Surface'])
    return mat

def create_box(name, size, loc, mat, bevel_width=0.003, rot_euler=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc, rotation=rot_euler)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel_width > 0:
        bev = obj.modifiers.new(name="Bevel", type='BEVEL')
        bev.width = bevel_width
        bev.segments = 2
        bpy.ops.object.modifier_apply(modifier="Bevel")
    obj.data.materials.append(mat)
    return obj

def create_cylinder(name, radius, depth, loc, rot_euler, mat, vertices=32):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=loc, rotation=rot_euler)
    obj = bpy.context.active_object
    obj.name = name
    obj.data.materials.append(mat)
    return obj

def build_new_target():
    clear_scene()
    scene = bpy.context.scene

    asset_dir = r"D:\Game_prj\game_engine\assets"
    models_dir = r"D:\Game_prj\game_engine\assets\models"
    os.makedirs(models_dir, exist_ok=True)

    tex_badge_p = os.path.join(asset_dir, "tex_vinashoot_badge.png")
    profile_p   = os.path.join(asset_dir, "plate_profile.json")

    # Materials: Authentic simulated PBR tactical materials
    mat_dark_steel     = create_pbr_material("Mat_Dark_Steel", (0.10, 0.10, 0.11, 1.0), metallic=0.75, roughness=0.35)
    mat_silver         = create_pbr_material("Mat_Silver_Hardware", (0.85, 0.85, 0.88, 1.0), metallic=0.95, roughness=0.15)
    mat_rubber         = create_pbr_material("Mat_Rubber_Bumpers", (0.04, 0.04, 0.04, 1.0), metallic=0.0, roughness=0.85)
    mat_accent_red     = create_pbr_material("Mat_Red_Accent", (0.80, 0.10, 0.10, 1.0), metallic=0.2, roughness=0.3)
    mat_badge          = create_pbr_material("Mat_Logo_Badge", (0.85, 0.85, 0.88, 1.0), metallic=0.3, roughness=0.30, image_path=tex_badge_p)
    
    # Tactical Polymer for Target Plate (Simulated 3D materials)
    mat_plate_body     = create_pbr_material("Mat_Tactical_Polymer_Body", (0.16, 0.20, 0.09, 1.0), metallic=0.02, roughness=0.58)
    mat_plate_block    = create_pbr_material("Mat_Tactical_Polymer_Block", (0.22, 0.27, 0.12, 1.0), metallic=0.02, roughness=0.50)
    mat_plate_groove   = create_pbr_material("Mat_Tactical_Polymer_Groove", (0.09, 0.12, 0.05, 1.0), metallic=0.0, roughness=0.65)

    base_objects = []
    swivel_objects = []
    hinge_objects = []

    # ================= 1. BASE UNIT & FIXED CANTILEVER L-ARM (Bone_Base) =================
    # Base Mounting Flange at ground Z = 0.0
    base_flange = create_box("Base_Mounting_Flange", (0.400, 0.350, 0.015), (0.0, 0.0, 0.0075), mat_dark_steel, 0.002)
    base_objects.append(base_flange)

    # 4 Corner mounting bolt pads
    for bx in [-0.175, 0.175]:
        for by in [-0.150, 0.150]:
            bolt = create_cylinder("Base_Bolt", 0.008, 0.020, (bx, by, 0.010), (0, 0, 0), mat_silver, 16)
            base_objects.append(bolt)

    # Base Motor Housing Enclosure
    base_box = create_box("Base_Housing_Enclosure", (0.370, 0.320, 0.110), (0.0, 0.0, 0.070), mat_dark_steel, 0.006)
    base_objects.append(base_box)

    # VINASHOOT Logo Badge on Front Face (-Y)
    mesh_badge = bpy.data.meshes.new("Mesh_Badge")
    bm_b = bmesh.new()
    bw = 0.180
    bh = 0.054
    by_pos = -0.161
    bz_pos = 0.065
    bv1 = bm_b.verts.new((-bw/2.0, by_pos, bz_pos - bh/2.0))
    bv2 = bm_b.verts.new(( bw/2.0, by_pos, bz_pos - bh/2.0))
    bv3 = bm_b.verts.new(( bw/2.0, by_pos, bz_pos + bh/2.0))
    bv4 = bm_b.verts.new((-bw/2.0, by_pos, bz_pos + bh/2.0))
    bf = bm_b.faces.new((bv1, bv2, bv3, bv4))
    buv = bm_b.loops.layers.uv.new("UVMap")
    for loop in bf.loops:
        if loop.vert == bv1: loop[buv].uv = (0.0, 0.0)
        elif loop.vert == bv2: loop[buv].uv = (1.0, 0.0)
        elif loop.vert == bv3: loop[buv].uv = (1.0, 1.0)
        elif loop.vert == bv4: loop[buv].uv = (0.0, 1.0)
    bm_b.to_mesh(mesh_badge)
    bm_b.free()
    obj_badge = bpy.data.objects.new("Base_Logo_Badge", mesh_badge)
    scene.collection.objects.link(obj_badge)
    obj_badge.data.materials.append(mat_badge)
    base_objects.append(obj_badge)

    # Rear Control Switch Panel (+Y)
    sw_box = create_box("Control_Panel_Recess", (0.090, 0.006, 0.045), (-0.080, 0.161, 0.065), mat_dark_steel, 0.002)
    base_objects.append(sw_box)
    sw_rocker = create_box("Switch_Rocker_Red", (0.024, 0.010, 0.016), (-0.065, 0.163, 0.065), mat_accent_red, 0.001)
    base_objects.append(sw_rocker)
    sw_port = create_cylinder("Control_Port", 0.010, 0.008, (-0.105, 0.163, 0.065), (math.pi/2, 0, 0), mat_silver, 16)
    base_objects.append(sw_port)

    # Drive Boss on Left Side Face (-X)
    drive_boss = create_cylinder("Drive_Shaft_Boss", 0.024, 0.035, (-0.185 - 0.0175, 0.0, 0.070), (0, math.pi/2, 0), mat_dark_steel, 32)
    base_objects.append(drive_boss)

    # Cantilever L-Bracket Arm
    l_col = create_box("L_Arm_Vertical_Column", (0.048, 0.055, 0.200), (-0.204, 0.0, 0.125), mat_dark_steel, 0.004)
    base_objects.append(l_col)

    l_beam = create_box("L_Arm_Horizontal_Beam", (0.228, 0.055, 0.048), (-0.090, 0.0, 0.201), mat_dark_steel, 0.004)
    base_objects.append(l_beam)

    bearing_collar = create_cylinder("Swivel_Bearing_Collar", 0.027, 0.052, (0.0, 0.0, 0.199), (0, 0, 0), mat_dark_steel, 32)
    base_objects.append(bearing_collar)

    collar_cap = create_cylinder("Collar_Bottom_Cap", 0.029, 0.008, (0.0, 0.0, 0.171), (0, 0, 0), mat_silver, 32)
    base_objects.append(collar_cap)

    # ================= 2. VERTICAL SWIVEL SHAFT & CLEVIS (Bone_Swivel_Y) =================
    swivel_pivot_z = 0.222
    hinge_pivot_z  = 0.358

    sw_washer = create_cylinder("Swivel_Thrust_Washer", 0.025, 0.006, (0.0, 0.0, 0.228), (0, 0, 0), mat_silver, 32)
    swivel_objects.append(sw_washer)

    sw_shaft = create_cylinder("Swivel_Vertical_Shaft", 0.012, 0.136, (0.0, 0.0, 0.290), (0, 0, 0), mat_silver, 32)
    swivel_objects.append(sw_shaft)

    sw_wing_knob = create_box("Swivel_Tighten_Knob", (0.048, 0.014, 0.014), (0.0, 0.0, 0.265), mat_dark_steel, 0.002)
    sw_wing_knob.rotation_euler = (0, 0, math.pi / 4)
    swivel_objects.append(sw_wing_knob)

    # U-Clevis Bracket
    clevis_base = create_box("Clevis_Base_Block", (0.048, 0.040, 0.028), (0.0, 0.0, 0.342), mat_dark_steel, 0.003)
    swivel_objects.append(clevis_base)

    ear_w = 0.012
    ear_gap = 0.024
    ear_h = 0.040
    ear_d = 0.038
    clevis_ear_l = create_box("Clevis_Ear_Left",  (ear_w, ear_d, ear_h), (-ear_gap/2.0 - ear_w/2.0, 0.0, hinge_pivot_z), mat_dark_steel, 0.002)
    clevis_ear_r = create_box("Clevis_Ear_Right", (ear_w, ear_d, ear_h), ( ear_gap/2.0 + ear_w/2.0, 0.0, hinge_pivot_z), mat_dark_steel, 0.002)
    swivel_objects.append(clevis_ear_l)
    swivel_objects.append(clevis_ear_r)

    # Horizontal Hinge Pin
    hinge_pin = create_cylinder("Hinge_Pin", 0.007, 0.068, (0.0, 0.0, hinge_pivot_z), (0, math.pi/2, 0), mat_silver, 24)
    swivel_objects.append(hinge_pin)

    star_knob = create_cylinder("StarKnob_Hinge", 0.016, 0.016, (0.0, 0.027, hinge_pivot_z), (math.pi/2, 0, 0), mat_dark_steel, 8)
    swivel_objects.append(star_knob)

    # ================= 3. SIMULATED 3D TARGET PLATE (Bone_Hinge_X) =================
    # Read 132-point calibrated silhouette contour
    if not os.path.exists(profile_p):
        raise FileNotFoundError(f"Missing plate profile contour: {profile_p}")
    with open(profile_p, "r") as f:
        loop_pts = json.load(f)

    half_th = 0.012  # 24mm plate base thickness
    t_height = 0.700

    # 3A. Extruded Solid Silhouette Plate Base
    mesh_target = bpy.data.meshes.new("Mesh_Plate_Base")
    bm_t = bmesh.new()
    front_verts = []
    rear_verts  = []
    for x, z in loop_pts:
        vf = bm_t.verts.new((x, -half_th, z + hinge_pivot_z))
        vr = bm_t.verts.new((x,  half_th, z + hinge_pivot_z))
        front_verts.append(vf)
        rear_verts.append(vr)

    bm_t.verts.ensure_lookup_table()
    num_pts = len(loop_pts)
    for i in range(num_pts):
        next_i = (i + 1) % num_pts
        bm_t.faces.new([front_verts[i], front_verts[next_i], rear_verts[next_i], rear_verts[i]])

    try: f_front = bm_t.faces.new(reversed(front_verts))
    except: f_front = bm_t.faces.new(front_verts)

    try: f_rear = bm_t.faces.new(rear_verts)
    except: f_rear = bm_t.faces.new(reversed(rear_verts))

    bmesh.ops.triangulate(bm_t, faces=bm_t.faces)
    bmesh.ops.recalc_face_normals(bm_t, faces=bm_t.faces)
    bm_t.to_mesh(mesh_target)
    bm_t.free()

    obj_target = bpy.data.objects.new("Target_Base_Plate", mesh_target)
    scene.collection.objects.link(obj_target)
    obj_target.data.materials.append(mat_plate_body)
    hinge_objects.append(obj_target)

    # 3B. FRONT 3D GEOMETRY: Molle Segmented Blocks & Fluting (Pure 3D Procedural)
    block_d = 0.006  # 6mm forward projection
    front_y = -half_th - block_d / 2.0
    block_h = 0.042

    # 7 rows of 3D Molle blocks on chest/abdomen (Z from 0.065 to 0.380 above hinge)
    for r_idx in range(7):
        rz = hinge_pivot_z + 0.065 + r_idx * 0.052
        # Center block (width 80mm)
        c_block = create_box(f"Molle_C_{r_idx}", (0.080, block_d, block_h), (0.0, front_y, rz), mat_plate_block, bevel_width=0.002)
        hinge_objects.append(c_block)
        # Left and Right side blocks (width 65mm, staggered)
        w_side = 0.065 if (r_idx % 2 == 0) else 0.058
        x_off  = 0.078 if (r_idx % 2 == 0) else 0.074
        l_block = create_box(f"Molle_L_{r_idx}", (w_side, block_d, block_h), (-x_off, front_y, rz), mat_plate_block, bevel_width=0.002)
        r_block = create_box(f"Molle_R_{r_idx}", (w_side, block_d, block_h), ( x_off, front_y, rz), mat_plate_block, bevel_width=0.002)
        hinge_objects.append(l_block)
        hinge_objects.append(r_block)

    # 3 Vertical 3D blocks in neck and head
    for h_idx, hz_rel in enumerate([0.465, 0.535, 0.605]):
        hz = hinge_pivot_z + hz_rel
        h_block = create_box(f"Molle_Head_{h_idx}", (0.052, block_d, 0.046), (0.0, front_y, hz), mat_plate_block, bevel_width=0.002)
        hinge_objects.append(h_block)

    # Left & Right 3D vertical outer flutes/ribs flanking the Molle grid
    rib_h = 0.380
    rib_z = hinge_pivot_z + 0.055 + rib_h / 2.0
    l_rib = create_box("Front_Flute_L", (0.016, block_d * 0.8, rib_h), (-0.128, front_y, rib_z), mat_plate_groove, bevel_width=0.002)
    r_rib = create_box("Front_Flute_R", (0.016, block_d * 0.8, rib_h), ( 0.128, front_y, rib_z), mat_plate_groove, bevel_width=0.002)
    hinge_objects.append(l_rib)
    hinge_objects.append(r_rib)

    # Bottom 3D Diamond Boss on front
    t_diamond = create_box("Target_Front_Diamond_Boss", (0.034, 0.008, 0.034), (0.0, -half_th - 0.004, hinge_pivot_z + 0.024), mat_plate_body, bevel_width=0.002)
    t_diamond.rotation_euler = (0, math.pi / 4, 0)
    hinge_objects.append(t_diamond)

    # Dual 3D Rubber Impact Bumpers
    for bx in [-0.140, 0.140]:
        bumper = create_box(f"Target_Bumper_{bx}", (0.028, 0.016, 0.020), (bx, -half_th - 0.008, hinge_pivot_z + 0.014), mat_rubber, bevel_width=0.002)
        hinge_objects.append(bumper)

    # Dual 3D Side Clamping Knobs at outer edges
    for sx in [-0.165, 0.165]:
        sknob = create_cylinder(f"Target_Side_Knob_{sx}", 0.010, 0.024, (sx, 0.0, hinge_pivot_z + 0.020), (0, math.pi/2, 0), mat_dark_steel, 16)
        hinge_objects.append(sknob)

    # 3C. REAR 3D GEOMETRY: Molded Structural X-Ribs, Hub & Stiffeners (Pure 3D Procedural)
    rear_d = 0.006  # 6mm rearward projection
    rear_y = half_th + rear_d / 2.0
    hub_z = hinge_pivot_z + 0.245

    # Center circular hub boss with concentric recess
    rear_hub_outer = create_cylinder("Rear_Center_Hub_Outer", 0.024, rear_d, (0.0, rear_y, hub_z), (math.pi/2, 0, 0), mat_plate_block, 32)
    rear_hub_inner = create_cylinder("Rear_Center_Hub_Inner", 0.014, rear_d + 0.002, (0.0, rear_y, hub_z), (math.pi/2, 0, 0), mat_plate_groove, 32)
    hinge_objects.append(rear_hub_outer)
    hinge_objects.append(rear_hub_inner)

    # Molded X-Ribs (4 radiating diagonal legs meeting at center hub)
    # Upper-left and upper-right legs (angle ~65 deg)
    x_rot_upper = math.atan2(0.195, 0.090)  # steep angle up to shoulders
    len_upper = math.sqrt(0.090**2 + 0.195**2)
    rib_ul = create_box("Rear_X_Rib_UL", (0.016, rear_d, len_upper), (-0.045, rear_y, hub_z + 0.0975), mat_plate_block, bevel_width=0.002, rot_euler=(0, -math.radians(25), 0))
    rib_ur = create_box("Rear_X_Rib_UR", (0.016, rear_d, len_upper), ( 0.045, rear_y, hub_z + 0.0975), mat_plate_block, bevel_width=0.002, rot_euler=(0,  math.radians(25), 0))
    hinge_objects.append(rib_ul)
    hinge_objects.append(rib_ur)

    # Lower-left and lower-right legs (angle ~58 deg down to bottom corners)
    len_lower = math.sqrt(0.120**2 + 0.195**2)
    rib_ll = create_box("Rear_X_Rib_LL", (0.016, rear_d, len_lower), (-0.060, rear_y, hub_z - 0.0975), mat_plate_block, bevel_width=0.002, rot_euler=(0,  math.radians(31), 0))
    rib_lr = create_box("Rear_X_Rib_LR", (0.016, rear_d, len_lower), ( 0.060, rear_y, hub_z - 0.0975), mat_plate_block, bevel_width=0.002, rot_euler=(0, -math.radians(31), 0))
    hinge_objects.append(rib_ll)
    hinge_objects.append(rib_lr)

    # Vertical Spine Rib rising into head
    spine_rib = create_box("Rear_Spine_Rib", (0.014, rear_d, 0.380), (0.0, rear_y, hub_z + 0.200), mat_plate_block, bevel_width=0.002)
    hinge_objects.append(spine_rib)

    # Vertical Outer Stiffener Ribs on rear
    outer_rib_l = create_box("Rear_Outer_Rib_L", (0.014, rear_d, 0.380), (-0.128, rear_y, hub_z), mat_plate_block, bevel_width=0.002)
    outer_rib_r = create_box("Rear_Outer_Rib_R", (0.014, rear_d, 0.380), ( 0.128, rear_y, hub_z), mat_plate_block, bevel_width=0.002)
    hinge_objects.append(outer_rib_l)
    hinge_objects.append(outer_rib_r)

    # Rear Mounting Bracket Tongue (steel plate fixed to target plate)
    t_tongue = create_box("Target_Mount_Tongue", (0.090, 0.012, 0.110), (0.0, half_th + 0.006, hinge_pivot_z + 0.055), mat_dark_steel, 0.002)
    hinge_objects.append(t_tongue)

    # 4 Mounting Hex Bolts & Washers on rear tongue
    for bx in [-0.030, 0.030]:
        for bz in [hinge_pivot_z + 0.025, hinge_pivot_z + 0.085]:
            bolt = create_cylinder("Target_Rear_Bolt", 0.007, 0.018, (bx, half_th + 0.012, bz), (math.pi/2, 0, 0), mat_silver, 6)
            washer = create_cylinder("Target_Rear_Washer", 0.011, 0.003, (bx, half_th + 0.012, bz), (math.pi/2, 0, 0), mat_dark_steel, 16)
            hinge_objects.append(bolt)
            hinge_objects.append(washer)

    # ================= 4. UNIFY MESH & ASSIGN RIGID VERTEX GROUPS =================
    all_objects = base_objects + swivel_objects + hinge_objects

    bpy.ops.object.select_all(action='DESELECT')
    for o in all_objects:
        o.select_set(True)
    scene.view_layers[0].objects.active = base_flange

    bpy.ops.object.make_single_user(object=True, obdata=True)
    bpy.ops.object.convert(target='MESH')

    # Ensure single consistent UVMap
    for o in all_objects:
        if o.type == 'MESH':
            while len(o.data.uv_layers) > 1:
                o.data.uv_layers.remove(o.data.uv_layers[-1])
            if len(o.data.uv_layers) == 1:
                o.data.uv_layers[0].name = "UVMap"

    # Assign rigid vertex groups to each component BEFORE joining (prevents vertex index shift)
    for o in base_objects:
        vg = o.vertex_groups.new(name="Bone_Base")
        vg.add(list(range(len(o.data.vertices))), 1.0, 'REPLACE')

    for o in swivel_objects:
        vg = o.vertex_groups.new(name="Bone_Swivel_Y")
        vg.add(list(range(len(o.data.vertices))), 1.0, 'REPLACE')

    for o in hinge_objects:
        vg = o.vertex_groups.new(name="Bone_Hinge_X")
        vg.add(list(range(len(o.data.vertices))), 1.0, 'REPLACE')

    # Join into single unified mesh
    bpy.ops.object.select_all(action='DESELECT')
    for o in all_objects:
        o.select_set(True)
    scene.view_layers[0].objects.active = base_flange
    bpy.ops.object.join()

    final_model = bpy.context.active_object
    final_model.name = "Target_Vinashoot_Swivel_Fold"

    # Ensure pivot grounded at Y=0.0 (glTF ground level)
    min_z = min(v.co.z for v in final_model.data.vertices)
    if abs(min_z) > 1e-4:
        for v in final_model.data.vertices:
            v.co.z -= min_z
        swivel_pivot_z -= min_z
        hinge_pivot_z  -= min_z

    xs = [v.co.x for v in final_model.data.vertices]
    ys = [v.co.y for v in final_model.data.vertices]
    zs = [v.co.z for v in final_model.data.vertices]
    print(f"Final Model Dimensions: Width={max(xs)-min(xs):.3f}m, Depth={max(ys)-min(ys):.3f}m, Height={max(zs)-min(zs):.3f}m")

    # ================= 5. CREATE ARMATURE SKELETON =================
    arm_data = bpy.data.armatures.new("Target_Armature")
    arm_obj  = bpy.data.objects.new("Target_Armature_Rig", arm_data)
    scene.collection.objects.link(arm_obj)
    scene.view_layers[0].objects.active = arm_obj

    top_z_final = max(zs)
    swivel_pivot_z_final = swivel_pivot_z
    hinge_pivot_z_final  = hinge_pivot_z

    bpy.ops.object.mode_set(mode='EDIT')
    bone_base = arm_data.edit_bones.new("Bone_Base")
    bone_base.head = (0.0, 0.0, 0.0)
    bone_base.tail = (0.0, 0.0, swivel_pivot_z_final)

    # Bone_Swivel_Y: Rotates around vertical axis (Yaw / Y-axis in Game Engine)
    bone_swivel = arm_data.edit_bones.new("Bone_Swivel_Y")
    bone_swivel.head = (0.0, 0.0, swivel_pivot_z_final)
    bone_swivel.tail = (0.0, 0.0, hinge_pivot_z_final)
    bone_swivel.parent = bone_base
    bone_swivel.use_connect = True

    # Bone_Hinge_X: Tilts backward around horizontal axis (Pitch / X-axis in Game Engine)
    bone_hinge = arm_data.edit_bones.new("Bone_Hinge_X")
    bone_hinge.head = (0.0, 0.0, hinge_pivot_z_final)
    bone_hinge.tail = (0.0, 0.0, top_z_final)
    bone_hinge.parent = bone_swivel
    bone_hinge.use_connect = False
    bpy.ops.object.mode_set(mode='OBJECT')

    # Bind Mesh to Armature
    mod = final_model.modifiers.new(name="Armature", type='ARMATURE')
    mod.object = arm_obj
    mod.use_vertex_groups = True
    final_model.parent = arm_obj

    # ================= 6. BAKE ACTIONS & NLA ANIMATION TRACKS =================
    if not arm_obj.animation_data:
        arm_obj.animation_data_create()

    pb_base   = arm_obj.pose.bones["Bone_Base"]
    pb_swivel = arm_obj.pose.bones["Bone_Swivel_Y"]
    pb_hinge  = arm_obj.pose.bones["Bone_Hinge_X"]

    pb_base.rotation_mode   = 'QUATERNION'
    pb_swivel.rotation_mode = 'QUATERNION'
    pb_hinge.rotation_mode  = 'QUATERNION'

    def make_action(act_name, keyframes_data, linear=False):
        act = bpy.data.actions.get(act_name)
        if act: bpy.data.actions.remove(act)
        act = bpy.data.actions.new(name=act_name)
        act.use_fake_user = True
        arm_obj.animation_data.action = act
        if hasattr(arm_obj.animation_data, "action_suitable_slots") and arm_obj.animation_data.action_suitable_slots:
            arm_obj.animation_data.action_slot = arm_obj.animation_data.action_suitable_slots[0]

        for item in keyframes_data:
            frame = item[0]
            swivel_deg = item[1]  # Rotation around vertical axis (Yaw / Y-axis in engine)
            hinge_deg  = item[2]  # Rotation around horizontal X-axis (Pitch, 0=upright, 90=fallen backward)

            # Swivel around vertical axis (local Y roll axis of bone in Blender armature):
            rad_s = math.radians(swivel_deg / 2.0)
            pb_swivel.rotation_quaternion = mathutils.Quaternion((math.cos(rad_s), 0.0, math.sin(rad_s), 0.0))
            pb_swivel.keyframe_insert(data_path="rotation_quaternion", frame=frame)

            # Hinge around horizontal X (backward fold = negative X angle in Blender convention):
            rad_h = math.radians(-hinge_deg / 2.0)
            pb_hinge.rotation_quaternion = mathutils.Quaternion((math.cos(rad_h), math.sin(rad_h), 0.0, 0.0))
            pb_hinge.keyframe_insert(data_path="rotation_quaternion", frame=frame)

            # Base stays fixed
            pb_base.rotation_quaternion = mathutils.Quaternion((1.0, 0.0, 0.0, 0.0))
            pb_base.keyframe_insert(data_path="rotation_quaternion", frame=frame)

        if linear:
            if hasattr(act, "fcurves"):
                for fc in act.fcurves:
                    for kp in fc.keyframe_points:
                        kp.interpolation = 'LINEAR'
            elif hasattr(act, "layers"):
                for l in act.layers:
                    for s in l.strips:
                        for cb in s.channelbags:
                            for fc in cb.fcurves:
                                for kp in fc.keyframe_points:
                                    kp.interpolation = 'LINEAR'

        return act

    # ---- 1. Y-AXIS ROTATION ANIMATIONS (SWIVEL / TURNING AROUND VERTICAL Y) ----
    # 1. Target_Rotate_Y_90: Swivel 90° edge-on to conceal target from shooter
    act_rot_y_90 = make_action("Target_Rotate_Y_90", [
        (1,   0.0, 0.0),
        (5,   3.0, 0.0),
        (10, 25.0, 0.0),
        (18, 72.0, 0.0),
        (23, 88.5, 0.0),
        (25, 90.0, 0.0)
    ])

    # 2. Target_Rotate_Y_Front: Swivel from edge-on 90° back to face-on 0°
    act_rot_y_front = make_action("Target_Rotate_Y_Front", [
        (1,  90.0, 0.0),
        (5,  87.0, 0.0),
        (10, 65.0, 0.0),
        (18, 18.0, 0.0),
        (23,  1.5, 0.0),
        (25,  0.0, 0.0)
    ])

    # 3. Target_Rotate_Y_180: Swivel 180° around Y (flip front to back / friend-to-foe target switch)
    act_rot_y_180 = make_action("Target_Rotate_Y_180", [
        (1,    0.0, 0.0),
        (6,    6.0, 0.0),
        (14,  45.0, 0.0),
        (22, 135.0, 0.0),
        (30, 175.0, 0.0),
        (35, 180.0, 0.0)
    ])

    # 4. Target_Rotate_Y_360: Full 360° continuous rotation around Y shaft (dense 30° steps, constant speed)
    act_rot_y_360 = make_action("Target_Rotate_Y_360", [
        (1,    0.0, 0.0),
        (6,   30.0, 0.0),
        (11,  60.0, 0.0),
        (16,  90.0, 0.0),
        (21, 120.0, 0.0),
        (26, 150.0, 0.0),
        (31, 180.0, 0.0),
        (36, 210.0, 0.0),
        (41, 240.0, 0.0),
        (46, 270.0, 0.0),
        (51, 300.0, 0.0),
        (56, 330.0, 0.0),
        (61, 360.0, 0.0)
    ], linear=True)

    # 4B. Target_Rotate_Y_360_CW: Dedicated Clockwise 360° circular rotation
    act_rot_y_360_cw = make_action("Target_Rotate_Y_360_CW", [
        (1,    0.0, 0.0),
        (6,   30.0, 0.0),
        (11,  60.0, 0.0),
        (16,  90.0, 0.0),
        (21, 120.0, 0.0),
        (26, 150.0, 0.0),
        (31, 180.0, 0.0),
        (36, 210.0, 0.0),
        (41, 240.0, 0.0),
        (46, 270.0, 0.0),
        (51, 300.0, 0.0),
        (56, 330.0, 0.0),
        (61, 360.0, 0.0)
    ], linear=True)

    # 4C. Target_Rotate_Y_360_CCW: Dedicated Counter-Clockwise 360° circular rotation
    act_rot_y_360_ccw = make_action("Target_Rotate_Y_360_CCW", [
        (1,      0.0, 0.0),
        (6,    -30.0, 0.0),
        (11,   -60.0, 0.0),
        (16,   -90.0, 0.0),
        (21,  -120.0, 0.0),
        (26,  -150.0, 0.0),
        (31,  -180.0, 0.0),
        (36,  -210.0, 0.0),
        (41,  -240.0, 0.0),
        (46,  -270.0, 0.0),
        (51,  -300.0, 0.0),
        (56,  -330.0, 0.0),
        (61,  -360.0, 0.0)
    ], linear=True)

    # 4D. Target_Spin_Continuous_Loop: 120-frame seamless continuous spin at constant speed (3 deg/frame)
    act_spin_loop = make_action("Target_Spin_Continuous_Loop", [
        (1,     0.0, 0.0),
        (11,   30.0, 0.0),
        (21,   60.0, 0.0),
        (31,   90.0, 0.0),
        (41,  120.0, 0.0),
        (51,  150.0, 0.0),
        (61,  180.0, 0.0),
        (71,  210.0, 0.0),
        (81,  240.0, 0.0),
        (91,  270.0, 0.0),
        (101, 300.0, 0.0),
        (111, 330.0, 0.0),
        (121, 360.0, 0.0)
    ], linear=True)

    # 4E. Target_Spin_Fast: Fast 30-frame 360° spin (12 deg/frame)
    act_spin_fast = make_action("Target_Spin_Fast", [
        (1,     0.0, 0.0),
        (4,    36.0, 0.0),
        (7,    72.0, 0.0),
        (10,  108.0, 0.0),
        (13,  144.0, 0.0),
        (16,  180.0, 0.0),
        (19,  216.0, 0.0),
        (22,  252.0, 0.0),
        (25,  288.0, 0.0),
        (28,  324.0, 0.0),
        (31,  360.0, 0.0)
    ], linear=True)

    # 5. Target_Swivel_Y_Cycle: Complete Y swivel concealment cycle (Face -> Edge 90° -> Wait -> Face)
    act_swivel_y_cycle = make_action("Target_Swivel_Y_Cycle", [
        (1,   0.0, 0.0),
        (15,  0.0, 0.0),
        (20,  5.0, 0.0),
        (26, 35.0, 0.0),
        (32, 75.0, 0.0),
        (36, 90.0, 0.0),
        (65, 90.0, 0.0),
        (70, 82.0, 0.0),
        (76, 45.0, 0.0),
        (82, 10.0, 0.0),
        (87, -0.8, 0.0),
        (90,  0.0, 0.0)
    ])

    # ---- 2. X-AXIS PITCH FOLD / RISE ANIMATIONS ----
    # 6. Target_Fall: Rapid backward physical fall with bumper rebound (90° Pitch)
    act_fall = make_action("Target_Fall", [
        (1,  0.0,  0.0),
        (3,  0.0,  0.5),
        (7,  0.0,  5.0),
        (12, 0.0, 22.0),
        (17, 0.0, 60.0),
        (21, 0.0, 88.0),
        (24, 0.0, 90.0),
        (27, 0.0, 87.2),
        (30, 0.0, 90.0)
    ])

    # 7. Target_Rise: Smooth motorized reset upright (Pitch back to 0°)
    act_rise = make_action("Target_Rise", [
        (1,  0.0, 90.0),
        (5,  0.0, 88.5),
        (12, 0.0, 62.0),
        (19, 0.0, 28.0),
        (25, 0.0,  6.0),
        (28, 0.0, -0.8),
        (30, 0.0,  0.0)
    ])

    # 8. Target_Hit_Cycle: Standing -> Hit & Fall 90° -> Downed -> Motorized Rise
    act_hit_cycle = make_action("Target_Hit_Cycle", [
        (1,  0.0,  0.0),
        (15, 0.0,  0.0),
        (18, 0.0,  2.0),
        (23, 0.0, 25.0),
        (29, 0.0, 78.0),
        (33, 0.0, 90.0),
        (36, 0.0, 87.0),
        (40, 0.0, 90.0),
        (65, 0.0, 90.0),
        (72, 0.0, 55.0),
        (80, 0.0, 16.0),
        (86, 0.0, -0.8),
        (90, 0.0,  0.0)
    ])

    # ---- 3. COMBINED DUAL-AXIS (Y-SWIVEL + X-PITCH) ANIMATIONS ----
    # 9. Target_Hit_Turn_Fall: High-impact realism: bullet hit triggers 35° Y-yaw deflection + 90° X-pitch drop
    act_hit_turn_fall = make_action("Target_Hit_Turn_Fall", [
        (1,   0.0,  0.0),
        (4,   8.0,  5.0),
        (10, 28.0, 28.0),
        (16, 35.0, 68.0),
        (22, 33.0, 88.0),
        (26, 30.0, 90.0),
        (35, 15.0, 90.0),
        (45,  0.0, 90.0)
    ])

    # ---- 4. STATIC IDLE POSES ----
    # 10. Target_Idle_Up: Static upright standing ready (0° X, 0° Y)
    act_idle_up = make_action("Target_Idle_Up", [(1, 0.0, 0.0), (10, 0.0, 0.0)])

    # 11. Target_Idle_Down: Static fallen horizontal pose (90° X, 0° Y)
    act_idle_down = make_action("Target_Idle_Down", [(1, 0.0, 90.0), (10, 0.0, 90.0)])

    # 12. Target_Idle_Edge: Static edge-on concealed pose (0° X, 90° Y)
    act_idle_edge = make_action("Target_Idle_Edge", [(1, 90.0, 0.0), (10, 90.0, 0.0)])

    # Setup NLA tracks for ALL actions
    while arm_obj.animation_data.nla_tracks:
        arm_obj.animation_data.nla_tracks.remove(arm_obj.animation_data.nla_tracks[0])

    all_actions = [
        ("Target_Rotate_Y_90",          act_rot_y_90),
        ("Target_Rotate_Y_Front",       act_rot_y_front),
        ("Target_Rotate_Y_180",         act_rot_y_180),
        ("Target_Rotate_Y_360",         act_rot_y_360),
        ("Target_Rotate_Y_360_CW",      act_rot_y_360_cw),
        ("Target_Rotate_Y_360_CCW",     act_rot_y_360_ccw),
        ("Target_Spin_Continuous_Loop", act_spin_loop),
        ("Target_Spin_Fast",            act_spin_fast),
        ("Target_Swivel_Y_Cycle",       act_swivel_y_cycle),
        ("Target_Fall",                 act_fall),
        ("Target_Rise",                 act_rise),
        ("Target_Hit_Cycle",            act_hit_cycle),
        ("Target_Hit_Turn_Fall",        act_hit_turn_fall),
        ("Target_Idle_Up",              act_idle_up),
        ("Target_Idle_Down",            act_idle_down),
        ("Target_Idle_Edge",            act_idle_edge)
    ]

    for track_name, act in all_actions:
        track = arm_obj.animation_data.nla_tracks.new()
        track.name = track_name
        track.mute = False
        strip = track.strips.new(track_name, int(act.frame_range[0]), act)
        if hasattr(strip, "action_suitable_slots") and strip.action_suitable_slots:
            strip.action_slot = strip.action_suitable_slots[0]

    # Clear active action so glTF exporter bakes all NLA tracks
    arm_obj.animation_data.action = None
    scene.frame_set(1)

    # ================= 7. PACK TEXTURES & EXPORT ASSETS =================
    bpy.ops.file.pack_all()

    model_base_name = "Target_Vinashoot_Swivel_Fold"
    blend_path = os.path.join(models_dir, f"{model_base_name}.blend")
    glb_path   = os.path.join(models_dir, f"{model_base_name}.glb")
    fbx_path   = os.path.join(models_dir, f"{model_base_name}.fbx")
    obj_path   = os.path.join(models_dir, f"{model_base_name}.obj")

    # Select both Armature and Mesh for export
    bpy.ops.object.select_all(action='DESELECT')
    arm_obj.select_set(True)
    final_model.select_set(True)
    scene.view_layers[0].objects.active = arm_obj

    # Export glTF / GLB (Y-up convention, with all skeletal NLA strips)
    bpy.ops.export_scene.gltf(
        filepath=glb_path,
        export_format='GLB',
        use_selection=True,
        export_yup=True,
        export_apply=False,
        export_animations=True,
        export_nla_strips=True,
        export_anim_single_armature=True
    )
    print(f"Exported animated .glb with {len(all_actions)} actions: {glb_path}")

    # Export FBX with all animations, 1:1 metric scale, and embedded textures
    arm_obj.hide_set(False)
    final_model.hide_set(False)
    bpy.ops.object.select_all(action='DESELECT')
    arm_obj.select_set(True)
    final_model.select_set(True)
    scene.view_layers[0].objects.active = arm_obj

    bpy.ops.export_scene.fbx(
        filepath=fbx_path,
        use_selection=True,
        axis_forward='-Z',
        axis_up='Y',
        bake_anim=True,
        bake_anim_use_nla_strips=True,
        bake_anim_step=1.0,
        add_leaf_bones=False,
        apply_scale_options='FBX_SCALE_ALL',
        path_mode='COPY',
        embed_textures=True
    )
    print(f"Exported animated .fbx: {fbx_path}")

    # Export OBJ + MTL for raw mesh fallback
    bpy.ops.wm.obj_export(
        filepath=obj_path,
        export_selected_objects=True,
        apply_modifiers=True,
        export_materials=True,
        path_mode='STRIP'
    )
    print(f"Exported .obj: {obj_path}")

    # Restore default action for Blender UI inspection and save .blend
    arm_obj.animation_data.action = act_fall
    bia_blend_path = os.path.join(r"D:\Game_prj\blender\bia", f"{model_base_name}.blend")
    os.makedirs(os.path.dirname(bia_blend_path), exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=bia_blend_path)
    shutil.copy2(bia_blend_path, blend_path)
    print(f"Saved .blend: {blend_path}")

    # ================= 8. MIRROR ASSETS ACROSS WORKSPACES =================
    mirror_dirs = [
        r"D:\Game_prj\game_engine\assets\models",
        r"D:\Game_prj\game_engine\dist\UnityHub3D_Vulkan\assets\models",
        r"D:\Game_prj\Jme_scene\assets\models"
    ]

    export_files = [
        f"{model_base_name}.blend",
        f"{model_base_name}.glb",
        f"{model_base_name}.fbx",
        f"{model_base_name}.obj",
        f"{model_base_name}.mtl"
    ]

    fbm_dir_src = os.path.join(models_dir, f"{model_base_name}.fbm")

    for m_dir in mirror_dirs:
        os.makedirs(m_dir, exist_ok=True)
        for ef in export_files:
            src_f = os.path.join(models_dir, ef)
            dst_f = os.path.join(m_dir, ef)
            if os.path.exists(src_f) and os.path.abspath(src_f) != os.path.abspath(dst_f):
                shutil.copy2(src_f, dst_f)
        if os.path.isdir(fbm_dir_src):
            dst_fbm = os.path.join(m_dir, f"{model_base_name}.fbm")
            if os.path.abspath(fbm_dir_src) != os.path.abspath(dst_fbm):
                shutil.copytree(fbm_dir_src, dst_fbm, dirs_exist_ok=True)

    print("Successfully mirrored Target_Vinashoot_Swivel_Fold across game_engine, dist, and Jme_scene!")

if __name__ == "__main__":
    build_new_target()
