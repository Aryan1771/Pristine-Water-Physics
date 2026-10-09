"""UE 5.7 Python: creates a contained pool and small-volume test scene. Preserves existing level."""
import unreal

ROOT='/PristineWaterPhysics/Demo'
PATH=ROOT+'/L_PristineFluidLab'
assets=unreal.EditorAssetLibrary
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

def mat(name,color,roughness):
    path=ROOT+'/'+name
    if assets.does_asset_exist(path): return assets.load_asset(path)
    m=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,ROOT,unreal.Material,unreal.MaterialFactoryNew())
    c=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant3Vector)
    c.set_editor_property('constant',unreal.LinearColor(*color,1))
    unreal.MaterialEditingLibrary.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
    r=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant)
    r.set_editor_property('r',roughness)
    unreal.MaterialEditingLibrary.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(m)
    assets.save_loaded_asset(m)
    return m

def box(name,p,s,m):
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*p))
    a.set_actor_label(name)
    a.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    a.static_mesh_component.set_material(0,m)
    a.set_actor_scale3d(unreal.Vector(*s))
    return a

if assets.does_asset_exist(PATH):
    unreal.log('Fluid lab already exists; preserved.')
else:
    levels.new_level(PATH)
    stone=mat('M_LabStone',(0.18,0.19,0.20),0.65)
    white=mat('M_LabIvory',(0.65,0.62,0.53),0.38)
    dark=mat('M_LabSlate',(0.045,0.065,0.075),0.4)
    copper=mat('M_LabCopper',(0.35,0.095,0.045),0.24)
    box('Surrounding landscape platform',(0,0,-260),(400,400,1),stone)
    box('Pool bed',(0,0,-170),(30,20,0.8),white)
    for x in (-1550,1550): box('Basin wall',(x,0,-45),(1,22,3.1),white)
    for y in (-1050,1050): box('Basin wall',(0,y,-45),(30,1,3.1),white)
    # Underwater steps provide depth and refraction references.
    for i in range(4): box('Submerged step %d'%i,(-1250+i*140,0,-35-i*32),(1.4,18,0.32),stone)
    for i in range(10): box('Bed inlay %d'%i,(-1350+i*300,0,-128.5),(0.04,20,0.01),dark)
    region=actors.spawn_actor_from_class(unreal.PristineWaterRegion,unreal.Vector())
    region.set_actor_label('Contained water - edit boundary, Bake Landscape, Water Level')
    region.set_editor_property('resolution',48)
    region.bake_landscape()
    unreal.log('PRISTINE_BAKED_VOLUME_M3='+str(region.get_water_volume_m3()))
    water=region.get_editor_property('water')
    water.set_liquid_colors(unreal.LinearColor(0.08,0.25,0.27,1),unreal.LinearColor(0.01,0.18,0.24,1))
    water.set_clarity(0.94)
    water.set_absorption(0.9)
    water.set_volume_depth(500)
    water.set_ripple_strength(0.22)
    water.set_reflectivity(1)
    water.set_total_internal_reflection(0)
    region.reset_water()
    # Reflection references beyond the basin.
    for i in range(7): box('Reflection column %d'%i,(-1400+i*450,1600,220),(0.45,0.45,8+i%3),white if i%2 else copper)
    box('Small fluid plinth',(2100,-600,-155),(4,4,1.5),dark)
    small=actors.spawn_actor_from_class(unreal.PristineSmallFluid,unreal.Vector(2100,-600,-62))
    small.set_actor_label('Small fluid - play to drop and slosh; select to change viscosity')
    small.reset_fluid()
    actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector())
    sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,3000),unreal.Rotator(-30,-35,0))
    sun.light_component.set_editor_property('intensity',5)
    sun.light_component.set_editor_property('atmosphere_sun_light',True)
    sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,1500))
    sky.light_component.set_editor_property('real_time_capture',True)
    sky.light_component.set_editor_property('intensity',1.0)
    p=unreal.Vector(2400,-2900,1100)
    r=unreal.MathLibrary.find_look_at_rotation(p,unreal.Vector(0,100,-50))
    camera=actors.spawn_actor_from_class(unreal.CameraActor,p,r)
    camera.set_editor_property('auto_activate_for_player',unreal.AutoReceiveInput.PLAYER0)
    camera.camera_component.set_editor_property('field_of_view',60)
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(p,r)
    levels.save_current_level()
    unreal.log('PRISTINE_FLUID_LAB_CREATED')
