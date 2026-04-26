// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Inventory/Spetial/Inv_SpetialInventory.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/WidgetSwitcher.h"
#include "InventoryManagment/Utils/Inv_InventoryStatics.h"
#include "Items/Inv_InventoryItem.h"
#include "Widgets/Inventory/Spetial/Inv_InventoryGrid.h"

void UInv_SpetialInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	Button_Equippables->OnClicked.AddDynamic(this, &ThisClass::ShowEquippables);
	Button_Consumbles->OnClicked.AddDynamic(this, &ThisClass::ShowConsumbles);
	Button_Craftables->OnClicked.AddDynamic(this, &ThisClass::ShowCraftables);
	
	Grid_Equippables->SetOwningCanvas(CanvasPanel);
	Grid_Consumbles->SetOwningCanvas(CanvasPanel);
	Grid_Craftables->SetOwningCanvas(CanvasPanel);
	
	ShowEquippables();
	
	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		UInv_EquippedGridSlot* EquippedGridSlot = Cast<UInv_EquippedGridSlot>(Widget);
		if (IsValid(EquippedGridSlot))
		{
			EquippedGridSlots.Add(EquippedGridSlot);
			EquippedGridSlot->EquippedGridSlotClicked.AddDynamic(this, &ThisClass::EquippedGridSlotClicked);
		}
	});
}

FReply UInv_SpetialInventory::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	ActiveGrid->DropItem();
	return FReply::Handled();
}

void UInv_SpetialInventory::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if (!IsValid(ItemDescription)) return;
	
	SetItemDescriptionSizeAndPosition(ItemDescription, CanvasPanel);
	
}

FInv_SlotAvailabilityResult UInv_SpetialInventory::HasRoomForItem(UInv_ItemComponent* ItemComponent) const
{
	switch (UInv_InventoryStatics::GetItemCategoryFromItemComp(ItemComponent))
	{
		case EInv_ItemCategory::Equippable:
			return Grid_Equippables->HasRoomForItem(ItemComponent);
		case EInv_ItemCategory::Consumable:
			return Grid_Consumbles->HasRoomForItem(ItemComponent);
		case EInv_ItemCategory::Craftable:
			return Grid_Craftables->HasRoomForItem(ItemComponent);
		default:
			return FInv_SlotAvailabilityResult();
	}
}

void UInv_SpetialInventory::OnItemHovered(UInv_InventoryItem* Item)
{
	const auto& Manifest = Item->GetItemManifest();
	
	UInv_ItemDescription* DescriptionWidget = GetItemDescription();
	DescriptionWidget->SetVisibility(ESlateVisibility::Collapsed);
	
	GetOwningPlayer()->GetWorldTimerManager().ClearTimer(DescriptionTimer);
	
	FTimerDelegate DescriptionTimerDelegate;
	DescriptionTimerDelegate.BindLambda([this, &Manifest, DescriptionWidget]()
	{
		GetItemDescription()->SetVisibility(ESlateVisibility::HitTestInvisible);
		Manifest.AssimilateInventoryFragments(DescriptionWidget);
	});
		
	GetOwningPlayer()->GetWorldTimerManager().SetTimer(DescriptionTimer, DescriptionTimerDelegate, DescriptionTimerDelay, false);
}

void UInv_SpetialInventory::OnItemUnhovered()
{
	GetItemDescription()->SetVisibility(ESlateVisibility::Collapsed);
	GetOwningPlayer()->GetWorldTimerManager().ClearTimer(DescriptionTimer);
}

bool UInv_SpetialInventory::HasHoveredItem() const
{
	if (Grid_Equippables->HasHoverItem()) return true;
	if (Grid_Consumbles->HasHoverItem()) return true;
	if (Grid_Craftables->HasHoverItem()) return true;
	return false;
	
}

UInv_HoverItem* UInv_SpetialInventory::GetHoverItem() const
{
	if (!ActiveGrid.IsValid()) return nullptr;
	
	return ActiveGrid->GetHoverItem();
}

float UInv_SpetialInventory::GetTileSize() const
{
	return Grid_Equippables->GetTileSize();
}

void UInv_SpetialInventory::ShowEquippables()
{
	SetActiveGrid(Grid_Equippables, Button_Equippables);
}

