import unreal,time,json
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
r=actors.spawn_actor_from_class(unreal.PristineWaterRegion,unreal.Vector())
r.set_editor_property('resolution',48)
r.reset_water()
f=actors.spawn_actor_from_class(unreal.PristineSmallFluid,unreal.Vector(5000,0,0))
f.set_editor_property('scene_collision',False)
f.reset_fluid()
for a in [r,f]:
    p=a.get_editor_property('performance')
    p.set_editor_property('adaptive_visuals',False)
    p.set_editor_property('max_surface_hz',60)
    p.set_editor_property('preset',unreal.PristinePerformancePreset.CUSTOM)
    a.set_editor_property('performance',p)
    a.apply_performance_settings()
f.set_editor_property('wall_friction',0)
results={}
for name,a in [('region48',r),('small216',f)]:
    for i in range(5): a.advance_simulation(1/60)
    t=time.perf_counter()
    for i in range(60): a.advance_simulation(1/60)
    results[name]=(time.perf_counter()-t)*1000/60
unreal.log('PRISTINE_BENCHMARK '+json.dumps(results))

for a in [r,f]: unreal.log('PROFILE '+str(a.get_editor_property('performance')))
