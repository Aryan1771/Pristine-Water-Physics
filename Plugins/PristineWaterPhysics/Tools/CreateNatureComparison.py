"""UE 5.7: Execute Python Script to build a self-contained four-scale water walk.
All terrain and vegetation are original procedural geometry; no Fab download required.
An existing map is preserved. Change ROOT to create a separate variation.
"""
import math
import random
import unreal

ROOT = '/Game/PristineNature'
MAP = ROOT + '/L_WaterComparison'
# name, x, y, radius x/y, surface elevation, maximum depth, simulation grid
BODIES = [
    ('01 Puddle', -7500, -7000, 150, 90, 250, 4, 48),
    ('02 Woodland pool', -4500, -6300, 550, 380, 200, 35, 64),
    ('03 Reed pond', -4500, -2000, 1500, 1100, 100, 220, 96),
    ('04 Mirror lake', 4000, 4300, 6000, 4200, 0, 650, 128),
]
assets = unreal.EditorAssetLibrary
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
rng = random.Random(20261010)

def shoreline(angle):
    return 1 + .045*math.sin(3*angle) + .035*math.cos(5*angle)

def radius(x, y, b):
    dx,dy=(x-b[1])/b[3],(y-b[2])/b[4]
    return math.hypot(dx,dy)/shoreline(math.atan2(dy,dx))

def height(x, y):
    base=320+80*math.sin(x/2300)*math.cos(y/1900)+35*math.sin((x+y)/870)
    for b in BODIES:
        r=radius(x,y,b)
        if r<1.65:
            basin=b[5]+b[6]*(r*r-1)
            # Puddle banks need a gentle shallow local grade rather than a pit wall.
            bank=b[5]+max(8,b[6])*(r*r-1)
            target=basin if r<1 else bank
            t=max(0,min(1,(r-1.12)/.53)); t=t*t*(3-2*t)
            base=min(base, target*(1-t)+base*t)
    return base

