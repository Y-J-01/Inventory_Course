// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Inventory/GridSlots/Inv_EquippedGridSlot.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "InventoryManagment/Utils/Inv_InventoryStatics.h"
#include "Items/Fragments/Inv_FragmentTags.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Widgets/Inventory/HoverItem/Inv_HoverItem.h"
#include "Widgets/Inventory/SlottedItems/Inv_EquippedSlottedItem.h"
#include "Components/OverlaySlot.h"

void UInv_EquippedGridSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!IsAvailable()) return;
	UInv_HoverItem* HoverItem = UInv_InventoryStatics::GetHoverItem(GetOwningPlayer());
	if (!IsValid(HoverItem)) return;
	
	if (HoverItem->GetItemType().MatchesTag(EquippedTypeTag))
	{
		SetOccupiedTexture();
		Image_EquippedOutIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UInv_EquippedGridSlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	if (!IsAvailable()) return;
	UInv_HoverItem* HoverItem = UInv_InventoryStatics::GetHoverItem(GetOwningPlayer());
	if (!IsValid(HoverItem)) return;
	
	if (IsValid(EquippedSlottedItem)) return;
	
	if (HoverItem->GetItemType().MatchesTag(EquippedTypeTag))
	{
		SetUnoccupideTexture();
		Image_EquippedOutIcon->SetVisibility(ESlateVisibility::Visible);
	}
}

FReply UInv_EquippedGridSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	EquippedGridSlotClicked.Broadcast(this, EquippedTypeTag);
	return FReply::Handled();
}

UInv_EquippedSlottedItem* UInv_EquippedGridSlot::OnItemEquipped(UInv_InventoryItem* Item,
	const FGameplayTag& EquipmentTag, float TileSize)
{
	if (!EquipmentTag.MatchesTagExact(EquippedTypeTag)) return nullptr;
	
	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(Item, FragmentTags::GridFragment);
	if (!GridFragment) return nullptr;
	const FIntPoint GridDimensions = GridFragment->GetGridSize();
	
	const float IconTileWidth = TileSize - GridFragment->GetGridPadding() * 2.0f;
	const FVector2D DrawSize = GridDimensions * IconTileWidth;
	
	EquippedSlottedItem = CreateWidget<UInv_EquippedSlottedItem>(GetOwningPlayer(), EquippedSlottedItemClass);
	
	EquippedSlottedItem->SetInventoryItem(Item);
	
	EquippedSlottedItem->SetEquipmentTypeTag(EquipmentTag);
	
	EquippedSlottedItem->UpdateStackCount(0);
	
	SetInventoryItem(Item);
	
	const FInv_ImageFragment* ImageFragment = GetFragment<FInv_ImageFragment>(Item, FragmentTags::IconFragment);
	if (!ImageFragment) return nullptr;
	
	FSlateBrush Brush;
	Brush.SetResourceObject(ImageFragment->GetIcon());
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = DrawSize;
	
	EquippedSlottedItem->SetImageBrush(Brush);
	
	Overlay_Root->AddChildToOverlay(EquippedSlottedItem);
	FGeometry OverlayGeometry = Overlay_Root->GetCachedGeometry();
	auto OverlayPos = OverlayGeometry.Position;
	auto OverlaySize = OverlayGeometry.Size;
	
	const float LeftPadding = OverlaySize.X / 2.0f - DrawSize.X / 2.0f;
	const float TopPadding = OverlaySize.Y /2.0f - DrawSize.Y / 2.0f;
	
	UOverlaySlot* OverlaySlot = UWidgetLayoutLibrary::SlotAsOverlaySlot(EquippedSlottedItem);
	OverlaySlot->SetPadding(FMargin(LeftPadding, TopPadding));
	
	
	return EquippedSlottedItem;
}
