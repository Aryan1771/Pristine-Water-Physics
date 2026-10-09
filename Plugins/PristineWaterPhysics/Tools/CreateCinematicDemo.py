"""Run with Unreal Editor's Execute Python Script. Creates /Game/PristineCinematicDemo.
Requires the plugin assets, DX12/SM6 and hardware ray tracing already enabled.
Preserves an existing demo; does not alter the original plugin test map.
"""
import unreal
assets=unreal.EditorAssetLibrary
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
path='/Game/PristineCinematicDemo/L_CinematicMirrorWater'
if not assets.does_asset_exist(path):
    if not levels.new_level_from_template(path,'/PristineWaterPhysics/Demo/L_PristineFluidLab'):
        raise RuntimeError('Could not duplicate the fluid lab. Install the bundled plugin content first.')
    region=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.PristineWaterRegion))
    region.set_editor_property('resolution',128)
    region.bake_landscape()
    region.apply_cinematic_preset()
    region.get_editor_property('water').set_ripple_strength(0.005)
    ivory=unreal.load_asset('/PristineWaterPhysics/Demo/M_LabIvory')
    copper=unreal.load_asset('/PristineWaterPhysics/Demo/M_LabCopper')
    def box(label,position,scale,material):
        a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*position))
        a.set_actor_label(label)
        a.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
        a.static_mesh_component.set_material(0,material)
        a.set_actor_scale3d(unreal.Vector(*scale))
        return a
    for a in actors.get_all_level_actors():
        if a.get_actor_label()=='Pool bed':
            a.static_mesh_component.set_material(0,unreal.load_asset('/PristineWaterPhysics/Demo/M_LabSlate'))
        if a.get_actor_label().startswith('Reflection column'):
            actors.destroy_actor(a)
    for i in range(5):
        box('Reflection arcade pillar '+str(i),(-1400+i*700,1500,380),(1.2,1.2,8),ivory)
    box('Reflection arcade lintel',(0,1500,810),(30,1.7,0.6),ivory)
    box('Copper reflection panel',(0,1750,340),(6,0.3,6.8),copper)
    p=unreal.Vector(600,-750,70)
    rotation=unreal.MathLibrary.find_look_at_rotation(p,unreal.Vector(0,500,15))
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(p,rotation)
    for a in actors.get_all_level_actors():
        if isinstance(a,unreal.CameraActor):
            a.set_actor_location(p,False,False)
            a.set_actor_rotation(rotation,False)
    levels.save_current_level()
else:
    levels.load_level(path)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
# Session-only high-quality reflection settings. Requires compatible project configuration.
for setting in [
    'r.Lumen.HardwareRayTracing 1',
    'r.Lumen.HardwareRayTracing.LightingMode 2',
    'r.Lumen.Reflections.ScreenTraces 0',
    'r.Lumen.Reflections.DownsampleFactor 1',
    'r.Water.SingleLayer.Reflection.DownsampleFactor 1',
    'r.ScreenPercentage 100',
]:
    unreal.SystemLibrary.execute_console_command(world,setting)
unreal.log('Cinematic water demo ready: '+path)



