"""Extend the existing comparison map with an original alpine lake setting.
Run after CreateNatureComparison.py and UpgradeNatureInteractions.py. No downloads.
The lake solver and the three smaller comparison basins are retained.
"""
import math
import os
import random
import unreal

scope={'__name__':'alpine_dependencies'}
with open(os.path.join(os.path.dirname(__file__),'CreateNatureComparison.py'),encoding='utf-8-sig') as f:
    exec(f.read().rsplit('\nbuild()',1)[0],scope)
assets=unreal.EditorAssetLibrary
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
if editor.get_editor_world().get_path_name().split('.')[0]!='/Game/PristineNature/L_WaterComparison':
    assert levels.load_level('/Game/PristineNature/L_WaterComparison')
for a in actors.get_all_level_actors():
    if a.get_actor_label().startswith('Alpine - '): actors.destroy_actor(a)
    elif any(a.get_actor_label().startswith('Woodland %02d '%i) for i in (6,7,10,11)):
        a.set_actor_hidden_in_game(True)
        a.set_is_temporarily_hidden_in_editor(True)

def noise(x,y):
    def h(a,b):
        n=math.sin(a*127.1+b*311.7)*43758.5453
        return n-math.floor(n)
    ix,iy=math.floor(x),math.floor(y)
    fx,fy=x-ix,y-iy;fx=fx*fx*(3-2*fx);fy=fy*fy*(3-2*fy)
    return (h(ix,iy)*(1-fx)+h(ix+1,iy)*fx)*(1-fy)+(h(ix,iy+1)*(1-fx)+h(ix+1,iy+1)*fx)*fy

def mountain(x,y,r,a):
    # Keep the approach to the comparison stations open to the south-west.
    north=(math.sin(a)+1)*.5
    envelope=math.exp(-((r-(2.45+.2*math.sin(a*3)))/1.05)**2)
    ridge=(1-abs(noise(x/2400,y/2400)*2-1))
    detail=noise(x/800,y/800)*.20+noise(x/270,y/270)*.07
    amplitude=900+6500*north**1.25
    rise=max(0,min(1,(r-1.01)/.55))
    # A buried collar joins the mountain mesh to the retained comparison terrain.
    # Start the rise beyond the cropped triangles, whose vertices straddle the cut.
    access=max(max(0,min(1,(x-2200)/4000)),max(0,min(1,(y-700)/1800)))
    for basin in scope['BODIES'][:3]:
        edge=max(abs(x-basin[1])-(basin[3]*1.8+500),abs(y-basin[2])-(basin[4]*1.8+500))
        blend=max(0,min(1,(edge-800)/1500));access*=blend*blend*(3-2*blend)
    z=scope['height'](x,y)-6+access*(6+rise*envelope*amplitude*(.6+.32*ridge+detail*.5))
    return z

def alpine_material():
    m=scope['material']('M_AlpineRock',(.20,.22,.21),.86)
    lib=unreal.MaterialEditingLibrary
    p=lib.create_material_expression(m,unreal.MaterialExpressionWorldPosition)
    n=lib.create_material_expression(m,unreal.MaterialExpressionVertexNormalWS)
    c=lib.create_material_expression(m,unreal.MaterialExpressionCustom)
    inputs=[]
    for name in ('P','N'):
        i=unreal.CustomInput();i.set_editor_property('input_name',name);inputs.append(i)
    c.set_editor_property('inputs',inputs)
    c.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    c.set_editor_property('code','''struct Noise {
      float hash(float2 p){return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);}
      float value(float2 p){float2 i=floor(p),f=frac(p);f=f*f*(3-2*f);
        return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),lerp(hash(i+float2(0,1)),hash(i+1),f.x),f.y);}
    };Noise n;float broad=n.value(P.xy/500),fine=n.value(P.xy/18);
    float strata=n.value(float2(P.x*.012+P.z*.014,P.y*.008));
    float3 rock=lerp(float3(.035,.045,.052),float3(.13,.15,.155),broad)*(.72+.38*fine+.15*strata);
    float grass=smoothstep(.55,.91,N.z)*(1-smoothstep(3000,7500,P.z))*(.55+.45*broad);
    float3 col=lerp(rock,float3(.075,.13,.025)*(.65+.65*fine),grass);
    float snow=smoothstep(4600,6400,P.z+900*(broad-.5))*smoothstep(.45,.82,N.z);
    return lerp(col,float3(.58,.65,.70),snow);''')
    assert lib.connect_material_expressions(p,'',c,'P')
    assert lib.connect_material_expressions(n,'',c,'N')
    lib.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
    lib.recompile_material(m);assets.save_loaded_asset(m,False)
    return m

