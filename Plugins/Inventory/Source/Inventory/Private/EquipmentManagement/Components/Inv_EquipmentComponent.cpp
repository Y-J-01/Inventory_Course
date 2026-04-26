// Fill out your copyright notice in the Description page of Project Settings.


#include "EquipmentManagement/Components/Inv_EquipmentComponent.h"

#include "EquipmentManagement/EquipActor/Inv_EquipActor.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "InventoryManagment/Utils/Inv_InventoryStatics.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Items/Manifest/Inv_ItemManifest.h"


void UInv_EquipmentComponent::SetOwningSkeletalMesh(USkeletalMeshComponent* OwningMesh)
{
	OwningSkeletalMesh = OwningMesh;
}

void UInv_EquipmentComponent::InitializeOwner(APlayerController* PlayerController)
{
	if (IsValid(PlayerController))
	{
		OwningPlayerController = PlayerController;
	}
	InitInventoryComponent();
}

void UInv_EquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	InitPlayerController();
}

void UInv_EquipmentComponent::OnItemEquipped(UInv_InventoryItem* EquippedItem)
{
	if (!IsValid(EquippedItem)) return;
	if (!OwningPlayerController.IsValid()) return;
	if (!OwningPlayerController->HasAuthority()) return;
	
	FInv_ItemManifest& ItemManifest = EquippedItem->GetItemManifestMutable();

	FInv_EquipmentFragment* EquipmentFragment =
		ItemManifest.GetFragmentOfTypeMutable<FInv_EquipmentFragment>();

	if (!EquipmentFragment)
	{
		return;
	}

	if (!bIsProxy)
	{
		EquipmentFragment->OnEquip(OwningPlayerController.Get());
	}
	
	if (!OwningSkeletalMesh.IsValid())
	{
		return;
	}

	AInv_EquipActor* SpawnedEquipActor =
		SpawnEquippedActor(EquipmentFragment, ItemManifest, OwningSkeletalMesh.Get());

	if (IsValid(SpawnedEquipActor))
	{
		EquippedActors.Add(SpawnedEquipActor);
	}
}

void UInv_EquipmentComponent::OnItemUnequipped(UInv_InventoryItem* UnequippedItem)
{
	if (!IsValid(UnequippedItem)) return;
	if (!OwningPlayerController->HasAuthority()) return;
	
	FInv_ItemManifest& ItemManifest = UnequippedItem->GetItemManifestMutable();
	FInv_EquipmentFragment* EquipmentFragment = ItemManifest.GetFragmentOfTypeMutable<FInv_EquipmentFragment>();
	if (!EquipmentFragment) return;
	
	if (!bIsProxy)
	{
		EquipmentFragment->OnUnEquip(OwningPlayerController.Get());
	}
	
	RemoveEquippedActor(EquipmentFragment->GetEquipmentType());
}

void UInv_EquipmentComponent::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(OwningPlayerController->GetPawn()); IsValid(OwnerCharacter))
	{
		OwningSkeletalMesh = OwnerCharacter->GetMesh();
	}
	InitInventoryComponent();
}

void UInv_EquipmentComponent::InitInventoryComponent()
{
	InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(OwningPlayerController.Get());
	if (!InventoryComponent.IsValid()) return;
	
	if (!InventoryComponent->OnItemEquipped.IsAlreadyBound(this, &ThisClass::OnItemEquipped))
	{
		InventoryComponent->OnItemEquipped.AddDynamic(this, &ThisClass::OnItemEquipped);
	}
	
	if (!InventoryComponent->OnItemUnEquipped.IsAlreadyBound(this, &ThisClass::OnItemUnequipped))
	{
		InventoryComponent->OnItemUnEquipped.AddDynamic(this, &ThisClass::OnItemUnequipped);
	}
	
	
}

AInv_EquipActor* UInv_EquipmentComponent::SpawnEquippedActor(
	FInv_EquipmentFragment* EquipmentFragment,
	const FInv_ItemManifest& Manifest,
	USkeletalMeshComponent* AttachMesh)
{
	
	if (!EquipmentFragment)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnEquippedActor: EquipmentFragment NULL"));
		return nullptr;
	}

	if (!IsValid(AttachMesh))
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnEquippedActor: AttachMesh INVALID"));
		return nullptr;
	}

	UE_LOG(LogTemp, Warning, TEXT("SpawnEquippedActor: AttachMesh=%s Owner=%s EquipmentType=%s"),
	*GetNameSafe(AttachMesh),
	*GetNameSafe(AttachMesh->GetOwner()),
	*EquipmentFragment->GetEquipmentType().ToString());

	AInv_EquipActor* SpawnedEquipActor = EquipmentFragment->SpawnAttachedActor(AttachMesh);

	if (!IsValid(SpawnedEquipActor))
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnEquippedActor: SpawnAttachedActor returned NULL"));
		return nullptr;
	}

	UE_LOG(LogTemp, Warning, TEXT("SpawnEquippedActor: Spawned actor=%s Location=%s Hidden=%d"),
		*GetNameSafe(SpawnedEquipActor),
		*SpawnedEquipActor->GetActorLocation().ToString(),
		SpawnedEquipActor->IsHidden() ? 1 : 0);

	SpawnedEquipActor->SetEquipmentType(EquipmentFragment->GetEquipmentType());
	SpawnedEquipActor->SetOwner(GetOwner());
	EquipmentFragment->SetEquippedActor(SpawnedEquipActor);

	return SpawnedEquipActor;
}

AInv_EquipActor* UInv_EquipmentComponent::FindEquippedActor(const FGameplayTag& EquipmentTypeTag)
{
	auto FoundActor = EquippedActors.FindByPredicate([&EquipmentTypeTag](const AInv_EquipActor* EquippedActor)
	{
		return EquippedActor->GetEquipmentType().MatchesTagExact(EquipmentTypeTag);
	});
	return FoundActor ? *FoundActor : nullptr;
}

void UInv_EquipmentComponent::RemoveEquippedActor(const FGameplayTag& EquippedTag)
{
	if (AInv_EquipActor* EquippedActor = FindEquippedActor(EquippedTag); IsValid(EquippedActor))
	{
		EquippedActors.Remove(EquippedActor);
		EquippedActor->Destroy();
	}
}

void UInv_EquipmentComponent::InitPlayerController()
{
	if (OwningPlayerController = Cast<APlayerController>(GetOwner()); OwningPlayerController.IsValid())
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(OwningPlayerController->GetPawn()); IsValid(OwnerCharacter))
		{
			OnPossessedPawnChanged(nullptr, OwnerCharacter);
		}
		else
		{
			OwningPlayerController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::OnPossessedPawnChanged);
		}
	}
}



