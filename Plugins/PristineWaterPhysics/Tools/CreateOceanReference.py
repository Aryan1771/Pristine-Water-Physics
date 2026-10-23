"""Create a daylight ocean look-development map in the same project; preserve existing maps.
Run after regenerating the plugin materials. This is a visual reference, not a fluid benchmark.
"""
import unreal
assets=unreal.EditorAssetLibrary
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
path='/Game/PristineNature/L_OceanReference'
if assets.does_asset_exist(path):
    levels.load_level(path)
else:
    assert levels.new_level(path)
if not any(isinstance(a,unreal.PristineWaterSource) for a in actors.get_all_level_actors()):
    ocean=actors.spawn_actor_from_class(unreal.PristineWaterSource,unreal.Vector())
    ocean.set_actor_label('Ocean - directional daylight reference')
    ocean.set_body_type(unreal.PristineWaterBody.OCEAN)
    ocean.apply_cinematic_preset()
    ocean.set_editor_property('half_extent',unreal.Vector2D(1000000,1000000))
    ocean.set_editor_property('normal_tiling',1200)
    ocean.set_editor_property('roughness',.07)
    ocean.set_editor_property('shore_foam',0)
    w=ocean.get_editor_property('water')
    motion=w.get_editor_property('motion')
    motion.set_editor_property('wave_amplitude',25)
    motion.set_editor_property('wave_frequency',.18)
    motion.set_editor_property('wave_speed',.65)
    motion.set_editor_property('ripple_strength',.3)
    motion.set_editor_property('flow_direction',unreal.Vector2D(.85,.53))
    w.set_editor_property('motion',motion)
    optics=w.get_editor_property('optics')
    optics.set_editor_property('deep_color',unreal.LinearColor(.015,.26,.48,1))
    optics.set_editor_property('shallow_color',unreal.LinearColor(.008,.08,.16,1))
    w.set_editor_property('optics',optics)
    w.set_volume_depth(2000);w.set_absorption(1);w.set_clarity(.98)
    ocean.rebuild_surface();ocean.apply_appearance()
    atmosphere=actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector())
    # These are absolute scattering coefficients, not dimensionless multipliers.
    atmosphere.get_component_by_class(unreal.SkyAtmosphereComponent).set_editor_property('mie_scattering_scale',.0008)
    atmosphere.get_component_by_class(unreal.SkyAtmosphereComponent).set_editor_property('rayleigh_scattering_scale',.04)
    sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,4000),unreal.Rotator(pitch=-60,yaw=130,roll=0))
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property('intensity',8)
    sun.light_component.set_editor_property('atmosphere_sun_light',True)
    sun.light_component.set_editor_property('light_source_angle',.535)
    sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector())
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property('real_time_capture',True)
    pp=actors.spawn_actor_from_class(unreal.PostProcessVolume,unreal.Vector())
    pp.set_editor_property('unbound',True)
    settings=pp.get_editor_property('settings')
    settings.set_editor_property('override_auto_exposure_bias',True)
    settings.set_editor_property('auto_exposure_bias',.2)
    pp.set_editor_property('settings',settings)
    camera=actors.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(0,0,650),unreal.Rotator(pitch=-8,yaw=0,roll=0))
    camera.set_actor_label('Ocean reference camera')
    camera.camera_component.set_editor_property('field_of_view',65)
    editor.set_level_viewport_camera_info(camera.get_actor_location(),camera.get_actor_rotation())
    assert levels.save_current_level()
for a in actors.get_all_level_actors():
    if a.get_actor_label()=='Ocean reference camera':editor.set_level_viewport_camera_info(a.get_actor_location(),a.get_actor_rotation())
unreal.log('PRISTINE_OCEAN_REFERENCE_READY')

