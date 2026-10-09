#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "PristineWaterAssetsCommandlet.h"

class FPristineWaterEditorModule : public IModuleInterface
{
    virtual void StartupModule() override
    {
        if (!IsRunningCommandlet()) UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPristineWaterEditorModule::RegisterMenus));
    }
    void RegisterMenus()
    {
        FToolMenuOwnerScoped Owner(this);
        UToolMenu* Menu=UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
        Menu->FindOrAddSection("PristineWater").AddMenuEntry("PristineWaterAssets", FText::FromString("Create Pristine Water Assets"), FText::FromString("Create missing original water textures, master materials and presets. Existing assets are preserved."), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([]{ UPristineWaterAssetsCommandlet::BuildAssets(); })));
    }
    virtual void ShutdownModule() override
    {
        UToolMenus::UnRegisterStartupCallback(this);
        UToolMenus::UnregisterOwner(this);
    }
};
IMPLEMENT_MODULE(FPristineWaterEditorModule, PristineWaterEditor)