def material(name, color, roughness=.85, foliage=False):
    path=ROOT+'/'+name
    if assets.does_asset_exist(path):
        m=unreal.load_asset(path)
        unreal.MaterialEditingLibrary.delete_all_material_expressions(m)
    else:
        m=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,ROOT,unreal.Material,unreal.MaterialFactoryNew())
    m.set_editor_property('two_sided',True)
    lib=unreal.MaterialEditingLibrary
    pos=lib.create_material_expression(m,unreal.MaterialExpressionWorldPosition)
    custom=lib.create_material_expression(m,unreal.MaterialExpressionCustom)
    inp=unreal.CustomInput(); inp.set_editor_property('input_name','P')
    custom.set_editor_property('inputs',[inp])
    custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    rgb='float3(%s)'%','.join(str(v) for v in color)
    code='''struct Noise {
        float hash(float2 p) { return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453); }
        float value(float2 p) { float2 i=floor(p),f=frac(p);f=f*f*(3-2*f);
            return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),lerp(hash(i+float2(0,1)),hash(i+1),f.x),f.y); }
    }; Noise n;
    float macro=n.value(P.xy/570), fine=n.value(P.xy/23);
    float3 col='''+rgb+'''*(0.65+0.45*macro+0.12*fine);
    '''
    if name=='M_Meadow':
        for b in BODIES:
            code+='float2 d%s=(P.xy-float2(%s,%s))/float2(%s,%s);\n'%(b[7],b[1],b[2],b[3],b[4])
            code+='col=lerp(float3(.12,.095,.05)*(0.8+0.4*fine),col,smoothstep(.94,1.18,length(d%s)));\n'%b[7]
    code+='return col*(.72+.35*n.value(P.xy/3.8)+.18*n.value(P.xy*1.4));'
    custom.set_editor_property('code',code)
    lib.connect_material_expressions(pos,'',custom,'P')
    lib.connect_material_property(custom,'',unreal.MaterialProperty.MP_BASE_COLOR)
    if foliage:
        m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
        lib.connect_material_property(custom,'',unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    else:
        normal=lib.create_material_expression(m,unreal.MaterialExpressionCustom)
        normal_input=unreal.CustomInput();normal_input.set_editor_property('input_name','P')
        normal.set_editor_property('inputs',[normal_input])
        normal.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        normal.set_editor_property('code',code.split('float macro=')[0]+'float2 p=P.xy/8; float h=n.value(p); float dx=(n.value(p+float2(.1,0))-h)*1.2; float dy=(n.value(p+float2(0,.1))-h)*1.2; float fade=saturate(1-length(fwidth(P.xy))*.025); return normalize(float3(-dx*fade,-dy*fade,1));')
        lib.connect_material_expressions(pos,'',normal,'P')
        lib.connect_material_property(normal,'',unreal.MaterialProperty.MP_NORMAL)
    r=lib.create_material_expression(m,unreal.MaterialExpressionConstant)
    r.set_editor_property('r',roughness)
    lib.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
    lib.recompile_material(m); assets.save_loaded_asset(m)
    return m

def mesh(name, verts, tris, mat, collision=False, folder='Nature'):
    a=unreal.PristineSceneTools.create_mesh_actor(editor.get_editor_world(),name,[unreal.Vector(*v) for v in verts],tris,mat,collision)
    if not a: raise RuntimeError('Mesh creation failed: '+name)
    a.set_folder_path(folder)
    return a

def terrain(name,x0,y0,nx,ny,step,mat,cut=False):
    v=[(x0+x*step,y0+y*step,height(x0+x*step,y0+y*step)) for y in range(ny+1) for x in range(nx+1)]
    t=[]
    for y in range(ny):
        for x in range(nx):
            px,py=x0+(x+.5)*step,y0+(y+.5)*step
            if cut and -8250<px<-6750 and -7750<py<-6250: continue
            i=y*(nx+1)+x
            t.extend([i,i+nx+1,i+1,i+1,i+nx+1,i+nx+2])
    return mesh(name,v,t,mat,True,'Terrain')

def tube(v,t,p,q,r1,r2,sides=7):
    # Local orthonormal frame supports inclined branches and reeds.
    def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
    def norm(a):
        length=math.sqrt(sum(c*c for c in a))
        return tuple(c/length for c in a)
    axis=norm(tuple(q[i]-p[i] for i in range(3)))
    ref=(0,0,1) if abs(axis[2])<.9 else (1,0,0)
    u=norm(cross(axis,ref)); w=cross(axis,u)
    off=len(v)
    for c,r in [(p,r1),(q,r2)]:
        for j in range(sides):
            a=j*math.tau/sides
            v.append(tuple(c[k]+r*(u[k]*math.cos(a)+w[k]*math.sin(a)) for k in range(3)))
    for j in range(sides):
        k=(j+1)%sides
        t.extend([off+j,off+sides+j,off+k,off+k,off+sides+j,off+sides+k])

def leaf(v,t,p,size,angle):
    off=len(v); c,s=math.cos(angle),math.sin(angle)
    for x,y,z in [(-size,0,0),(0,size*.45,size*.22),(size,0,0),(0,-size*.45,size*.22)]:
        v.append((p[0]+x*c-y*s,p[1]+x*s+y*c,p[2]+z))
    t.extend([off,off+2,off+1,off,off+3,off+2])

def camera(name,p,target):
    p=(p[0],p[1],max(p[2],height(p[0],p[1])+80))
    r=unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*p),unreal.Vector(*target))
    a=actors.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(*p),r)
    a.set_actor_label(name); a.set_folder_path('Cameras - right click and Pilot')
    a.camera_component.set_editor_property('field_of_view',65)
    return a

def add_trail():
    if any(a.get_actor_label()=='Comparison walking trail' for a in actors.get_all_level_actors()): return
    earth=unreal.load_asset(ROOT+'/M_Shore')
    bark=unreal.load_asset(ROOT+'/M_Bark')
    v=[];t=[]
    points=[(-8000,-7600),(-6100,-7300),(-5000,-7100),(-5400,-4300),(-5700,-3450),(-2400,-2000),(700,-1000)]
    for start,end in zip(points,points[1:]):
        dx,dy=end[0]-start[0],end[1]-start[1];length=math.hypot(dx,dy)
        steps=max(2,int(length/50));nx,ny=-dy/length,dx/length
        for j in range(steps+1):
            f=j/steps;x=start[0]+dx*f;y=start[1]+dy*f;off=len(v)
            width=65+12*math.sin(f*math.pi)
            for side in (-1,1):
                px,py=x+nx*width*side,y+ny*width*side
                v.append((px,py,height(px,py)+1.5))
            if j<steps:t.extend([off,off+2,off+1,off+1,off+2,off+3])
    mesh('Comparison walking trail',v,t,earth,False,'Trail and signs')
    cube=unreal.load_asset('/Engine/BasicShapes/Cube')
    for b in BODIES:
        x,y=b[1]-b[3]*.8,b[2]-b[4]*1.22;z=height(x,y)
        for label,p,scale in [('post',(x,y,z+65),(.14,.14,1.3)),('board',(x,y,z+145),(3.8,.15,.7))]:
            a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*p))
            a.set_actor_label(b[0]+' '+label);a.set_folder_path('Trail and signs')
            a.static_mesh_component.set_static_mesh(cube);a.static_mesh_component.set_material(0,bark)
            a.set_actor_scale3d(unreal.Vector(*scale))
        a=actors.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(x-170,y-9,z+150),unreal.Rotator(pitch=0,yaw=-90,roll=0))
        a.set_actor_label(b[0]+' comparison label');a.set_folder_path('Trail and signs')
        text=a.get_component_by_class(unreal.TextRenderComponent)
        text.set_text(b[0]+'\nDepth: '+str(b[6])+' cm | Grid: '+str(b[7]))
        text.set_world_size(19)

