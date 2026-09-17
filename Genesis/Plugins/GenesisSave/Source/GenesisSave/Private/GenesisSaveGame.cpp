// GENESIS: Der Kreislauf des Lebens

#include "GenesisSaveGame.h"
#include "Misc/Crc.h"

const FGenesisSystemRecord* UGenesisSaveGame::FindRecord(FName SystemId) const
{
	return Records.FindByPredicate([SystemId](const FGenesisSystemRecord& Record)
	{
		return Record.SystemId == SystemId;
	});
}

uint32 UGenesisSaveGame::ComputeChecksum(const TArray<FGenesisSystemRecord>& InRecords)
{
	uint32 Crc = 0;
	for (const FGenesisSystemRecord& Record : InRecords)
	{
		// FName als String hashen: Name-Indizes sind nicht stabil zwischen Programmstarts
		const FString IdString = Record.SystemId.ToString();
		Crc = FCrc::StrCrc32(*IdString, Crc);
		Crc = FCrc::MemCrc32(&Record.SchemaVersion, sizeof(Record.SchemaVersion), Crc);
		if (Record.Payload.Num() > 0)
		{
			Crc = FCrc::MemCrc32(Record.Payload.GetData(), Record.Payload.Num(), Crc);
		}
	}
	return Crc;
}

void UGenesisSaveGame::UpdateChecksum()
{
	Header.RecordsChecksum = ComputeChecksum(Records);
}

bool UGenesisSaveGame::Validate(FString& OutError) const
{
	if (Header.FormatVersion <= 0 || Header.FormatVersion > CurrentFormatVersion)
	{
		OutError = FString::Printf(TEXT("Unbekannte Format-Version %d (unterstützt: 1..%d)."), Header.FormatVersion, CurrentFormatVersion);
		return false;
	}

	const uint32 Actual = ComputeChecksum(Records);
	if (Actual != Header.RecordsChecksum)
	{
		OutError = FString::Printf(TEXT("Prüfsumme ungültig (erwartet %08X, berechnet %08X)."), Header.RecordsChecksum, Actual);
		return false;
	}

	return true;
}
