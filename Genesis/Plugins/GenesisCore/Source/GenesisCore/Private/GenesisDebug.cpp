// GENESIS: Der Kreislauf des Lebens

#include "GenesisDebug.h"
#include "DisplayDebugHelpers.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeLock.h"

namespace GenesisDebug
{
	namespace
	{
		FCriticalSection PagesLock;
		TArray<FGenesisDebugPage> Pages;
		FDelegateHandle HudHookHandle;

		TAutoConsoleVariable<FString> CVarDebugPage(
			TEXT("genesis.Debug.Page"),
			TEXT(""),
			TEXT("Zeigt im GENESIS Developer HUD nur die Seite mit dieser Id (leer = alle Seiten)."));

		const FName GenesisDebugCategory(TEXT("Genesis"));

#if !UE_BUILD_SHIPPING
		// Führt Befehle nach einer Wartezeit aus – z. B. damit automatische Screenshots einen eingeschwungenen Zustand zeigen.
		// Mehrere Befehle mit ';' trennen (',' trennt bereits die -ExecCmds der Kommandozeile).
		FAutoConsoleCommandWithWorldAndArgs DelayedExecCommand(
			TEXT("genesis.Debug.After"),
			TEXT("Entwickler: genesis.Debug.After <Sekunden> <Befehl>[; <Befehl>...] – führt die Befehle verzögert aus."),
			FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
			{
				if (Args.Num() < 2)
				{
					return;
				}
				const float Seconds = FMath::Max(0.0f, FCString::Atof(*Args[0]));
				const FString Commands = FString::Join(TArrayView<const FString>(Args).RightChop(1), TEXT(" "));
				const TWeakObjectPtr<UWorld> WeakWorld(World);
				FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakWorld, Commands](float)
				{
					UWorld* TargetWorld = WeakWorld.Get();
					if (TargetWorld && GEngine)
					{
						TArray<FString> Parts;
						Commands.ParseIntoArray(Parts, TEXT(";"));
						for (const FString& Part : Parts)
						{
							// Über den Viewport wie eine Konsoleneingabe – nur so erreichen Viewport-Befehle (z. B. "shot") ihr Ziel
							if (GEngine->GameViewport && GEngine->GameViewport->GetWorld() == TargetWorld)
							{
								GEngine->GameViewport->ConsoleCommand(Part.TrimStartAndEnd());
							}
							else
							{
								GEngine->Exec(TargetWorld, *Part.TrimStartAndEnd());
							}
						}
					}
					return false;
				}), Seconds);
			}));
#endif

		void DrawOnHud(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& YL, float& YPos)
		{
			if (!Canvas || !HUD || !DisplayInfo.IsDisplayOn(GenesisDebugCategory))
			{
				return;
			}

			TArray<FString> Lines;
			CollectAllLines(HUD->GetWorld(), Lines);

			FDisplayDebugManager& DebugManager = Canvas->DisplayDebugManager;
			for (const FString& Line : Lines)
			{
				// Überschriften (beginnen mit "==") farbig hervorheben
				if (Line.StartsWith(TEXT("==")))
				{
					DebugManager.SetDrawColor(FColor(255, 196, 120));
				}
				else
				{
					DebugManager.SetDrawColor(FColor::White);
				}
				DebugManager.DrawString(Line);
			}
		}
	}

	void RegisterPage(FGenesisDebugPage Page)
	{
		FScopeLock Lock(&PagesLock);
		Pages.RemoveAll([&Page](const FGenesisDebugPage& Existing) { return Existing.Id == Page.Id; });
		Pages.Add(MoveTemp(Page));
		Pages.Sort([](const FGenesisDebugPage& A, const FGenesisDebugPage& B) { return A.Id.LexicalLess(B.Id); });
	}

	void UnregisterPage(FName PageId)
	{
		FScopeLock Lock(&PagesLock);
		Pages.RemoveAll([PageId](const FGenesisDebugPage& Existing) { return Existing.Id == PageId; });
	}

	void CollectAllLines(const UWorld* World, TArray<FString>& OutLines)
	{
		const FString Filter = CVarDebugPage.GetValueOnGameThread();

		TArray<FGenesisDebugPage> Snapshot;
		{
			FScopeLock Lock(&PagesLock);
			Snapshot = Pages;
		}

		for (const FGenesisDebugPage& Page : Snapshot)
		{
			if (!Filter.IsEmpty() && !Page.Id.ToString().Equals(Filter, ESearchCase::IgnoreCase))
			{
				continue;
			}

			OutLines.Add(FString::Printf(TEXT("== %s =="), *Page.Title));
			if (Page.CollectLines)
			{
				Page.CollectLines(World, OutLines);
			}
		}
	}

	void InstallHudHook()
	{
#if !UE_BUILD_SHIPPING
		if (!HudHookHandle.IsValid())
		{
			HudHookHandle = AHUD::OnShowDebugInfo.AddStatic(&DrawOnHud);
		}
#endif
	}

	void RemoveHudHook()
	{
		if (HudHookHandle.IsValid())
		{
			AHUD::OnShowDebugInfo.Remove(HudHookHandle);
			HudHookHandle.Reset();
		}
	}
}