void UInv_SpetialInventory::ShowConsumbles()
{
	SetActiveGrid(Grid_Consumbles, Button_Consumbles);
}

void UInv_SpetialInventory::ShowCraftables()
{
	SetActiveGrid(Grid_Craftables, Button_Craftables);
}

void UInv_SpetialInventory::EquippedGridSlotClicked(UInv_EquippedGridSlot* EquippedGridSlot,
	const FGameplayTag& EquippedTypeTag)
{
	if (!CanEquipHoverItem(EquippedGridSlot, EquippedTypeTag)) return;
	
	UInv_HoverItem* HoverItem = GetHoverItem();
	
	const float TileSize = UInv_InventoryStatics::GetInventoryWidget(GetOwningPlayer())->GetTileSize();
	UInv_EquippedSlottedItem* EquippedSlottedItem = EquippedGridSlot->OnItemEquipped(
	HoverItem->GetInventoryItem(),
	EquippedTypeTag,
	TileSize
	);
	EquippedSlottedItem->OnEquippedSlottedItemClicked.AddDynamic(this, &ThisClass::EquippedSlottedItemClicked);
	
	
	UInv_InventoryComponent* InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	check(IsValid(InventoryComponent));
	
	InventoryComponent->Server_EquipSlotClicked(HoverItem->GetInventoryItem(), nullptr);
	
	if (GetOwningPlayer()->GetNetMode() != NM_DedicatedServer)
	{
		InventoryComponent->OnItemEquipped.Broadcast(HoverItem->GetInventoryItem());
	}
	
	Grid_Equippables->ClearHoverItem();
	
}

void UInv_SpetialInventory::EquippedSlottedItemClicked(UInv_EquippedSlottedItem* EquippedSlottedItem)
{
	UInv_InventoryStatics::ItemUnhovered(GetOwningPlayer());
	
	if (IsValid(GetHoverItem()) && GetHoverItem()->IsStackable()) return;
	
	UInv_InventoryItem* ItemToEquip = IsValid(GetHoverItem()) ? GetHoverItem()->GetInventoryItem() : nullptr;
	
	UInv_InventoryItem* ItemToUnequip = EquippedSlottedItem->GetInventoryItem();
	
	UInv_EquippedGridSlot* EquippedGridSlot = FindSlotWithEquippedItem(ItemToUnequip);
	
	ClearSlotOfItem(EquippedGridSlot);
	
	Grid_Equippables->AssignHoverItem(ItemToUnequip);
	
	RemoveEquippedSlottedItem(EquippedSlottedItem);
	
	MakeEquippedSlottedItem(EquippedSlottedItem, EquippedGridSlot, ItemToEquip);
	
	BroadcastSlotClickedDelegates(ItemToEquip, ItemToUnequip);
}

void UInv_SpetialInventory::DisableButton(UButton* Button)
{
	Button_Equippables->SetIsEnabled(true);
	Button_Consumbles->SetIsEnabled(true);
	Button_Craftables->SetIsEnabled(true);
	Button->SetIsEnabled(false);
}

void UInv_SpetialInventory::SetActiveGrid(UInv_InventoryGrid* Grid, UButton* Button)
{
	if (ActiveGrid.IsValid())
	{
		ActiveGrid->HideCursor();
		ActiveGrid->OnHide();
	}
	ActiveGrid = Grid;
	if (ActiveGrid.IsValid()) ActiveGrid->ShowCursor();
	DisableButton(Button);
	Switcher->SetActiveWidget(Grid);
}

UInv_ItemDescription* UInv_SpetialInventory::GetItemDescription()
{
	if (!IsValid(ItemDescription))
	{
		ItemDescription = CreateWidget<UInv_ItemDescription>(GetOwningPlayer(), ItemDescriptionClass);
		CanvasPanel->AddChild(ItemDescription);
	}
	return ItemDescription;
}

