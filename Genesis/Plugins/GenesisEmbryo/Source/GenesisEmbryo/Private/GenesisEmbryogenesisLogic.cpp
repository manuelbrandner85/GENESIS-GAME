// GENESIS: Der Kreislauf des Lebens

#include "GenesisEmbryogenesisLogic.h"
#include "GenesisRandom.h"

namespace GenesisEmbryogenesisLogic
{
	namespace
	{
		/** Wert aus einer Stützstellentabelle (Tag → Wert), dazwischen linear, außerhalb gehalten. */
		float DayTable(std::initializer_list<FVector2f> List, float Day)
		{
			const FVector2f* Points = List.begin();
			const int32 Count = static_cast<int32>(List.size());
			if (Day <= Points[0].X)
			{
				return Points[0].Y;
			}
			for (int32 Index = 1; Index < Count; ++Index)
			{
				if (Day <= Points[Index].X)
				{
					const float Alpha = (Day - Points[Index - 1].X) / FMath::Max(1.0e-3f, Points[Index].X - Points[Index - 1].X);
					return FMath::Lerp(Points[Index - 1].Y, Points[Index].Y, Alpha);
				}
			}
			return Points[Count - 1].Y;
		}
	}

	float LengthAt(float Day)
	{
		// Scheitel-Steiß-Länge: Tag 15 gut 0,4 mm (die Keimscheibe), Tag 20 1,5–2 mm, Tag 24 3 mm, Tag 28 4–5 mm
		return DayTable({ {14.0f, 0.3f}, {15.0f, 0.4f}, {18.0f, 1.0f}, {20.0f, 1.8f}, {22.0f, 2.4f}, {24.0f, 3.0f}, {26.0f, 3.8f}, {28.0f, 4.6f} }, Day);
	}

	float HeartRateAt(float Day, const FGenesisEmbryogenesisTuning& Tuning)
	{
		if (Day < Tuning.HeartBeatDay)
		{
			return 0.0f;
		}
		// Der erste Schlag ist langsam und wird rasch schneller (Ultraschall: Woche 5 ~100, Woche 6 ~120, Woche 7 ~150)
		return Tuning.FirstBeatBpm + Tuning.BpmPerDay * (Day - Tuning.HeartBeatDay);
	}

	float DefectRiskFor(float NutritionQuality, const FGenesisEmbryogenesisTuning& Tuning)
	{
		const float Factor = FMath::Lerp(Tuning.PoorNutritionFactor, Tuning.GoodNutritionFactor, FMath::Clamp(NutritionQuality, 0.0f, 1.0f));
		return FMath::Clamp(Tuning.BaseDefectRisk * Factor, 0.0f, 1.0f);
	}

