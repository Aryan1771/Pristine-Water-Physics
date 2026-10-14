#include "PristineWaterAssetsCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionSingleLayerWaterMaterialOutput.h"
#include "Materials/MaterialExpressionSceneDepthWithoutWater.h"
#include "Materials/MaterialExpressionPixelDepth.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "MaterialEditingLibrary.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "ShaderCompiler.h"
#include "MaterialShared.h"
#include "PristineWaterSource.h"
#include "PristineWaterRegion.h"
#include "PristineSmallFluid.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionSceneDepth.h"
#include "Materials/MaterialExpressionTwoSidedSign.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"

namespace PristineAssets
{
bool Save(UObject* Asset)
{
    Asset->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags=RF_Public|RF_Standalone;
    return UPackage::SavePackage(Asset->GetOutermost(),Asset,*FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),Args);
}

UTexture2D* Texture(const FString& Name, bool Foam)
{
    const FString Path=TEXT("/PristineWaterPhysics/Textures/")+Name;
    if (UTexture2D* Existing=LoadObject<UTexture2D>(nullptr,*Path)) return Existing;
    UTexture2D* T=NewObject<UTexture2D>(CreatePackage(*Path),*Name,RF_Public|RF_Standalone);
    constexpr int32 N=512;
    TArray<FColor> Pixels;
    Pixels.SetNumUninitialized(N*N);
    // Integer-frequency periodic waves produce seamless authored data, not photographs.
    for (int32 Y=0;Y<N;++Y) for (int32 X=0;X<N;++X)
    {
        const double U=double(X)/N, V=double(Y)/N;
        double DX=0,DY=0,H=0;
        for (int32 I=1;I<=12;++I)
        {
            const double KX=(I%4+1), KY=(I*7%9-4);
            const double Phase=2*PI*(U*KX+V*KY)+I*2.39996;
            const double Weight=1.0/(I+2);
            H+=FMath::Sin(Phase)*Weight;
            DX+=FMath::Cos(Phase)*KX*Weight*0.3;
            DY+=FMath::Cos(Phase)*KY*Weight*0.3;
        }
        if (Foam)
        {
            const uint8 C=uint8(FMath::Clamp((0.18-FMath::Abs(H))*7.0,0.0,1.0)*255);
            Pixels[Y*N+X]=FColor(C,C,C,255);
        }
        else
        {
            const FVector Normal=FVector(-DX,-DY,1).GetSafeNormal();
            Pixels[Y*N+X]=FColor(uint8((Normal.X*0.5+0.5)*255),uint8((Normal.Y*0.5+0.5)*255),uint8((Normal.Z*0.5+0.5)*255),255);
        }
    }
    T->Source.Init(N,N,1,1,TSF_BGRA8,reinterpret_cast<const uint8*>(Pixels.GetData()));
    T->SRGB=false;
    T->CompressionSettings=Foam ? TC_Masks : TC_Normalmap;
    T->LODGroup=Foam ? TEXTUREGROUP_World : TEXTUREGROUP_WorldNormalMap;
    T->AddressX=TA_Wrap; T->AddressY=TA_Wrap;
    T->PostEditChange();
    FAssetRegistryModule::AssetCreated(T);
    Save(T);
    return T;
}

struct FGraph
{
    UMaterial* M;
    TMap<FString,UMaterialExpression*> Inputs;
    int32 Row=0;
    template<class T> T* Node()
    {
        T* E=NewObject<T>(M);
        M->GetExpressionCollection().AddExpression(E);
        E->MaterialExpressionEditorX=(Row/16)*360-2400;
        E->MaterialExpressionEditorY=(Row%16)*180;
        ++Row;
        return E;
    }
    void Scalar(const TCHAR* Name,float Value)
    {
        auto* E=Node<UMaterialExpressionScalarParameter>();
        E->ParameterName=Name; E->DefaultValue=Value; E->Group=TEXT("Pristine Water"); Inputs.Add(Name,E);
    }
    void Vector(const TCHAR* Name,FLinearColor Value)
    {
        auto* E=Node<UMaterialExpressionVectorParameter>();
        E->ParameterName=Name; E->DefaultValue=Value; E->Group=TEXT("Pristine Water"); Inputs.Add(Name,E);
    }
    UMaterialExpressionCustom* Custom(const TCHAR* Label,const FString& Code,ECustomMaterialOutputType Type,std::initializer_list<const TCHAR*> Names)
    {
        auto* E=Node<UMaterialExpressionCustom>();
        E->Description=Label; E->Code=Code; E->OutputType=Type; E->Inputs.Empty();
        for (const TCHAR* Name:Names)
        {
            FCustomInput I; I.InputName=Name;
            I.Input.Connect(FString(Name).StartsWith(TEXT("Interaction")) ? 5 : 0,Inputs.FindChecked(Name));
            E->Inputs.Add(I);
        }
        Inputs.Add(Label,E); return E;
    }
};

