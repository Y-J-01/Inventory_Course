// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inv_EquipmentComponent.generated.h"

struct FGameplayTag;
struct FInv_EquipmentFragment;
class UInv_InventoryComponent;
class UInv_InventoryItem;
class APlayerController;
class USkeletalMeshComponent;
struct FInv_ItemManifest;
class AInv_EquipActor;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORY_API UInv_EquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	void SetOwningSkeletalMesh(USkeletalMeshComponent* OwningMesh);
	void SetIsProxy(bool Proxy) { bIsProxy = Proxy; }
	void InitializeOwner(APlayerController* PlayerController);

protected:
	
	virtual void BeginPlay() override;

private:
	
	TWeakObjectPtr<UInv_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<APlayerController> OwningPlayerController;
	TWeakObjectPtr<USkeletalMeshComponent> OwningSkeletalMesh;
	bool bIsProxy{false};
	
	UFUNCTION()
	void OnItemEquipped(UInv_InventoryItem* EquippedItem);
	
	UFUNCTION()
	void OnItemUnequipped(UInv_InventoryItem* UnequippedItem);
	
	UFUNCTION()
	void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);
	
	void InitInventoryComponent();
	AInv_EquipActor* SpawnEquippedActor(FInv_EquipmentFragment* EquipmentFragment, const FInv_ItemManifest& Manifest, USkeletalMeshComponent* AttachMesh);
	AInv_EquipActor* FindEquippedActor(const FGameplayTag& EquipmentTypeTag);
	void RemoveEquippedActor(const FGameplayTag& EquippedTag);
	void InitPlayerController();
	
	UPROPERTY()
	TArray<TObjectPtr<AInv_EquipActor>> EquippedActors;
	
};