rock=alpine_material()
verts=[];tri=[]
angles=256;rings=112
for j in range(rings+1):
    r=1.012+(4.8-1.012)*(j/rings)**1.45
    for i in range(angles+1):
        a=math.tau*i/angles;s=scope['shoreline'](a)
        x=4000+6000*r*math.cos(a)*s;y=4300+4200*r*math.sin(a)*s
        verts.append((x,y,mountain(x,y,r,a)+2))
cut_vertices=set()
for j in range(rings):
    for i in range(angles):
        k=j*(angles+1)+i
        corners=[verts[k],verts[k+1],verts[k+angles+1],verts[k+angles+2]]
        cx=sum(p[0] for p in corners)/4;cy=sum(p[1] for p in corners)/4
        # Do not span the centimetre-deep comparison pits with coarse mountain triangles.
        if (cx<1000 and cy<-500) or any(abs(cx-b[1])<b[3]*1.8+500 and abs(cy-b[2])<b[4]*1.8+500 for b in scope['BODIES'][:3]):
            cut_vertices.update((k,k+1,k+angles+1,k+angles+2))
            continue
        tri.extend((k,k+1,k+angles+1,k+1,k+angles+2,k+angles+1))
# Boundary vertices are shared with retained triangles. Bury the join even
# where coarse outer-ring cells cross the analytic fade margin.
for k in cut_vertices:
    x,y,_=verts[k];verts[k]=(x,y,scope['height'](x,y)-8)
scope['mesh']('Alpine - rocky cirque and meadow',verts,tri,rock,True,'Alpine lake')

# Irregular original rocks at the shore and the overview foreground.
rng=random.Random(1771)
for index in range(34):
    a=rng.uniform(-math.pi,math.pi);r=rng.uniform(1.025,1.25)
    x=4000+6000*r*math.cos(a)*scope['shoreline'](a)
    y=4300+4200*r*math.sin(a)*scope['shoreline'](a)
    z=mountain(x,y,r,a);size=rng.uniform(70,220)
    v=[];t=[];seg=16;lat=10
    for j in range(lat+1):
        phi=math.pi*j/lat
        for i in range(seg+1):
            theta=math.tau*i/seg
            irregular=.8+.35*noise(i*.7+index,j*.7)
            v.append((x+math.sin(phi)*math.cos(theta)*size*irregular,y+math.sin(phi)*math.sin(theta)*size*.8*irregular,z+math.cos(phi)*size*.7*irregular))
    for j in range(lat):
        for i in range(seg):
            k=j*(seg+1)+i;t.extend((k,k+1,k+seg+1,k+1,k+seg+2,k+seg+1))
    scope['mesh']('Alpine - shore boulder %02d'%index,v,t,rock,True,'Alpine lake/Rocks')

grass=scope['material']('M_AlpineGrass',(.14,.25,.035),.82,True)
v=[];t=[]
for index in range(1400):
    a=rng.uniform(-math.pi,0);r=rng.uniform(1.02,1.55)
    x=4000+6000*r*math.cos(a)*scope['shoreline'](a)
    y=4300+4200*r*math.sin(a)*scope['shoreline'](a)
    z=mountain(x,y,r,a)+3
    for blade in range(3):
        ang=rng.uniform(0,math.tau);h=rng.uniform(8,25);k=len(v)
        v.extend(((x-1,y,z),(x+1,y,z),(x+math.cos(ang)*h*.3,y+math.sin(ang)*h*.3,z+h)))
        t.extend((k,k+2,k+1))