	bool Advance(FGenesisEmbryogenesisState& State, const FGenesisEmbryogenesisTuning& Tuning,
		float Day, float NutritionQuality, bool bPlayerEmbryo, uint64 Seed)
	{
		const EGenesisEmbryogenesisStage Before = State.Stage;

		// Form und Größe
		State.LengthMm = LengthAt(Day);
		State.PrimitiveStreak = FMath::SmoothStep(Tuning.StreakDay, Tuning.StreakDay + 1.5f, Day);
		State.ThreeLayers = FMath::SmoothStep(Tuning.GastrulationDay, Tuning.NotochordDay, Day);
		State.Notochord = FMath::SmoothStep(Tuning.NotochordDay, Tuning.NeuralPlateDay + 0.5f, Day);
		State.NeuralPlate = FMath::SmoothStep(Tuning.NeuralPlateDay, Tuning.NeuralPlateDay + 1.0f, Day);
		State.NeuralFolds = FMath::SmoothStep(Tuning.NeuralPlateDay + 0.5f, Tuning.TubeClosureStartDay, Day);

		// Somiten: das erste Paar an Tag 20, danach eines alle 6–8 Stunden
		const float SomiteHours = (Day - Tuning.FirstSomiteDay) * 24.0f;
		State.Somites = Day < Tuning.FirstSomiteDay ? 0
			: FMath::Clamp(1 + FMath::FloorToInt(SomiteHours / FMath::Max(1.0f, Tuning.HoursPerSomite)), 0, Tuning.MaxSomites);

		// Das Neuralrohr schließt in der Mitte zuerst und läuft nach vorn und hinten zu
		State.TubeClosure = FMath::SmoothStep(Tuning.TubeClosureStartDay, Tuning.CaudalClosedDay, Day);
		State.RostralNeuropore = 1.0f - FMath::SmoothStep(Tuning.TubeClosureStartDay + 0.5f, Tuning.RostralClosedDay, Day);
		State.CaudalNeuropore = 1.0f - FMath::SmoothStep(Tuning.RostralClosedDay - 0.5f, Tuning.CaudalClosedDay, Day);

		// Herz: zwei Schläuche verschmelzen, dann schlägt es
		State.HeartTube = FMath::SmoothStep(Tuning.HeartBeatDay - 3.0f, Tuning.HeartBeatDay, Day);
		State.bHeartBeating = Day >= Tuning.HeartBeatDay;
		State.HeartRateBpm = HeartRateAt(Day, Tuning);

		// Gesicht, Sinne, Gliedmaßen kündigen sich an
		State.PharyngealArches = Day < 24.0f ? 0 : FMath::Clamp(1 + FMath::FloorToInt((Day - 24.0f) / 1.2f), 0, 4);
		State.OpticVesicles = FMath::SmoothStep(22.0f, 26.0f, Day);
		State.OticPits = FMath::SmoothStep(23.0f, 27.0f, Day);
		State.LimbBuds = FMath::SmoothStep(26.0f, 29.0f, Day);
		State.Curvature = FMath::SmoothStep(21.0f, 28.0f, Day);

		// Neuralrohrdefekt: einmal gewürfelt, wenn sich das Rohr zu schließen beginnt
		if (!State.bDefectRolled && Day >= Tuning.TubeClosureStartDay)
		{
			State.bDefectRolled = true;
			State.DefectRisk = DefectRiskFor(NutritionQuality, Tuning);
			FGenesisRandomStream Rng(GenesisHash::Combine(Seed, 0x4E7B12ull));
			if (!bPlayerEmbryo && Rng.Bernoulli(State.DefectRisk))
			{
				// Vorn offen heißt Anenzephalie, hinten offen ein offener Rücken. Vorn ist seltener.
				State.Defect = Rng.Bernoulli(0.45f) ? EGenesisNeuralTubeDefect::Anencephaly : EGenesisNeuralTubeDefect::SpinaBifida;
			}
		}

		// Ein offener Neuroporus bleibt offen
		if (State.Defect == EGenesisNeuralTubeDefect::Anencephaly)
		{
			State.RostralNeuropore = FMath::Max(State.RostralNeuropore, 0.6f);
			State.TubeClosure = FMath::Min(State.TubeClosure, 0.75f);
		}
		else if (State.Defect == EGenesisNeuralTubeDefect::SpinaBifida)
		{
			State.CaudalNeuropore = FMath::Max(State.CaudalNeuropore, 0.45f);
			State.TubeClosure = FMath::Min(State.TubeClosure, 0.85f);
		}

		// Stufe
		EGenesisEmbryogenesisStage Stage = EGenesisEmbryogenesisStage::None;
		if (Day >= Tuning.StreakDay) Stage = EGenesisEmbryogenesisStage::PrimitiveStreak;
		if (Day >= Tuning.GastrulationDay) Stage = EGenesisEmbryogenesisStage::Gastrulation;
		if (Day >= Tuning.NotochordDay) Stage = EGenesisEmbryogenesisStage::Notochord;
		if (Day >= Tuning.NeuralPlateDay) Stage = EGenesisEmbryogenesisStage::NeuralPlate;
		if (Day >= Tuning.FirstSomiteDay) Stage = EGenesisEmbryogenesisStage::Somites;
		if (Day >= Tuning.HeartBeatDay) Stage = EGenesisEmbryogenesisStage::HeartBeats;
		if (Day >= Tuning.CaudalClosedDay) Stage = EGenesisEmbryogenesisStage::NeuralTubeClosed;
		if (Day >= Tuning.CompleteDay) Stage = EGenesisEmbryogenesisStage::Complete;
		State.Stage = Stage;

		return State.Stage != Before;
	}