def build():
    if assets.does_asset_exist(MAP):
        levels.load_level(MAP)
        unreal.log('Existing comparison scene preserved: '+MAP)
        return
    if not levels.new_level(MAP): raise RuntimeError('Could not create comparison map')
    grass=material('M_Meadow',(0.11,.16,.043))
    bark=material('M_Bark',(.12,.07,.028))
    leaves=material('M_Leaves',(.085,.19,.038),.72,True)
    reeds=material('M_Reeds',(.18,.24,.045))
    stone=material('M_Stone',(.22,.24,.21))
    earth=material('M_Shore',(.19,.14,.075),.64)
    ground=terrain('Rolling terrain - collision bed',-16000,-15000,256,240,125,grass,True)
    patch=terrain('Fine puddle terrain - 12.5 cm cells',-8250,-7750,120,120,12.5,grass)
    # Thin shore skirts follow the exact collision terrain; keep centres unobstructed.
    for b in BODIES:
        v=[];t=[]
        for j in range(129):
            a=math.tau*j/128
            for rad in (.96,1.035):
                x=b[1]+b[3]*math.cos(a)*shoreline(a)*rad
                y=b[2]+b[4]*math.sin(a)*shoreline(a)*rad
                v.append((x,y,height(x,y)+.8))
            if j<128:
                i=j*2;t.extend([i,i+2,i+1,i+1,i+2,i+3])
        mesh(b[0]+' damp shoreline',v,t,earth,False,'Terrain')
        region=actors.spawn_actor_from_class(unreal.PristineWaterRegion,unreal.Vector(b[1],b[2],b[5]))
        region.set_actor_label(b[0]+' - select Water settings');region.set_folder_path('Water - four scales')
        region.set_editor_property('resolution',b[7]);region.set_editor_property('water_level',0)
        region.set_editor_property('default_depth',b[6])
        boundary=region.get_editor_property('boundary');boundary.clear_spline_points(False)
        for j in range(64):
            a=math.tau*j/64
            boundary.add_spline_point(unreal.Vector(b[3]*1.2*math.cos(a)*shoreline(a),b[4]*1.2*math.sin(a)*shoreline(a),0),unreal.SplineCoordinateSpace.LOCAL,False)
        boundary.set_closed_loop(True,True)
        region.set_editor_property('terrain_actors',[patch if b is BODIES[0] else ground])
        region.bake_landscape();region.apply_cinematic_preset()
        water=region.get_editor_property('water')
        water.set_roughness(.008 if b[7]<96 else .025)
        water.set_ripple_strength(.005 if b[7]<96 else .045)
        water.set_clarity(.995);water.set_absorption(.6)
        # Optical reference distance is a property of the water, not basin depth.
        water.set_volume_depth(500);region.reset_water()
        if region.get_water_volume_m3()<=0: raise RuntimeError('Empty terrain bake for '+b[0])
        unreal.log('NATURE_VOLUME '+b[0]+' '+str(region.get_water_volume_m3()))
    # Original broadleaf woodland: clustered branches and individual leaf geometry.
    for cluster in range(12):
        tv=[];tt=[];lv=[];lt=[]
        for tree in range(12):
            for attempt in range(100):
                # Spatially bounded clusters let the renderer cull distant woodland.
                cx=-10500+(cluster%4)*7000;cy=-7500+(cluster//4)*8500
                x,y=cx+rng.uniform(-3300,3300),cy+rng.uniform(-3800,3800)
                if all(radius(x,y,b)>1.27 for b in BODIES): break
            else: continue
            z=height(x,y);h=rng.uniform(650,1500);width=h*.22
            tube(tv,tt,(x,y,z),(x+30,y-20,z+h),h*.027,5)
            for branch in range(9):
                a=rng.uniform(0,math.tau);r=width*rng.uniform(.55,1.1)
                end=(x+r*math.cos(a),y+r*math.sin(a),z+h*rng.uniform(.57,.98))
                tube(tv,tt,(x,y,z+h*.42),end,10,1.8,5)
                for n in range(65):
                    theta=rng.uniform(0,math.tau);rr=rng.random()**.5*width*.58
                    p=(end[0]+rr*math.cos(theta),end[1]+rr*math.sin(theta),end[2]+rng.uniform(-width*.3,width*.38))
                    leaf(lv,lt,p,rng.uniform(24,48),rng.uniform(0,math.tau))
        mesh('Woodland %02d branches'%cluster,tv,tt,bark,False,'Vegetation/Woodland')
        mesh('Woodland %02d leaves'%cluster,lv,lt,leaves,False,'Vegetation/Woodland')
    for bi,b in enumerate(BODIES):
        v=[];t=[]
        for n in range(150 if bi<2 else 850):
            a=rng.uniform(0,math.tau);rad=rng.uniform(1.015,1.22)
            x=b[1]+b[3]*math.cos(a)*shoreline(a)*rad
            y=b[2]+b[4]*math.sin(a)*shoreline(a)*rad
            z=height(x,y)+1
            for j in range(5):
                h=rng.uniform(12,35) if bi==0 else rng.uniform(30,115)
                ang=rng.uniform(0,math.tau);off=len(v);w=1.4
                v.extend([(x-w,y,z),(x+w,y,z),(x+math.cos(ang)*h*.3,y+math.sin(ang)*h*.3,z+h)])
                t.extend([off,off+2,off+1])
        mesh(b[0]+' marginal grasses',v,t,reeds,False,'Vegetation/Shore')
    # Rounded irregular river stones from engine geometry (no external assets).
    sphere=unreal.load_asset('/Engine/BasicShapes/Sphere')
    for b in BODIES:
        for j in range(24):
            a=rng.uniform(0,math.tau);rad=rng.uniform(1.02,1.2)
            x=b[1]+b[3]*math.cos(a)*shoreline(a)*rad;y=b[2]+b[4]*math.sin(a)*shoreline(a)*rad
            s=rng.uniform(.1,.35) if b[7]<64 else rng.uniform(.35,1.7)
            rock=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x,y,height(x,y)))
            rock.set_actor_label(b[0]+' shoreline stone');rock.set_folder_path('Nature/Stones')
            rock.static_mesh_component.set_static_mesh(sphere);rock.static_mesh_component.set_material(0,stone)
            rock.set_actor_scale3d(unreal.Vector(s*1.3,s,s*.6))
    actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector())
    sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,4000),unreal.Rotator(pitch=-24,yaw=-42,roll=0))
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property('intensity',5)
    sun.light_component.set_editor_property('atmosphere_sun_light',True)
    sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,3000))
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property('real_time_capture',True)
    fog=actors.spawn_actor_from_class(unreal.ExponentialHeightFog,unreal.Vector(0,0,-400))
    fog.component.set_editor_property('fog_density',.006)
    camera('00 OVERVIEW - all four scales',(-14500,-16000,12500),(0,500,0))
    for b in BODIES:
        camera(b[0]+' CAMERA',(b[1]+b[3]*.55,b[2]-b[4]*1.32,b[5]+max(55,b[3]*.13)),(b[1],b[2]+b[4]*.3,b[5]))
    add_trail()
    start=actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(-7800,-7600,450))
    start.set_actor_label('Start at puddle - fly toward pool, pond, lake')
    p=unreal.Vector(-14500,-16000,12500)
    editor.set_level_viewport_camera_info(p,unreal.MathLibrary.find_look_at_rotation(p,unreal.Vector(0,500,0)))
    if not levels.save_current_level(): raise RuntimeError('Could not save comparison map')
    assets.save_directory(ROOT)
    unreal.log('PRISTINE_NATURE_COMPLETE '+MAP)

build()
