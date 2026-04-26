// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inv_SlottedItem.h"
#include "GameplayTagContainer.h"
#include "Inv_EquippedSlottedItem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEquippedSlottedItemClicked, class UInv_EquippedSlottedItem*, SlottedItem);

UCLASS()
class INVENTORY_API UInv_EquippedSlottedItem : public UInv_SlottedItem
{
	GENERATED_BODY()
	
public:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	void SetEquipmentTypeTag(const FGameplayTag& Tag) { EquippedTypeTag = Tag; }
	FGameplayTag GetEquipmentTypeTag() const { return EquippedTypeTag; }
	
	FEquippedSlottedItemClicked OnEquippedSlottedItemClicked;
	
private:
	
	UPROPERTY()
	FGameplayTag EquippedTypeTag;
};
