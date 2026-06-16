// Copyright Epic Games, Inc. All Rights Reserved.

#include "CellTransformer/InstancedActorsCellTransformerBase.h"

#include "Engine/Level.h"
#include "Engine/World.h"
#include "InstancedActorsData.h"
#include "InstancedActorsManager.h"
#include "WorldPartition/WorldPartitionRuntimeCellInterface.h"

DEFINE_LOG_CATEGORY(LogInstancedActorsCellTransformer);

#if WITH_EDITOR

bool UInstancedActorsCellTransformerBase::ShouldConvertActorToInstanced(const AActor* Actor) const
{
	// Stub extension point - override to add per-actor filtering. By default every actor matching
	// ActorClassesToConvert is converted.
	return true;
}

TSubclassOf<AInstancedActorsManager> UInstancedActorsCellTransformerBase::GetInstancedActorsManagerClass() const
{
	return AInstancedActorsManager::StaticClass();
}

void UInstancedActorsCellTransformerBase::Transform(ULevel* InLevel)
{
	if (!InLevel)
	{
		return;
	}

	UWorld* World = InLevel->GetWorld();
	if (!World)
	{
		return;
	}

	if (ActorClassesToConvert.IsEmpty())
	{
		return;
	}

	UE_LOG(LogInstancedActorsCellTransformer, Log, TEXT("Transforming level '%s' (%d actors) - scanning for actors to convert to Instanced Actors"),
		*InLevel->GetName(), InLevel->Actors.Num());

	// Bucket matching actors by class -> world-space transforms, and remember the source actors to remove.
	TMap<TSubclassOf<AActor>, TArray<FTransform>> InstancedActorData;
	TArray<AActor*> ActorsToDestroy;

	for (AActor* Actor : InLevel->Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		// Match against the configured candidate classes (including subclasses).
		bool bClassMatches = false;
		for (const TSubclassOf<AActor>& CandidateClass : ActorClassesToConvert)
		{
			if (CandidateClass && Actor->IsA(CandidateClass))
			{
				bClassMatches = true;
				break;
			}
		}

		if (!bClassMatches || !ShouldConvertActorToInstanced(Actor))
		{
			continue;
		}

		const FTransform ActorTransform = Actor->GetActorTransform();
		InstancedActorData.FindOrAdd(Actor->GetClass()).Add(ActorTransform);
		ActorsToDestroy.Add(Actor);

		UE_LOG(LogInstancedActorsCellTransformer, Verbose, TEXT("  Converting actor '%s' (class '%s') at %s"),
			*Actor->GetName(), *Actor->GetClass()->GetName(), *ActorTransform.GetLocation().ToString());
	}

	if (InstancedActorData.IsEmpty())
	{
		return;
	}

	const TSubclassOf<AInstancedActorsManager> ManagerClass = GetInstancedActorsManagerClass();
	if (!ManagerClass)
	{
		UE_LOG(LogInstancedActorsCellTransformer, Warning, TEXT("  GetInstancedActorsManagerClass() returned null - skipping conversion in level '%s'"), *InLevel->GetName());
		return;
	}

	// The manager needs to be placed and sized against the streaming cell being generated.
	const IWorldPartitionCell* WorldPartitionCell = InLevel->GetWorldPartitionRuntimeCell();
	if (!WorldPartitionCell)
	{
		UE_LOG(LogInstancedActorsCellTransformer, Warning, TEXT("  Level '%s' has no World Partition runtime cell (not a partitioned cook?) - skipping conversion"), *InLevel->GetName());
		return;
	}

	const uint32 ManagerGridSize = ManagerClass.GetDefaultObject()->GetDefaultGridSize(World);

	const FString DesiredName = FString::Printf(TEXT("%s_%s"), *ManagerClass->GetName(), *WorldPartitionCell->GetDebugName());
	const FName UniqueManagerName = MakeUniqueObjectName(InLevel, ManagerClass, FName(DesiredName));

	AInstancedActorsManager* NewManager = NewObject<AInstancedActorsManager>(InLevel, ManagerClass, UniqueManagerName);
	NewManager->SetGridSize(ManagerGridSize);
	NewManager->SetLockLocation(true);

	const FBox CellBounds = WorldPartitionCell->GetCellBounds();
	NewManager->SetActorLocation(CellBounds.GetCenter());

	// Populate the manager: one InstancedActorsData per source class, one instance per source actor transform.
	int32 TotalInstances = 0;
	for (const TPair<TSubclassOf<AActor>, TArray<FTransform>>& ClassToTransforms : InstancedActorData)
	{
		UInstancedActorsData& InstanceData = NewManager->GetOrCreateActorInstanceData(ClassToTransforms.Key, /*AdditionalInstanceTags*/ {}, /*bCreateEditorPreviewISMCs*/ false);
		for (const FTransform& InstanceTransform : ClassToTransforms.Value)
		{
			InstanceData.AddInstance(InstanceTransform, /*bWorldSpace*/ true);
		}

		TotalInstances += ClassToTransforms.Value.Num();
		UE_LOG(LogInstancedActorsCellTransformer, Verbose, TEXT("  Added %d instance(s) of class '%s'"),
			ClassToTransforms.Value.Num(), *ClassToTransforms.Key->GetName());
	}

	InLevel->Actors.AddUnique(NewManager);

	// Remove the now-instanced source actors.
	for (AActor* Actor : ActorsToDestroy)
	{
		if (IsValid(Actor))
		{
			World->DestroyActor(Actor);
		}
	}

	// In PIE, PostLoad() won't be called for these newly created Manager/IADs, so set them up manually.
	if (World->WorldType == EWorldType::PIE)
	{
		NewManager->SetupLoadedInstances();
		for (UInstancedActorsData* InstanceData : NewManager->GetAllInstanceData())
		{
			if (InstanceData)
			{
				InstanceData->SetupLoadedInstances();
			}
		}
	}

	UE_LOG(LogInstancedActorsCellTransformer, Log, TEXT("Converted %d instance(s) across %d class(es) into '%s' for cell '%s'"),
		TotalInstances, InstancedActorData.Num(), *NewManager->GetName(), *WorldPartitionCell->GetDebugName());
}

#endif // WITH_EDITOR
