// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Inventory/GridSlots/Inv_EquippedGridSlot.h"
#include "Widgets/Inventory/InventoryBase/Inv_InventoryBase.h"
#include "Widgets/ItemDescription/Inv_ItemDescription.h"
#include "Inv_SpetialInventory.generated.h"

class UCanvasPanel;
class UInv_InventoryGrid;
class UWidgetSwitcher;
class UButton;
class UInv_HoverItem;
/**
 * 
 */
UCLASS()
class INVENTORY_API UInv_SpetialInventory : public UInv_InventoryBase
{
	GENERATED_BODY()
	
public:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	virtual FInv_SlotAvailabilityResult HasRoomForItem(UInv_ItemComponent* ItemComponent) const override;
	virtual void OnItemHovered(UInv_InventoryItem* Item) override;
	virtual void OnItemUnhovered() override;
	virtual bool HasHoveredItem() const override;
	virtual UInv_HoverItem* GetHoverItem() const override;
	virtual float GetTileSize() const override;
	
private:
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Switcher;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInv_InventoryGrid> Grid_Equippables;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInv_InventoryGrid> Grid_Consumbles;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInv_InventoryGrid> Grid_Craftables;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Equippables;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Consumbles;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Craftables;
	
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInv_ItemDescription> ItemDescriptionClass;
	
	UPROPERTY(EditAnywhere, Category = "Inventory")
	float DescriptionTimerDelay = 0.5f;
	
	UPROPERTY()
	TObjectPtr<UInv_ItemDescription> ItemDescription;
	
	UPROPERTY()
	TArray<TObjectPtr<UInv_EquippedGridSlot>> EquippedGridSlots;
	
	TWeakObjectPtr<UInv_InventoryGrid> ActiveGrid;
	FTimerHandle DescriptionTimer;
	
	
	UFUNCTION()
	void ShowEquippables();
	
	UFUNCTION()
	void ShowConsumbles();
	
	UFUNCTION()
	void ShowCraftables();
	
	UFUNCTION()
	void EquippedGridSlotClicked(UInv_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquippedTypeTag);
	
	UFUNCTION()
	void EquippedSlottedItemClicked(UInv_EquippedSlottedItem* EquippedSlottedItem);
	
	void DisableButton(UButton* Button);
	void SetActiveGrid(UInv_InventoryGrid* Grid, UButton* Button);
	UInv_ItemDescription* GetItemDescription();
	void SetItemDescriptionSizeAndPosition(UInv_ItemDescription* Description, UCanvasPanel* Canvas) const;
	bool CanEquipHoverItem(UInv_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag) const;
	UInv_EquippedGridSlot* FindSlotWithEquippedItem(UInv_InventoryItem* EquippedItem) const;
	void ClearSlotOfItem(UInv_EquippedGridSlot* EquippedGridSlot);
	void RemoveEquippedSlottedItem(UInv_EquippedSlottedItem* EquippedSlottedItem);
	void MakeEquippedSlottedItem(UInv_EquippedSlottedItem* EquippedSlottedItem, UInv_EquippedGridSlot* EquippedGridSlot, UInv_InventoryItem* ItemToEquip);
	void BroadcastSlotClickedDelegates(UInv_InventoryItem* ItemToEquip, UInv_InventoryItem* ItemToUnequip) const;
	
};
