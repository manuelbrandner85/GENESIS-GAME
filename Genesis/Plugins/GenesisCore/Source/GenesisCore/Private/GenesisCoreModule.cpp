// GENESIS: Der Kreislauf des Lebens

#include "Modules/ModuleManager.h"
#include "GenesisDebug.h"
#include "GenesisLog.h"

DEFINE_LOG_CATEGORY(LogGenesis);

/** Core-Modul: installiert beim Start die Entwickler-Infrastruktur (Developer HUD). */
class FGenesisCoreModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		GenesisDebug::InstallHudHook();
	}

	virtual void ShutdownModule() override
	{
		GenesisDebug::RemoveHudHook();
	}
};

IMPLEMENT_MODULE(FGenesisCoreModule, GenesisCore)
