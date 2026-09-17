// GENESIS: Der Kreislauf des Lebens

#include "GenesisRandom.h"

namespace GenesisHash
{
	uint64 FromGuid(const FGuid& Guid)
	{
		const uint64 High = (static_cast<uint64>(Guid.A) << 32) | static_cast<uint64>(Guid.B);
		const uint64 Low = (static_cast<uint64>(Guid.C) << 32) | static_cast<uint64>(Guid.D);
		return Combine(Mix64(High), Low);
	}

	uint64 FromString(FStringView Text)
	{
		// FNV-1a 64 Bit
		uint64 Hash = 0xCBF29CE484222325ull;
		for (TCHAR Character : Text)
		{
			Hash ^= static_cast<uint64>(FChar::ToLower(Character));
			Hash *= 0x00000100000001B3ull;
		}
		return Mix64(Hash);
	}

	uint64 FromName(FName Name)
	{
		return FromString(Name.ToString());
	}
}

namespace
{
	inline uint64 RotateLeft(uint64 Value, int32 Shift)
	{
		return (Value << Shift) | (Value >> (64 - Shift));
	}

	/** SplitMix64-Schritt: empfohlene Initialisierung für xoshiro-Zustände. */
	inline uint64 SplitMix64Next(uint64& InOutState)
	{
		InOutState += 0x9E3779B97F4A7C15ull;
		return GenesisHash::Mix64(InOutState);
	}
}

void FGenesisRandomStream::Reset(uint64 InSeed)
{
	Seed = InSeed;

	uint64 SplitState = InSeed;
	State0 = SplitMix64Next(SplitState);
	State1 = SplitMix64Next(SplitState);
	State2 = SplitMix64Next(SplitState);
	State3 = SplitMix64Next(SplitState);

	// xoshiro darf nie einen reinen Null-Zustand haben
	if ((State0 | State1 | State2 | State3) == 0)
	{
		State0 = 0x9E3779B97F4A7C15ull;
	}
}

uint64 FGenesisRandomStream::NextUInt64()
{
	const uint64 Result = RotateLeft(State1 * 5, 7) * 9;
	const uint64 Temp = State1 << 17;

	State2 ^= State0;
	State3 ^= State1;
	State1 ^= State2;
	State0 ^= State3;
	State2 ^= Temp;
	State3 = RotateLeft(State3, 45);

	return Result;
}

double FGenesisRandomStream::NextDouble()
{
	// 53 signifikante Bits → exakt darstellbare Gleitkommazahl in [0, 1)
	return static_cast<double>(NextUInt64() >> 11) * (1.0 / 9007199254740992.0);
}

int32 FGenesisRandomStream::RandRange(int32 Min, int32 Max)
{
	if (Max <= Min)
	{
		return Min;
	}

	const uint64 Range = static_cast<uint64>(static_cast<int64>(Max) - static_cast<int64>(Min) + 1);

	// Rejection Sampling gegen Modulo-Verzerrung
	const uint64 Threshold = (0ull - Range) % Range;
	uint64 Value = NextUInt64();
	while (Value < Threshold)
	{
		Value = NextUInt64();
	}

	return static_cast<int32>(static_cast<int64>(Min) + static_cast<int64>(Value % Range));
}

bool FGenesisRandomStream::Bernoulli(float Probability)
{
	if (Probability <= 0.0f)
	{
		return false;
	}
	if (Probability >= 1.0f)
	{
		return true;
	}
	return NextDouble() < static_cast<double>(Probability);
}

float FGenesisRandomStream::Gaussian(float Mean, float StandardDeviation)
{
	const double U1 = FMath::Max(NextDouble(), 1.0e-12);
	const double U2 = NextDouble();
	const double StandardNormal = FMath::Sqrt(-2.0 * FMath::Loge(U1)) * FMath::Cos(2.0 * UE_DOUBLE_PI * U2);
	return Mean + StandardDeviation * static_cast<float>(StandardNormal);
}

FGuid FGenesisRandomStream::NewGuid()
{
	const uint64 High = NextUInt64();
	const uint64 Low = NextUInt64();

	FGuid Result(
		static_cast<uint32>(High >> 32),
		static_cast<uint32>(High),
		static_cast<uint32>(Low >> 32),
		static_cast<uint32>(Low));

	if (!Result.IsValid())
	{
		Result.D = 1;
	}
	return Result;
}

FGenesisRandomStream FGenesisRandomStream::Derive(uint64 Salt) const
{
	return FGenesisRandomStream(GenesisHash::Combine(Seed, Salt));
}
