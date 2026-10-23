"""Apply scale-aware surface detail to the existing comparison; preserve terrain and water volume.
Run after CreateAlpineLake.py. Save your custom optical settings before applying this look preset.
"""
import unreal
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
if editor.get_editor_world().get_path_name().split('.')[0]!='/Game/PristineNature/L_WaterComparison':
    assert levels.load_level('/Game/PristineNature/L_WaterComparison')
profiles={'01':(90,.035,.012),'02':(180,.07,.018),'03':(400,.15,.025),'04':(900,.25,.025)}
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if not isinstance(a,unreal.PristineWaterRegion):continue
    profile=profiles.get(a.get_actor_label()[:2])
    if profile is None:continue
    volume=a.get_water_volume_m3()
    scale,strength,roughness=profile
    w=a.get_editor_property('water')
    motion=w.get_editor_property('motion')
    motion.set_editor_property('surface_detail_scale_cm',scale)
    motion.set_editor_property('ripple_strength',strength)
    w.set_editor_property('motion',motion)
    w.set_roughness(roughness)
    w.apply_water_settings()
    assert abs(a.get_water_volume_m3()-volume)<1e-6,'Appearance must not refill a basin'
assert levels.save_current_level()
unreal.log('PRISTINE_SCALE_DETAIL_READY')