UMaterial* Material(const FString& Name,int32 Quality,UTexture2D* Normal,UTexture2D* Foam)
{
    const FString Path=TEXT("/PristineWaterPhysics/Materials/")+Name;
    UMaterial* M=LoadObject<UMaterial>(nullptr,*Path);
    const bool bNew=M==nullptr;
    if (!M) M=NewObject<UMaterial>(CreatePackage(*Path),*Name,RF_Public|RF_Standalone);
    else UMaterialEditingLibrary::DeleteAllMaterialExpressions(M);
    M->SetShadingModel(Quality==0 ? MSM_DefaultLit : MSM_SingleLayerWater);
    M->BlendMode=BLEND_Opaque;
    M->TwoSided=true;
    M->bTangentSpaceNormal=Quality>=3;
    FGraph G{M};
    G.Scalar(TEXT("RefractiveIndex"),1.333f); G.Scalar(TEXT("Reflectivity"),0.65f);
    G.Scalar(TEXT("FresnelPower"),5); G.Scalar(TEXT("FresnelStrength"),1);
    G.Scalar(TEXT("TotalInternalReflection"),0.35f); G.Scalar(TEXT("RefractionStrength"),0.18f);
    G.Scalar(TEXT("VolumeDepth"),350); G.Scalar(TEXT("Absorption"),1.15f); G.Scalar(TEXT("Clarity"),0.75f);
    G.Scalar(TEXT("FoamAmount"),0.15f); G.Scalar(TEXT("WaveAmplitude"),8); G.Scalar(TEXT("WaveFrequency"),1.2f);
    G.Scalar(TEXT("WaveSpeed"),0.7f); G.Scalar(TEXT("RippleStrength"),0.25f);
    G.Scalar(TEXT("FlowDirectionX"),1); G.Scalar(TEXT("FlowDirectionY"),0);
    G.Scalar(TEXT("Roughness"),Quality==4?0.008f:0.075f); G.Scalar(TEXT("NormalTiling"),500);
    G.Scalar(TEXT("ShoreFoam"),0.3f); G.Scalar(TEXT("ShoreWidth"),100);
    G.Vector(TEXT("ShallowColor"),FLinearColor(0.12f,0.55f,0.75f));
    G.Vector(TEXT("DeepColor"),FLinearColor(0.01f,0.06f,0.12f));
    for (int32 I=0;I<4;++I) G.Vector(*FString::Printf(TEXT("Interaction%d"),I),FLinearColor(0,0,-100,0));
    auto* WP=G.Node<UMaterialExpressionWorldPosition>(); WP->WorldPositionShaderOffset=WPT_ExcludeAllShaderOffsets;
    G.Inputs.Add(TEXT("P"),WP);
    G.Inputs.Add(TEXT("T"),G.Node<UMaterialExpressionTime>());
    G.Inputs.Add(TEXT("View"),G.Node<UMaterialExpressionCameraVectorWS>());
    const FString WaveCode=TEXT("float2 d=float2(FlowDirectionX,FlowDirectionY); d=dot(d,d)>0.0001?normalize(d):float2(1,0); float2 e=float2(d.x*0.6-d.y*0.8,d.x*0.8+d.y*0.6); float k=max(0,WaveFrequency)*0.0062831853; float a=dot(P.xy,d)*k-T*WaveSpeed; float b=dot(P.xy,e)*k*1.73-T*WaveSpeed*1.31; ");
    auto* WPO=G.Custom(TEXT("Displacement"),WaveCode+TEXT("return float3(0,0,WaveAmplitude*(sin(a)*0.65+sin(b)*0.35));"),CMOT_Float3,{TEXT("P"),TEXT("T"),TEXT("WaveAmplitude"),TEXT("WaveFrequency"),TEXT("WaveSpeed"),TEXT("FlowDirectionX"),TEXT("FlowDirectionY")});
    if(Quality<3) M->GetEditorOnlyData()->WorldPositionOffset.Connect(0,WPO);
    G.Custom(TEXT("MacroNormal"),WaveCode+TEXT("float2 g=WaveAmplitude*k*(d*cos(a)*0.65+e*cos(b)*0.6055); return normalize(float3(-g,1));"),CMOT_Float3,{TEXT("P"),TEXT("T"),TEXT("WaveAmplitude"),TEXT("WaveFrequency"),TEXT("WaveSpeed"),TEXT("FlowDirectionX"),TEXT("FlowDirectionY")});
    auto* UV=G.Custom(TEXT("WaterUV"),TEXT("return P.xy/max(10,NormalTiling)+float2(FlowDirectionX,FlowDirectionY)*T*WaveSpeed*0.025;"),CMOT_Float2,{TEXT("P"),TEXT("T"),TEXT("NormalTiling"),TEXT("FlowDirectionX"),TEXT("FlowDirectionY"),TEXT("WaveSpeed")});
    auto* N1=G.Node<UMaterialExpressionTextureSampleParameter2D>();
    N1->ParameterName=TEXT("NormalTextureA"); N1->Texture=Normal; N1->SamplerType=SAMPLERTYPE_Normal; N1->Coordinates.Connect(0,UV); G.Inputs.Add(TEXT("N1"),N1);
    auto* UV2=G.Custom(TEXT("WaterUV2"),TEXT("return float2(-WaterUV.y,WaterUV.x)*1.73+float2(0.12,-0.17)*T*WaveSpeed*0.1;"),CMOT_Float2,{TEXT("WaterUV"),TEXT("T"),TEXT("WaveSpeed")});
    auto* N2=G.Node<UMaterialExpressionTextureSampleParameter2D>();
    N2->ParameterName=TEXT("NormalTextureB"); N2->Texture=Normal; N2->SamplerType=SAMPLERTYPE_Normal; N2->Coordinates.Connect(0,UV2); G.Inputs.Add(TEXT("N2"),N2);
    auto* Mask=G.Node<UMaterialExpressionTextureSampleParameter2D>();
    Mask->ParameterName=TEXT("FoamMask"); Mask->Texture=Foam; Mask->SamplerType=SAMPLERTYPE_Masks; Mask->Coordinates.Connect(0,UV); G.Inputs.Add(TEXT("Mask"),Mask);
    FString NormalCode=Quality==0 ? TEXT("float2 g=N1.xy;") : TEXT("float2 g=N1.xy+N2.xy*0.55;");
    if (Quality==2) NormalCode+=TEXT("float4 r[4]={Interaction0,Interaction1,Interaction2,Interaction3}; for(int i=0;i<4;i++){float age=T-r[i].z; float2 v=P.xy-r[i].xy; float dist=length(v); float ring=dist-age*120; float strength=r[i].w*step(0,age)*saturate(1-age/4)*exp(-ring*ring/1800); g+=v/max(dist,1)*cos(ring*0.12)*strength;} ");
    NormalCode+=TEXT("return normalize(float3(MacroNormal.xy+g*RippleStrength*(0.5+RefractionStrength),max(0.1,MacroNormal.z))); ");
    if(Quality>=3) NormalCode=TEXT("return normalize(float3((N1.xy+N2.xy*0.55)*RippleStrength,1));");
    auto* FinalNormal=G.Custom(TEXT("SurfaceNormal"),NormalCode,CMOT_Float3,{TEXT("N1"),TEXT("N2"),TEXT("MacroNormal"),TEXT("RippleStrength"),TEXT("RefractionStrength"),TEXT("Interaction0"),TEXT("Interaction1"),TEXT("Interaction2"),TEXT("Interaction3"),TEXT("P"),TEXT("T")});
    M->GetEditorOnlyData()->Normal.Connect(0,FinalNormal);
    if (Quality>0)
    {
        G.Inputs.Add(TEXT("SceneZ"),G.Node<UMaterialExpressionSceneDepthWithoutWater>());
        G.Inputs.Add(TEXT("PixelZ"),G.Node<UMaterialExpressionPixelDepth>());
        G.Custom(TEXT("DepthBlend"),TEXT("return saturate(max(0,SceneZ-PixelZ)/max(1,VolumeDepth));"),CMOT_Float1,{TEXT("SceneZ"),TEXT("PixelZ"),TEXT("VolumeDepth")});
        G.Custom(TEXT("Foam"),TEXT("float shore=1-saturate(max(0,SceneZ-PixelZ)/max(1,ShoreWidth)); return saturate((shore*ShoreFoam+FoamAmount*0.18)*smoothstep(0.2,0.8,Mask.r));"),CMOT_Float1,{TEXT("SceneZ"),TEXT("PixelZ"),TEXT("ShoreWidth"),TEXT("ShoreFoam"),TEXT("FoamAmount"),TEXT("Mask")});
    }
    else
    {
        G.Scalar(TEXT("DepthBlend"),0.8f);
        G.Custom(TEXT("Foam"),TEXT("return saturate(FoamAmount*0.18*smoothstep(0.2,0.8,Mask.r));"),CMOT_Float1,{TEXT("FoamAmount"),TEXT("Mask")});
    }
    if(Quality>=3)
    {
        G.Inputs.Add(TEXT("FlowFoam"),G.Node<UMaterialExpressionVertexColor>());
        G.Custom(TEXT("CombinedFoam"),Quality==4?TEXT("return saturate(Foam+FlowFoam.r*FoamAmount);"):TEXT("return saturate(Foam+FlowFoam.r);"),CMOT_Float1,{TEXT("Foam"),TEXT("FlowFoam"),TEXT("FoamAmount")});
        G.Inputs[TEXT("Foam")]=G.Inputs[TEXT("CombinedFoam")];
    }
    auto* Color=G.Custom(TEXT("SurfaceColor"),Quality==4?TEXT("return float3(0.8,0.85,0.82)*Foam;"):TEXT("return lerp(lerp(ShallowColor.rgb,DeepColor.rgb,DepthBlend)*0.35,float3(0.8,0.85,0.82),Foam);"),CMOT_Float3,{TEXT("ShallowColor"),TEXT("DeepColor"),TEXT("DepthBlend"),TEXT("Foam")});
    M->GetEditorOnlyData()->BaseColor.Connect(0,Color);
    auto* Rough=G.Custom(TEXT("SurfaceRoughness"),TEXT("return lerp(clamp(Roughness,0.001,1),0.55,Foam);"),CMOT_Float1,{TEXT("Roughness"),TEXT("Foam")});
    M->GetEditorOnlyData()->Roughness.Connect(0,Rough);
    auto* Spec=G.Custom(TEXT("SurfaceSpecular"),Quality==4?TEXT("float f=(RefractiveIndex-1)/(RefractiveIndex+1); return saturate(f*f/0.08*Reflectivity);"):TEXT("float i=max(1,RefractiveIndex); float f=(i-1)/(i+1); float edge=pow(1-saturate(dot(SurfaceNormal,View)),max(0.1,FresnelPower)); return saturate(f*f/0.08*Reflectivity*(FresnelStrength+edge*TotalInternalReflection));"),CMOT_Float1,{TEXT("RefractiveIndex"),TEXT("Reflectivity"),TEXT("FresnelPower"),TEXT("FresnelStrength"),TEXT("TotalInternalReflection"),TEXT("SurfaceNormal"),TEXT("View")});
    M->GetEditorOnlyData()->Specular.Connect(0,Spec);
    if (Quality>0)
    {
        auto* SLW=G.Node<UMaterialExpressionSingleLayerWaterMaterialOutput>();
        // Underwater path attenuation is supplied by the camera effect; behind a back face is air.
        G.Inputs.Add(TEXT("Facing"),G.Node<UMaterialExpressionTwoSidedSign>());
        auto* Absorb=G.Custom(TEXT("AbsorptionCoefficient"),TEXT("return max(0,Facing)*-log(clamp(DeepColor.rgb,0.001,0.99))*max(0,Absorption)/max(1,VolumeDepth);"),CMOT_Float3,{TEXT("DeepColor"),TEXT("Absorption"),TEXT("VolumeDepth"),TEXT("Facing")});
        auto* Scatter=G.Custom(TEXT("ScatteringCoefficient"),TEXT("return max(0,Facing)*max(0,ShallowColor.rgb)*(1-saturate(Clarity))*0.02;"),CMOT_Float3,{TEXT("ShallowColor"),TEXT("Clarity"),TEXT("Facing")});
        SLW->AbsorptionCoefficients.Connect(0,Absorb); SLW->ScatteringCoefficients.Connect(0,Scatter);
        auto* Opacity=G.Custom(TEXT("SurfaceOpacity"),TEXT("return lerp(0.02,1,Foam);"),CMOT_Float1,{TEXT("Foam")});
        M->GetEditorOnlyData()->Opacity.Connect(0,Opacity);
    }
    M->PostEditChange();
    if(bNew) FAssetRegistryModule::AssetCreated(M);
    return M;
}
UMaterial* VolumeMaterial(const FString& Name)
{
    const FString Path=TEXT("/PristineWaterPhysics/Materials/")+Name;
    if(UMaterial* Existing=LoadObject<UMaterial>(nullptr,*Path)) return Existing;
    UMaterial* M=NewObject<UMaterial>(CreatePackage(*Path),*Name,RF_Public|RF_Standalone);
    M->SetShadingModel(MSM_DefaultLit); M->BlendMode=BLEND_Translucent;
    M->TranslucencyLightingMode=TLM_SurfacePerPixelLighting; M->RefractionMethod=RM_IndexOfRefraction;
    M->TwoSided=true;
    M->bUsedWithInstancedStaticMeshes=true;
    FGraph G{M};
    G.Scalar(TEXT("RefractiveIndex"),1.333f); G.Scalar(TEXT("Roughness"),0.025f);
    G.Scalar(TEXT("Opacity"),0.12f); G.Vector(TEXT("Tint"),FLinearColor(0.88,0.97,1.0));
    auto* Spec=G.Custom(TEXT("DielectricF0"),TEXT("float f=(RefractiveIndex-1)/(RefractiveIndex+1); return saturate(f*f/0.08);"),CMOT_Float1,{TEXT("RefractiveIndex")});
    M->GetEditorOnlyData()->BaseColor.Connect(0,G.Inputs[TEXT("Tint")]);
    M->GetEditorOnlyData()->Roughness.Connect(0,G.Inputs[TEXT("Roughness")]);
    M->GetEditorOnlyData()->Specular.Connect(0,Spec);
    M->GetEditorOnlyData()->Opacity.Connect(0,G.Inputs[TEXT("Opacity")]);
    M->GetEditorOnlyData()->Refraction.Connect(0,G.Inputs[TEXT("RefractiveIndex")]);
    M->PostEditChange(); FAssetRegistryModule::AssetCreated(M); return M;
}
UMaterial* UnderwaterMaterial()
{
    const FString Path=TEXT("/PristineWaterPhysics/Materials/M_PristineUnderwater");
    UMaterial* M=LoadObject<UMaterial>(nullptr,*Path);
    const bool bNew=!M;
    if(!M) M=NewObject<UMaterial>(CreatePackage(*Path),TEXT("M_PristineUnderwater"),RF_Public|RF_Standalone);
    else UMaterialEditingLibrary::DeleteAllMaterialExpressions(M);
    M->MaterialDomain=MD_PostProcess;
    FGraph G{M};
    auto* Scene=G.Node<UMaterialExpressionSceneTexture>();Scene->SceneTextureId=PPI_PostProcessInput0;
    G.Inputs.Add(TEXT("Scene"),Scene);
    G.Inputs.Add(TEXT("Distance"),G.Node<UMaterialExpressionSceneDepth>());
    G.Inputs.Add(TEXT("View"),G.Node<UMaterialExpressionCameraVectorWS>());
    G.Scalar(TEXT("Immersion"),0);G.Scalar(TEXT("EyeDepth"),100);G.Scalar(TEXT("OpticalDistance"),500);G.Scalar(TEXT("Absorption"),.6);
    G.Vector(TEXT("Transmission"),FLinearColor(.18,.65,.72));
    G.Vector(TEXT("ScatterColor"),FLinearColor(.08,.18,.16));G.Scalar(TEXT("Clarity"),.995);
    auto* Color=G.Custom(TEXT("Underwater attenuation"),TEXT("float path=max(0,Distance); if(View.z<-.001) path=min(path,EyeDepth/max(.001,-View.z)); float3 sigma=-log(clamp(Transmission.rgb,.001,.999))*max(0,Absorption)/max(1,OpticalDistance)+max(0,ScatterColor.rgb)*(1-saturate(Clarity))*.02; float3 tr=exp(-sigma*path); return lerp(Scene.rgb,Scene.rgb*tr+max(0,ScatterColor.rgb)*.35*(1-tr),saturate(Immersion));"),CMOT_Float3,{TEXT("Scene"),TEXT("Distance"),TEXT("View"),TEXT("Immersion"),TEXT("EyeDepth"),TEXT("OpticalDistance"),TEXT("Absorption"),TEXT("Transmission"),TEXT("ScatterColor"),TEXT("Clarity")});
    M->GetEditorOnlyData()->EmissiveColor.Connect(0,Color);
    M->PostEditChange();if(bNew) FAssetRegistryModule::AssetCreated(M);return M;
}
}