	FString GetStageName(EGenesisEmbryogenesisStage Stage)
	{
		switch (Stage)
		{
		case EGenesisEmbryogenesisStage::None: return TEXT("zweiblättrige Keimscheibe");
		case EGenesisEmbryogenesisStage::PrimitiveStreak: return TEXT("Primitivstreifen");
		case EGenesisEmbryogenesisStage::Gastrulation: return TEXT("Gastrulation");
		case EGenesisEmbryogenesisStage::Notochord: return TEXT("Chorda");
		case EGenesisEmbryogenesisStage::NeuralPlate: return TEXT("Neuralplatte");
		case EGenesisEmbryogenesisStage::Somites: return TEXT("Somiten");
		case EGenesisEmbryogenesisStage::HeartBeats: return TEXT("das Herz schlägt");
		case EGenesisEmbryogenesisStage::NeuralTubeClosed: return TEXT("Neuralrohr geschlossen");
		case EGenesisEmbryogenesisStage::Complete: return TEXT("Ende der vierten Woche");
		default: return TEXT("unbekannt");
		}
	}

	FString DescribeStage(EGenesisEmbryogenesisStage Stage)
	{
		switch (Stage)
		{
		case EGenesisEmbryogenesisStage::None:
			return TEXT("Zwei Blätter, eine Scheibe von einem halben Millimeter. Mehr bist du noch nicht.");
		case EGenesisEmbryogenesisStage::PrimitiveStreak:
			return TEXT("Tag 15 – am hinteren Rand zieht sich eine Furche durch die Scheibe: der Primitivstreifen. Von jetzt an hast du vorn und hinten, links und rechts.");
		case EGenesisEmbryogenesisStage::Gastrulation:
			return TEXT("Tag 16 – Zellen wandern durch die Furche nach innen. Aus zwei Blättern werden drei: Darm und Lunge, Muskeln und Knochen, Haut und Nerven.");
		case EGenesisEmbryogenesisStage::Notochord:
			return TEXT("Tag 18 – die Chorda spannt sich als Achse durch die Scheibe. Über ihr verdickt sich das Ektoderm zur Neuralplatte.");
		case EGenesisEmbryogenesisStage::NeuralPlate:
			return TEXT("Tag 19 – die Ränder der Neuralplatte heben sich. In dieser Rinne entstehen Gehirn und Rückenmark.");
		case EGenesisEmbryogenesisStage::Somites:
			return TEXT("Tag 20 – rechts und links der Achse erscheinen Somitenpaare, eines alle sieben Stunden. Aus ihnen werden Wirbel, Rippen und Muskeln.");
		case EGenesisEmbryogenesisStage::HeartBeats:
			return TEXT("Tag 22 – aus zwei Schläuchen ist einer geworden, und er zieht sich zusammen. Dein Herz schlägt. Es wird nicht mehr aufhören, bis alles vorbei ist.");
		case EGenesisEmbryogenesisStage::NeuralTubeClosed:
			return TEXT("Tag 26–28 – das Neuralrohr ist geschlossen. Kiemenbögen formen das Gesicht, Augenbläschen wölben sich vor, an den Seiten knospen Arme und Beine.");
		case EGenesisEmbryogenesisStage::Complete:
			return TEXT("Ende der vierten Woche – ein gekrümmter Embryo von vier Millimetern mit dreißig Somitenpaaren und einem schlagenden Herzen.");
		default:
			return FString();
		}
	}

	FString GetDefectName(EGenesisNeuralTubeDefect Defect)
	{
		switch (Defect)
		{
		case EGenesisNeuralTubeDefect::Anencephaly: return TEXT("vorderer Neuroporus offen (Anenzephalie)");
		case EGenesisNeuralTubeDefect::SpinaBifida: return TEXT("hinterer Neuroporus offen (offener Rücken)");
		default: return TEXT("keiner");
		}
	}
}