void UInv_SpetialInventory::SetItemDescriptionSizeAndPosition(UInv_ItemDescription* Description,
	UCanvasPanel* Canvas) const
{
	UCanvasPanelSlot* ItemDescriptionCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(Description);
	if (!IsValid(ItemDescriptionCPS)) return;
	
	const FVector2D ItemDescriptionSize = Description->GetBoxSize();
	ItemDescriptionCPS->SetSize(ItemDescriptionSize);
	
	FVector2D ClampedPosition = UInv_WidgetUtils::GetClampedWidgetPosition(
		UInv_WidgetUtils::GetWidgetSize(Canvas),
		ItemDescriptionSize,
		UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer()));
	
	ItemDescriptionCPS->SetPosition(ClampedPosition); 
}

bool UInv_SpetialInventory::CanEquipHoverItem(UInv_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag) const
{
	if (!IsValid(EquippedGridSlot) || EquippedGridSlot->GetInventoryItem().IsValid()) return false;
	
	UInv_HoverItem* HoverItem = GetHoverItem();
	if (!IsValid(HoverItem)) return false;
	
	UInv_InventoryItem* Helditem = HoverItem->GetInventoryItem();
	
	return HasHoveredItem() && IsValid(Helditem) &&
		!HoverItem->IsStackable() &&
			Helditem->GetItemManifest().GetItemCategory() == EInv_ItemCategory::Equippable &&
				Helditem->GetItemManifest().GetItemType().MatchesTag(EquipmentTypeTag);
}

UInv_EquippedGridSlot* UInv_SpetialInventory::FindSlotWithEquippedItem(UInv_InventoryItem* EquippedItem) const
{
	auto* FoundEquippedGridSlot = EquippedGridSlots.FindByPredicate([EquippedItem](UInv_EquippedGridSlot* Slot)
	{
		return Slot->GetInventoryItem() == EquippedItem;
	});
	return FoundEquippedGridSlot ? *FoundEquippedGridSlot : nullptr;
}

void UInv_SpetialInventory::ClearSlotOfItem(UInv_EquippedGridSlot* EquippedGridSlot)
{
	if (IsValid(EquippedGridSlot))
	{
		EquippedGridSlot->SetEquippedSlottedItem(nullptr);
		EquippedGridSlot->SetInventoryItem(nullptr);
	}
}

void UInv_SpetialInventory::RemoveEquippedSlottedItem(UInv_EquippedSlottedItem* EquippedSlottedItem)
{
	if (!IsValid(EquippedSlottedItem)) return;
	
	if (EquippedSlottedItem->OnEquippedSlottedItemClicked.IsAlreadyBound(this, &ThisClass::EquippedSlottedItemClicked))
	{
		EquippedSlottedItem->OnEquippedSlottedItemClicked.RemoveDynamic(this, &ThisClass::EquippedSlottedItemClicked);
	}
	EquippedSlottedItem->RemoveFromParent();
}

void UInv_SpetialInventory::MakeEquippedSlottedItem(UInv_EquippedSlottedItem* EquippedSlottedItem,
	UInv_EquippedGridSlot* EquippedGridSlot, UInv_InventoryItem* ItemToEquip)
{
	if (!IsValid(EquippedGridSlot)) return;
	
	UInv_EquippedSlottedItem* SlottedItem = EquippedGridSlot->OnItemEquipped(
		ItemToEquip,
		EquippedSlottedItem->GetEquipmentTypeTag(),
		UInv_InventoryStatics::GetInventoryWidget(GetOwningPlayer())->GetTileSize());
	if (IsValid(SlottedItem)) SlottedItem->OnEquippedSlottedItemClicked.AddDynamic(this, &ThisClass::EquippedSlottedItemClicked);
	
	EquippedGridSlot->SetEquippedSlottedItem(SlottedItem);
}

void UInv_SpetialInventory::BroadcastSlotClickedDelegates(UInv_InventoryItem* ItemToEquip,
	UInv_InventoryItem* ItemToUnequip) const
{
	UInv_InventoryComponent* InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	check(IsValid(InventoryComponent));
	InventoryComponent->Server_EquipSlotClicked(ItemToEquip, ItemToUnequip);
	
	if (GetOwningPlayer()->GetNetMode() != NM_DedicatedServer)
	{
		InventoryComponent->OnItemEquipped.Broadcast(ItemToEquip);
		InventoryComponent->OnItemUnEquipped.Broadcast(ItemToUnequip);
	}
}