UPristineWaterAssetsCommandlet::UPristineWaterAssetsCommandlet() { IsClient=false; IsEditor=true; IsServer=false; LogToConsole=true; }
int32 UPristineWaterAssetsCommandlet::Main(const FString& Params) { return BuildAssets()?0:1; }

bool UPristineWaterAssetsCommandlet::BuildAssets()
{
    using namespace PristineAssets;
    UTexture2D* Normal=Texture(TEXT("T_PristineNormal"),false);
    UTexture2D* Foam=Texture(TEXT("T_PristineFoam"),true);
    bool OK=true;
    const TCHAR* Names[]={TEXT("M_PristineLow"),TEXT("M_PristineMedium"),TEXT("M_PristineHigh")};
    UMaterial* Masters[4];
    for (int32 Q=0;Q<3;++Q) Masters[Q]=Material(Names[Q],Q,Normal,Foam);
    Masters[3]=Material(TEXT("M_PristineFluid"),3,Normal,Foam);
    UMaterial* Cinematic=Material(TEXT("M_PristineCinematic"),4,Normal,Foam);
    UMaterial* Volume=VolumeMaterial(TEXT("M_PristineVolume"));
    UMaterial* Droplet=VolumeMaterial(TEXT("M_PristineDroplet"));
    UMaterial* Underwater=UnderwaterMaterial();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    for (UMaterial* M:{Masters[0],Masters[1],Masters[2],Masters[3],Cinematic,Volume,Droplet,Underwater})
    {
        if (FMaterialResource* Resource=M->GetMaterialResource(GMaxRHIShaderPlatform))
        {
            for (const FString& Error:Resource->GetCompileErrors())
            {
                UE_LOG(LogTemp,Error,TEXT("%s: %s"),*M->GetName(),*Error);
                OK=false;
            }
        }
        OK=Save(M)&&OK;
    }
    const TCHAR* Presets[]={TEXT("Clear"),TEXT("Tropical"),TEXT("Ocean"),TEXT("Emerald"),TEXT("Oil")};
    const FLinearColor Colors[]={FLinearColor(0.02,0.12,0.18),FLinearColor(0.01,0.32,0.28),FLinearColor(0.004,0.025,0.09),FLinearColor(0.025,0.12,0.035),FLinearColor(0.06,0.025,0.008)};
    for (int32 I=0;I<5;++I)
    {
        const FString Name=FString(TEXT("MI_Pristine_"))+Presets[I];
        const FString Path=TEXT("/PristineWaterPhysics/Materials/Instances/")+Name;
        if (LoadObject<UMaterialInstanceConstant>(nullptr,*Path)) continue;
        auto* MI=NewObject<UMaterialInstanceConstant>(CreatePackage(*Path),*Name,RF_Public|RF_Standalone);
        MI->SetParentEditorOnly(Masters[1]);
        MI->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("DeepColor")),Colors[I]);
        if (I==4) MI->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("RefractiveIndex")),1.47);
        MI->PostEditChange(); FAssetRegistryModule::AssetCreated(MI); OK=Save(MI)&&OK;
    }
    UE_LOG(LogTemp,Display,TEXT("PRISTINE_ASSETS_RESULT: %s"),OK?TEXT("SUCCESS"):TEXT("FAILED"));
    const TCHAR* Bodies[]={TEXT("Puddle"),TEXT("Pond"),TEXT("Lake"),TEXT("Sea"),TEXT("Ocean")};
    for (int32 I=0;I<5;++I)
    {
        const FString Name=FString(TEXT("BP_Pristine"))+Bodies[I];
        const FString Path=TEXT("/PristineWaterPhysics/Blueprints/")+Name;
        if (LoadObject<UBlueprint>(nullptr,*Path)) continue;
        UBlueprint* BP=FKismetEditorUtilities::CreateBlueprint(APristineWaterSource::StaticClass(),CreatePackage(*Path),*Name,BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass());
        auto* CDO=CastChecked<APristineWaterSource>(BP->GeneratedClass->GetDefaultObject());
        CDO->BodyType=static_cast<EPristineWaterBody>(I);
        CDO->ApplyBodyPreset();
        FAssetRegistryModule::AssetCreated(BP);
        OK=Save(BP)&&OK;
    }
    return OK;
}
