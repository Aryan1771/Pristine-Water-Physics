"""Run in UE's Python console or with -run=pythonscript -script=<this file>.
Creates only missing demo assets. The demo is a rendering test scene.
"""
import math
import unreal

ROOT = '/PristineWaterPhysics/Demo'
assets = unreal.EditorAssetLibrary
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

def material(name, color):
    path = ROOT + '/' + name
    if assets.does_asset_exist(path):
        return assets.load_asset(path)
    m = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    color_node = unreal.MaterialEditingLibrary.create_material_expression(m, unreal.MaterialExpressionConstant3Vector)
    color_node.set_editor_property('constant', unreal.LinearColor(*color, 1))
    unreal.MaterialEditingLibrary.connect_material_property(color_node, '', unreal.MaterialProperty.MP_BASE_COLOR)
    rough = unreal.MaterialEditingLibrary.create_material_expression(m, unreal.MaterialExpressionConstant)
    rough.set_editor_property('r', 0.85)
    unreal.MaterialEditingLibrary.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(m)
    assets.save_loaded_asset(m)
    return m

def mesh(label, asset, position, scale, mat):
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*position))
    a.set_actor_label(label)
    a.static_mesh_component.set_static_mesh(unreal.load_asset(asset))
    a.static_mesh_component.set_material(0, mat)
    a.set_actor_scale3d(unreal.Vector(*scale))
    return a

if assets.does_asset_exist(ROOT + '/L_PristineWaterDemo'):
    unreal.log('Demo exists; preserved.')
else:
    levels.new_level(ROOT + '/L_PristineWaterDemo')
    stone = material('M_DemoStone', (0.16, 0.19, 0.20))
    bed = material('M_DemoBed', (0.23, 0.25, 0.17))
    mesh('Lake bed', '/Engine/BasicShapes/Cube', (0,0,-240), (100,100,2), bed)
    source = actors.spawn_actor_from_class(unreal.PristineWaterSource, unreal.Vector(0,0,0))
    source.set_actor_label('Pristine Lake - change Quality and Water Settings')
    source.set_body_type(unreal.PristineWaterBody.LAKE)
    source.set_editor_property('half_extent', unreal.Vector2D(3200,2600))
    source.set_quality(unreal.PristineWaterQuality.HIGH)
    water = source.get_editor_property('water')
    water.set_liquid_colors(unreal.LinearColor(0.11,0.45,0.35,1), unreal.LinearColor(0.012,0.10,0.13,1))
    water.set_wave_amplitude(5)
    water.set_wave_frequency(0.8)
    source.apply_appearance()
    for i in range(34):
        angle = i * math.tau / 34
        radius = 1 + 0.03 * math.sin(i*3.4)
        position = (3150*radius*math.cos(angle),2550*radius*math.sin(angle),-40)
        scale = (7+2*math.sin(i*1.9),6+math.cos(i*2.1),3+math.sin(i))
        rock = mesh('Shore stone %02d' % i, '/Engine/BasicShapes/Sphere', position, scale, stone)
        rock.set_actor_rotation(unreal.Rotator(7*math.sin(i),i*31,8*math.cos(i)),False)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,4000),unreal.Rotator(-32,-40,0))
    sun.light_component.set_editor_property('intensity', 6.0)
    sun.light_component.set_editor_property('atmosphere_sun_light', True)
    sky = actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,1500))
    sky.light_component.set_editor_property('real_time_capture', True)
    sky.light_component.set_editor_property('intensity', 1.2)
    location = unreal.Vector(4300,-4800,2600)
    rotation = unreal.MathLibrary.find_look_at_rotation(location,unreal.Vector(0,0,-70))
    camera = actors.spawn_actor_from_class(unreal.CameraActor,location,rotation)
    camera.set_editor_property('auto_activate_for_player',unreal.AutoReceiveInput.PLAYER0)
    camera.camera_component.set_editor_property('field_of_view',65)
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(location,rotation)
    levels.save_current_level()
    unreal.log('PRISTINE_DEMO_CREATED')