scope['mesh']('Alpine - meadow grasses',v,t,grass,False,'Alpine lake/Vegetation')

for a in actors.get_all_level_actors():
    if isinstance(a,unreal.PristineWaterRegion) and a.get_actor_label().startswith('04'):
        w=a.get_editor_property('water')
        optics=w.get_editor_property('optics')
        optics.set_editor_property('deep_color',unreal.LinearColor(.12,.56,.70,1))
        optics.set_editor_property('shallow_color',unreal.LinearColor(.025,.16,.20,1))
        w.set_editor_property('optics',optics)
        w.set_roughness(.012);w.set_ripple_strength(.022);w.set_clarity(.997)
        w.set_absorption(.7);w.set_volume_depth(500);w.apply_water_settings()
    if isinstance(a,unreal.DirectionalLight):
        a.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        a.set_actor_rotation(unreal.Rotator(pitch=-48,yaw=65,roll=0),False)
    if isinstance(a,unreal.SkyLight):a.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)

if not any(isinstance(a,unreal.VolumetricCloud) for a in actors.get_all_level_actors()):
    cloud=actors.spawn_actor_from_class(unreal.VolumetricCloud,unreal.Vector())
    cloud.set_actor_label('Alpine - cloud reflections')
    component=cloud.get_component_by_class(unreal.VolumetricCloudComponent)
    component.set_material(unreal.load_asset('/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst'))
    component.set_editor_property('layer_bottom_altitude',.7)
    component.set_editor_property('layer_height',2.0)

# Keep the lake station above the new bank before gravity settles the character.
path='/Game/PristineNature/Interaction/BP_WaterTester'
if assets.does_asset_exist(path):
    cdo=unreal.get_default_object(assets.load_blueprint_class(path))
    positions=list(cdo.get_editor_property('test_locations'))
    if len(positions)>3:
        positions[3]=unreal.Vector(4000,-300,1500)
        cdo.set_editor_property('test_locations',positions)
        assets.save_asset(path,False)
# Centre each teaching label on its existing board.
by_label={a.get_actor_label():a for a in actors.get_all_level_actors()}
for label,a in by_label.items():
    if isinstance(a,unreal.TextRenderActor) and label.endswith(' comparison label'):
        board=by_label.get(label.replace(' comparison label',' board'))
        if board:
            a.set_actor_location(board.get_actor_location()+unreal.Vector(0,-9,5),False,False)
            text=a.get_component_by_class(unreal.TextRenderComponent)
            alignment=next(getattr(unreal.HorizTextAligment,n) for n in dir(unreal.HorizTextAligment) if n.endswith('CENTER'))
            text.set_horizontal_alignment(alignment)
volume=actors.spawn_actor_from_class(unreal.PostProcessVolume,unreal.Vector())
volume.set_actor_label('Alpine - exposure')
volume.set_editor_property('unbound',True)
settings=volume.get_editor_property('settings')
settings.set_editor_property('override_auto_exposure_bias',True)
settings.set_editor_property('auto_exposure_bias',.25)
volume.set_editor_property('settings',settings)

p=unreal.Vector(8000,-5500,4000);target=unreal.Vector(3000,5800,1800)
rotation=unreal.MathLibrary.find_look_at_rotation(p,target)
camera=actors.spawn_actor_from_class(unreal.CameraActor,p,rotation)
camera.set_actor_label('Alpine - reference overview');camera.set_folder_path('Cameras - right click and Pilot')
camera.camera_component.set_editor_property('field_of_view',65)
editor.set_level_viewport_camera_info(p,rotation)
assert levels.save_current_level()
unreal.log('PRISTINE_ALPINE_COMPLETE')
