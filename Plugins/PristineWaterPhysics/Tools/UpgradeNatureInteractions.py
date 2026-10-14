"""Run in the nature comparison project after rebuilding plugin and material assets."""
import unreal
import os
LEVEL='/Game/PristineNature/L_WaterComparison'
ROOT='/Game/PristineNature/Interaction'
assets=unreal.EditorAssetLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
if editor.get_editor_world().get_path_name().split('.')[0]!=LEVEL:
    assert levels.load_level(LEVEL)
# Refresh seabed/shore detail so clear water has something useful to reveal.
scope={'__name__':'pristine_nature_materials'}
with open(os.path.join(os.path.dirname(__file__),'CreateNatureComparison.py'),encoding='utf-8-sig') as f:
    exec(f.read().rsplit('\nbuild()',1)[0],scope)
for name,color,roughness,foliage in [('M_Meadow',(.11,.16,.043),.85,False),('M_Shore',(.19,.14,.075),.64,False),('M_Stone',(.22,.24,.21),.85,False),('M_Leaves',(.085,.19,.038),.72,True)]:
    scope['material'](name,color,roughness,foliage)
for actor in actors.get_all_level_actors():
    if isinstance(actor,(unreal.DirectionalLight,unreal.SkyLight)):
        actor.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    if isinstance(actor,unreal.TextRenderActor) and 'comparison label' in actor.get_actor_label():
        actor.set_actor_rotation(unreal.Rotator(pitch=0,yaw=-90,roll=0),False)
    if isinstance(actor,unreal.PristineWaterRegion):
        water=actor.get_editor_property('water')
        water.set_volume_depth(500);water.set_clarity(.995);water.set_absorption(.6)
        water.set_foam_amount(.025 if actor.resolution>=96 else 0)
        water.apply_water_settings()

def blueprint(name,parent):
    bp=assets.load_asset(ROOT+'/'+name) if assets.does_asset_exist(ROOT+'/'+name) else None
    if not bp:
        factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',parent)
        bp=tools.create_asset(name,ROOT,unreal.Blueprint,factory)
    return bp

bp=blueprint('BP_WaterTester',unreal.PristineWaterCharacter)
cls=assets.load_blueprint_class(ROOT+'/BP_WaterTester')
cdo=unreal.get_default_object(cls)
cdo.set_editor_property('test_locations',[
    unreal.Vector(-7500,-7300,500),unreal.Vector(-4500,-7000,500),
    unreal.Vector(-4500,-3450,450),unreal.Vector(4000,-1200,500),unreal.Vector(4000,4300,-250)])
human_paths=assets.list_assets('/Game/MetaHumans',True,False) if assets.does_directory_exist('/Game/MetaHumans') else []
human=next((p for p in human_paths if p.rsplit('/',1)[-1].split('.')[0]=='BP_MHC_WaterTester'),None)
if human:
    human_cls=assets.load_blueprint_class(human)
    cdo.visual.set_child_actor_class(human_cls)
    cdo.visual.set_relative_rotation(unreal.Rotator(pitch=0,yaw=-90,roll=0),False,False)
    cdo.set_editor_property('third_person',True)
    preview=actors.spawn_actor_from_class(human_cls,unreal.Vector(-7500,-7450,350))
    try:
        body=next((m for m in preview.get_components_by_class(unreal.SkeletalMeshComponent) if m.get_name().lower()=='body'),None)
        if body:
            skeleton=body.get_editor_property('skeletal_mesh_asset').get_editor_property('skeleton')
            for prop,name in [('idle_animation','AS_MH_Neutral_Stand_Idle_Loop'),('walk_animation','AS_MH_Neutral_Walk_Loop_F')]:
                source=assets.load_asset('/MetaHumanCharacter/Optional/Animation/UEFNAnimPreset/Locomotion/'+name)
                if source:
                    # Preserve the clip's authored Control Rig binding. The two MetaHuman
                    # skeletons share their bone layout; changing the clip's skeleton
                    # directly invalidates its editor Control Rig metadata.
                    skeleton.add_compatible_skeleton(source.get_editor_property('skeleton'))
                    target_name='AS_WaterIdle' if prop=='idle_animation' else 'AS_WaterWalk'
                    animation=assets.load_asset(ROOT+'/'+target_name) if assets.does_asset_exist(ROOT+'/'+target_name) else tools.duplicate_asset(target_name,ROOT,source)
                    assets.save_loaded_asset(animation,False)
                    cdo.set_editor_property(prop,animation)
            assets.save_loaded_asset(skeleton,False)
            idle=cdo.get_editor_property('idle_animation')
            if idle:
                tread=unreal.PristineSceneTools.create_treading_animation(idle,ROOT+'/AS_WaterTreading')
                if tread:
                    assets.save_loaded_asset(tread,False)
                    cdo.set_editor_property('swim_animation',tread)
    finally:
        actors.destroy_actor(preview)
assets.save_loaded_asset(bp,False)
mode=blueprint('BP_WaterTestGameMode',unreal.GameModeBase)
mode_cls=assets.load_blueprint_class(ROOT+'/BP_WaterTestGameMode')
unreal.get_default_object(mode_cls).set_editor_property('default_pawn_class',cls)
assets.save_loaded_asset(mode,False)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',mode_cls)
for a in actors.get_all_level_actors():
    if isinstance(a,unreal.PlayerStart):
        a.set_actor_location(unreal.Vector(-7500,-7350,470),False,False)
        a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=90,roll=0),False)
        a.set_actor_label('WASD move - mouse look - Space up - Ctrl dive - V view - 1 to 5 stations')
assert levels.save_current_level()
unreal.log('PRISTINE_INTERACTIONS_INSTALLED MetaHuman='+str(human))
