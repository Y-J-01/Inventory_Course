// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Inventory/Spetial/Inv_InventoryGrid.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "InventoryManagment/Utils/Inv_InventoryStatics.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Items/Fragments/Inv_FragmentTags.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Items/Manifest/Inv_ItemManifest.h"
#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"
#include "Widgets/Inventory/SlottedItems/Inv_SlottedItem.h"
#include "Widgets/Utils/Inv_WidgetUtils.h"
#include "Widgets/ItemPopUp/Inv_ItemPopUp.h"



void UInv_InventoryGrid::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	ConstructGrid();
	
	InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());

	if (!InventoryComponent.IsValid())
	{
		return;
	}

	InventoryComponent->OnItemAdded.AddDynamic(this, &ThisClass::AddItem);
	InventoryComponent->OnStackChange.AddDynamic(this, &ThisClass::AddStacks);
	InventoryComponent->OnInventoryMenuToggled.AddDynamic(this, &ThisClass::OnInventoryMenuToggled);
}

void UInv_InventoryGrid::NativeTick(const FGeometry& Geometry, float InDeltaTime)
{
	Super::NativeTick(Geometry, InDeltaTime);
	
	const FVector2D CanvasPosition = UInv_WidgetUtils::GetWidgetPosition(CanvasPanel);
	const FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());
	
	if (CursorExitedCanvas(CanvasPosition, UInv_WidgetUtils::GetWidgetSize(CanvasPanel), MousePosition))
	{
		return;
	}
	
	UpdateTileParameters(CanvasPosition, MousePosition);
}

FInv_SlotAvailabilityResult UInv_InventoryGrid::HasRoomForItem(const UInv_ItemComponent* ItemComponent)
{
	if (!IsValid(ItemComponent))
	{
		UE_LOG(LogTemp, Error, TEXT("HasRoomForItem(ItemComponent): ItemComponent INVALID"));
		return FInv_SlotAvailabilityResult{};
	}

	return HasRoomForItem(ItemComponent->GetItemManifest());
}

FInv_SlotAvailabilityResult UInv_InventoryGrid::HasRoomForItem(const UInv_InventoryItem* Item, const int32 StackAmountOverride)
{
	if (!IsValid(Item))
	{
		return FInv_SlotAvailabilityResult{};
	}

	return HasRoomForItem(Item->GetItemManifest(), StackAmountOverride);
}

FInv_SlotAvailabilityResult UInv_InventoryGrid::HasRoomForItem(const FInv_ItemManifest& Manifest, const int32 StackAmountOverride)
{
	FInv_SlotAvailabilityResult Result;
	
	const FInv_StackableFragment* StackableFragment = Manifest.GetFragmentOfType<FInv_StackableFragment>();
	Result.bStackable = StackableFragment != nullptr;
	
	const int32 MaxStackSize = StackableFragment ? StackableFragment->GetMaxStackSize() : 1;
	int32 AmountToFill = StackableFragment ? StackableFragment->GetStackCount() : 1;
	if (StackAmountOverride != -1 && Result.bStackable)
	{
		AmountToFill = StackAmountOverride;
	}
	
	TSet<int32> CheckedIndices;
	
	for (const auto& GridSlot : GridSlots)
	{
		if (AmountToFill == 0) break;
		
		if (IsIndexClaimed(CheckedIndices, GridSlot->GetIndex())) continue;
		
		if (!IsInGridBounds(GridSlot->GetIndex(), GetItemDimensions(Manifest))) continue;
		
		TSet<int32> TentativelyClaimed;
		if (!HasRoomAtIndex(GridSlot, GetItemDimensions(Manifest), CheckedIndices, TentativelyClaimed, Manifest.GetItemTag(), MaxStackSize))
		{
			continue;
		}
		
		const int32 AmountToFillInSlot = DetermineFillAmountForSlot(Result.bStackable, MaxStackSize, AmountToFill, GridSlot);
		if (AmountToFillInSlot == 0) continue;
		
		CheckedIndices.Append(TentativelyClaimed);
		
		Result.TotalRoomToFill += AmountToFillInSlot;
		Result.SlotAvailabilities.Emplace(
		FInv_SlotAvailability{
			HasValidItem(GridSlot) ? GridSlot->GetUpperLeftIndex() : GridSlot->GetIndex(),
			Result.bStackable ? AmountToFillInSlot : 0,
			HasValidItem(GridSlot)
			}
		);
		
		AmountToFill -= AmountToFillInSlot;
		Result.Remainder = AmountToFill;
		if (AmountToFill == 0) return Result;
		
	}
		return Result;
}

bool UInv_InventoryGrid::HasRoomAtIndex(const UInv_GridSlot* GridSlot,
										const FIntPoint& Dimensions,
										const TSet<int32>& CheckedIndices,
										TSet<int32>& OutTentativelyClaimed,
										const FGameplayTag& ItemType,
										const int32 MaxStackSize)
{
	bool bHasRoomAtIndex = true;
	UInv_InventoryStatics::ForEach2D(GridSlots, GridSlot->GetIndex(), Dimensions, Columns, [&](const UInv_GridSlot* SubGridSlot)
	{
		if (CheckSlotConstraints(GridSlot, SubGridSlot, CheckedIndices, OutTentativelyClaimed, ItemType, MaxStackSize))
		{
			OutTentativelyClaimed.Add(SubGridSlot->GetIndex());
		}
		else
		{
			{
				bHasRoomAtIndex = false;
			}
		}
	});
	
	return bHasRoomAtIndex;
}

FIntPoint UInv_InventoryGrid::GetItemDimensions(const FInv_ItemManifest& Manifest) const
{
	const FInv_GridFragment* GridFragment = Manifest.GetFragmentOfType<FInv_GridFragment>();
	return GridFragment ? GridFragment->GetGridSize() : FIntPoint(1,1);
		
}

bool UInv_InventoryGrid::CheckSlotConstraints(const UInv_GridSlot* GridSlot,
												const UInv_GridSlot* SubGridSlot,
												const TSet<int32>& CheckedIndices,
												TSet<int32>& OutTentativelyClaimed,
												const FGameplayTag& ItemType,
												const int32 MaxStackSize) const
											{
												if (IsIndexClaimed(CheckedIndices, SubGridSlot->GetIndex())) return false;
												
												if (!HasValidItem(SubGridSlot))
												{
													OutTentativelyClaimed.Add(SubGridSlot->GetIndex());
													return true;
												}
												
												if (!IsUpperLeftSlot(SubGridSlot, SubGridSlot)) return false;
												
												const UInv_InventoryItem* SubItem = SubGridSlot->GetInventoryItem().Get();
												if (!SubItem->IsStackable()) return false;
												
												if (!DoesItemTypeMatch(SubItem, ItemType)) return false;
												
												if (GridSlot->GetStackCount() >= MaxStackSize) return false;
												
												return true;
											}

bool UInv_InventoryGrid::HasValidItem(const UInv_GridSlot* GridSlot) const
{
	return GridSlot->GetInventoryItem().IsValid();
}

bool UInv_InventoryGrid::IsUpperLeftSlot(const UInv_GridSlot* GridSlot, const UInv_GridSlot* SubGridSlot) const
{
	return SubGridSlot->GetUpperLeftIndex() == GridSlot->GetIndex();
}

bool UInv_InventoryGrid::DoesItemTypeMatch(const UInv_InventoryItem* SubItem, const FGameplayTag& ItemType) const
{
	return SubItem->GetItemManifest().GetItemTag().MatchesTagExact(ItemType);
}

bool UInv_InventoryGrid::IsInGridBounds(const int32 StartIndex, const FIntPoint& ItemDimensions) const
{
	if (StartIndex < 0 || StartIndex >= GridSlots.Num()) return false;
	const int32 EndColum = (StartIndex % Columns) + ItemDimensions.X;
	const int32 EndRow = (StartIndex / Columns) + ItemDimensions.Y;
	return EndColum <= Columns && EndRow <= Rows;
}

int32 UInv_InventoryGrid::DetermineFillAmountForSlot(const bool bStackable, const int32 MaxStackSize,
	const int32 AmountToFill, const UInv_GridSlot* GridSlot) const
{
	const int32 RoomInSlot = MaxStackSize - GetStackAmount(GridSlot);
	return bStackable ? FMath::Min(AmountToFill, RoomInSlot) : 1;
}

int32 UInv_InventoryGrid::GetStackAmount(const UInv_GridSlot* GridSlot) const
{
	int32 CurrentSlotStackCount = GridSlot->GetStackCount();
	if (const int32 UpperLeftIndex = GridSlot->GetUpperLeftIndex(); UpperLeftIndex != INDEX_NONE)
	{
		UInv_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
		CurrentSlotStackCount = UpperLeftGridSlot->GetStackCount();
	}
	return CurrentSlotStackCount;
}

bool UInv_InventoryGrid::IsRightClick(const FPointerEvent& MouseEvent) const
{
	return MouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
}

bool UInv_InventoryGrid::IsLeftClick(const FPointerEvent& MouseEvent) const
{
	return MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
}

void UInv_InventoryGrid::PickUp(UInv_InventoryItem* ClickedInventoryItem, const int32 GridIndex)
{
	if (!IsValid(ClickedInventoryItem) || !GridSlots.IsValidIndex(GridIndex))
	{
		return;
	}

	ClearItemPopUp();

	const int32 UpperLeftIndex = GridSlots[GridIndex]->GetUpperLeftIndex() != INDEX_NONE
		? GridSlots[GridIndex]->GetUpperLeftIndex()
		: GridIndex;

	if (!GridSlots.IsValidIndex(UpperLeftIndex))
	{
		return;
	}

	AssignHoverItem(ClickedInventoryItem, UpperLeftIndex, UpperLeftIndex);
	RemoveItemFromGrid(ClickedInventoryItem, UpperLeftIndex);
}

void UInv_InventoryGrid::AssignHoverItem(UInv_InventoryItem* InventoryItem, const int32 GridIndex,
	const int32 PreviousGridIndex)
{
	if (!IsValid(InventoryItem) || !GridSlots.IsValidIndex(GridIndex))
	{
		return;
	}

	AssignHoverItem(InventoryItem);

	if (!IsValid(HoverItem))
	{
		return;
	}

	HoverItem->SetPreviousGridIndex(PreviousGridIndex);

	const int32 StackCount = InventoryItem->IsStackable()
		? GridSlots[GridIndex]->GetStackCount()
		: 0;

	HoverItem->UpdateStackCount(StackCount);
}

void UInv_InventoryGrid::RemoveItemFromGrid(UInv_InventoryItem* InventoryItem, const int32 GridIndex)
{
	ClearItemPopUp();

	if (!IsValid(InventoryItem) || !GridSlots.IsValidIndex(GridIndex))
	{
		return;
	}

	const int32 UpperLeftIndex = GridSlots[GridIndex]->GetUpperLeftIndex() != INDEX_NONE
		? GridSlots[GridIndex]->GetUpperLeftIndex()
		: GridIndex;

	if (!GridSlots.IsValidIndex(UpperLeftIndex))
	{
		return;
	}

	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(InventoryItem, FragmentTags::GridFragment);
	if (!GridFragment)
	{
		return;
	}

	UInv_InventoryStatics::ForEach2D(GridSlots, UpperLeftIndex, GridFragment->GetGridSize(), Columns,
		[&](UInv_GridSlot* GridSlot)
		{
			if (!IsValid(GridSlot)) return;

			GridSlot->SetInventoryItem(nullptr);
			GridSlot->SetUpperLeftIndex(INDEX_NONE);
			GridSlot->SetUnoccupideTexture();
			GridSlot->SetAvailable(true);
			GridSlot->SetStackCount(0);
		});

	if (TObjectPtr<UInv_SlottedItem>* FoundSlottedItem = SlottedItems.Find(UpperLeftIndex))
	{
		if (IsValid(FoundSlottedItem->Get()))
		{
			FoundSlottedItem->Get()->RemoveFromParent();
		}

		SlottedItems.Remove(UpperLeftIndex);
	}
}

void UInv_InventoryGrid::UpdateTileParameters(const FVector2D CanvasPosition, const FVector2D MousePosition)
{
	if (!bMouseWithinCanvas) return;
	
	const FIntPoint HoveredTileCoordinates = CalculateHoveredCoordinates(CanvasPosition, MousePosition);
	
	LastTileParameters = TileParameters;
	TileParameters.TileCoordinats = HoveredTileCoordinates;
	TileParameters.TileIndex = UInv_WidgetUtils::GetIndexFromPosition(HoveredTileCoordinates, Columns);
	TileParameters.TileQuadrant = CalculateTileQuadrant(CanvasPosition, MousePosition);
	
	OnTileParametersUpdated(TileParameters);
	
	
}

FIntPoint UInv_InventoryGrid::CalculateHoveredCoordinates(const FVector2D& CanvasPosition,
	const FVector2D MousePosition) const
{
	return FIntPoint{
		static_cast<int32>(FMath::FloorToInt((MousePosition.X - CanvasPosition.X) / TileSize)),
		static_cast<int32>(FMath::FloorToInt((MousePosition.Y - CanvasPosition.Y) / TileSize))
	};
}

EInv_TileQuadrant UInv_InventoryGrid::CalculateTileQuadrant(const FVector2D& CanvasPosition,
	const FVector2D MousePosition) const
{
	const float TileLocalX = FMath::Fmod(MousePosition.X - CanvasPosition.X, TileSize);
	const float TileLocalY = FMath::Fmod(MousePosition.Y - CanvasPosition.Y, TileSize);
	
	const bool bIsTop = TileLocalY < TileSize / 2.f;
	const bool bIsLeft = TileLocalX < TileSize / 2.f;
	
	EInv_TileQuadrant HoveredTileQuadrant{EInv_TileQuadrant::None};
	if (bIsTop && bIsLeft) HoveredTileQuadrant = EInv_TileQuadrant::TopLeft;
	else if (bIsTop && !bIsLeft) HoveredTileQuadrant = EInv_TileQuadrant::TopRight;
	else if (!bIsTop && bIsLeft) HoveredTileQuadrant = EInv_TileQuadrant::BottomLeft;
	else if (!bIsTop && !bIsLeft) HoveredTileQuadrant = EInv_TileQuadrant::BottomRight;
	
	return HoveredTileQuadrant;
}

void UInv_InventoryGrid::OnTileParametersUpdated(const FInv_TileParameters& Parameters)
{
	if (!IsValid(HoverItem)) return;
	
	const FIntPoint Dimensions = HoverItem->GetGridDimensions(); 
	
	const FIntPoint StartingCoordinate = CalculateStartingCoordinates(Parameters.TileCoordinats, Dimensions, Parameters.TileQuadrant);
	ItemDropIndex = UInv_WidgetUtils::GetIndexFromPosition(StartingCoordinate, Columns);
	
	CurrecntQueryResult = CheckHoverPosition(StartingCoordinate, Dimensions);
	
	if (CurrecntQueryResult.bHasSpace)
	{
		HighlightSlots(ItemDropIndex, Dimensions);
		return;
	}
	UnHighlightSlots(LastHightlightedIndex, LastHightlightedDimensions);
	
	if (CurrecntQueryResult.ValidItem.IsValid() && GridSlots.IsValidIndex(CurrecntQueryResult.UpperLeftIndex))
	{
		const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(CurrecntQueryResult.ValidItem.Get(), FragmentTags::GridFragment);
		if (!GridFragment) return;
		
		ChangeHoverType(CurrecntQueryResult.UpperLeftIndex, GridFragment->GetGridSize(), EInv_GridSlotState::GrayedOut);
	}
	
}

FIntPoint UInv_InventoryGrid::CalculateStartingCoordinates(const FIntPoint& Coordinate, const FIntPoint& Dimensions,
	const EInv_TileQuadrant Quadrant) const
{
	const int32 HasEvenWidth = Dimensions.X % 2 == 0 ? 1 : 0;
	const int32 HasEvenHeight = Dimensions.Y % 2 == 0 ? 1 : 0;
	
	FIntPoint StartingCoord;
	switch (Quadrant)
	{
		case EInv_TileQuadrant::TopLeft:
				StartingCoord.X = Coordinate.X - FMath::FloorToInt(0.5 * Dimensions.X);
				StartingCoord.Y = Coordinate.Y - FMath::FloorToInt(0.5 * Dimensions.Y);
		break;
		
		case EInv_TileQuadrant::TopRight:
				StartingCoord.X = Coordinate.X - FMath::FloorToInt(0.5 * Dimensions.X) + HasEvenWidth;
				StartingCoord.Y = Coordinate.Y - FMath::FloorToInt(0.5 * Dimensions.Y);
		break;
		
		case EInv_TileQuadrant::BottomLeft:
				StartingCoord.X = Coordinate.X - FMath::FloorToInt(0.5 * Dimensions.X);
				StartingCoord.Y = Coordinate.Y - FMath::FloorToInt(0.5 * Dimensions.Y) + HasEvenHeight;
		break;
		
		case EInv_TileQuadrant::BottomRight:
				StartingCoord.X = Coordinate.X - FMath::FloorToInt(0.5 * Dimensions.X) + HasEvenWidth;
				StartingCoord.Y = Coordinate.Y - FMath::FloorToInt(0.5 * Dimensions.Y) + HasEvenHeight;
		break;
		
		default:
		return FIntPoint(-1, -1);
	}
	return StartingCoord;
}

FInv_SpaceQueryResult UInv_InventoryGrid::CheckHoverPosition(const FIntPoint& Position,
	const FIntPoint& Dimensions) 
{
	FInv_SpaceQueryResult Result;
	
	if (!IsInGridBounds(UInv_WidgetUtils::GetIndexFromPosition(Position, Columns), Dimensions)) return Result;
	
	Result.bHasSpace = true;
	
	TSet<int32> OccupiedUpperLeftIndices;
	UInv_InventoryStatics::ForEach2D(GridSlots, UInv_WidgetUtils::GetIndexFromPosition(Position, Columns), Dimensions, Columns, [&](const UInv_GridSlot* GridSlot)
	{
		if (GridSlot->GetInventoryItem().IsValid())
		{
			OccupiedUpperLeftIndices.Add(GridSlot->GetUpperLeftIndex());
			Result.bHasSpace = false;
		}
	});
	
	if (OccupiedUpperLeftIndices.Num() == 1)
	{
		const int32 Index = *OccupiedUpperLeftIndices.CreateConstIterator();
		Result.ValidItem = GridSlots[Index]->GetInventoryItem();
		Result.UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	}
	
	
	return Result;
}

bool UInv_InventoryGrid::CursorExitedCanvas(const FVector2D& BoundaryPos, const FVector2D& BoundarySize,
	const FVector2D& Location)
{
	bLastMouseWithinCanvas = bMouseWithinCanvas;
	bMouseWithinCanvas = UInv_WidgetUtils::IsWithinBounds(BoundaryPos, BoundarySize, Location);

	if (!bMouseWithinCanvas && bLastMouseWithinCanvas)
	{
		UnHighlightSlots(LastHightlightedIndex, LastHightlightedDimensions);
		return true;
	}

	return false;
}

void UInv_InventoryGrid::HighlightSlots(const int32 Index, const FIntPoint& Dimensions)
{
	if (!bMouseWithinCanvas) return;
	UnHighlightSlots(LastHightlightedIndex, LastHightlightedDimensions);
	UInv_InventoryStatics::ForEach2D(GridSlots, Index, Dimensions, Columns, [&](UInv_GridSlot* GridSlot)
	{
		GridSlot->SetOccupiedTexture();
	});
	LastHightlightedDimensions = Dimensions;
	LastHightlightedIndex = Index;
}

void UInv_InventoryGrid::UnHighlightSlots(const int32 Index, const FIntPoint& Dimensions)
{
	UInv_InventoryStatics::ForEach2D(GridSlots, Index, Dimensions, Columns, [&](UInv_GridSlot* GridSlot)
	{
		if (GridSlot->IsAvailable())
		{
			GridSlot->SetUnoccupideTexture();
		}
		else
		{
			GridSlot->SetOccupiedTexture();
		}
	});
}

void UInv_InventoryGrid::ChangeHoverType(const int32 Index, const FIntPoint& Dimensions,
	EInv_GridSlotState GridSlotState)
{
	UnHighlightSlots(LastHightlightedIndex, LastHightlightedDimensions);
	UInv_InventoryStatics::ForEach2D(GridSlots, Index, Dimensions, Columns, [State = GridSlotState](UInv_GridSlot* GridSlot)
	{
		switch (State)
		{
			case EInv_GridSlotState::Occupide:
				GridSlot->SetOccupiedTexture();
			break;
			
			case EInv_GridSlotState::Unoccupide:
				GridSlot->SetUnoccupideTexture();
			break;
			
			case EInv_GridSlotState::GrayedOut:
				GridSlot->SetGreyedOutTexture();
			break;
			
			case EInv_GridSlotState::Selected:
				GridSlot->SetSelectedTexture();
			break;
		}
	});
}

void UInv_InventoryGrid::PutDownOnIndex(const int32 Index)
{
	if (!IsValid(HoverItem))
	{
		return;
	}

	if (!GridSlots.IsValidIndex(Index))
	{
		return;
	}

	UInv_InventoryItem* ItemToPlace = HoverItem->GetInventoryItem();
	if (!IsValid(ItemToPlace))
	{
		ClearHoverItem();
		return;
	}

	const bool bStackable = HoverItem->IsStackable();
	const int32 StackCount = HoverItem->GetStackCount();

	AddItemAtIndex(ItemToPlace, Index, bStackable, StackCount);
	UpdateGridSlot(ItemToPlace, Index, bStackable, StackCount);

	ClearHoverItem();
}

void UInv_InventoryGrid::ClearHoverItem()
{
	ClearItemPopUp();

	if (!IsValid(HoverItem)) return;

	HoverItem->SetInventoryItem(nullptr);
	HoverItem->SetIsStackable(false);
	HoverItem->SetPreviousGridIndex(INDEX_NONE);
	HoverItem->UpdateStackCount(0);
	HoverItem->SetImageBrush(FSlateNoResource());

	HoverItem->RemoveFromParent();
	HoverItem = nullptr;

	ShowCursor();
}

UUserWidget* UInv_InventoryGrid::GetVisibleCursorWidget()
{
	if (!IsValid(GetOwningPlayer())) return nullptr;
	if (!IsValid(VisibleCursorWidget))
	{
		VisibleCursorWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), VisibleCursorWidgetClass);
	}
	return VisibleCursorWidget;
}

UUserWidget* UInv_InventoryGrid::GetHiddenCursorWidget()
{
	if (!IsValid(GetOwningPlayer())) return nullptr;
	if (!IsValid(HiddenCursorWidget))
	{
		HiddenCursorWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), HiddenCursorWidgetClass);
	}
	return HiddenCursorWidget;
}

bool UInv_InventoryGrid::IsSameStackable(const UInv_InventoryItem* ClickedInventoryItem) const
{
	const bool bIsSameItem = ClickedInventoryItem == HoverItem->GetInventoryItem();
	const bool bIsStackable = ClickedInventoryItem->IsStackable();
	return bIsSameItem && bIsStackable && HoverItem->GetItemType().MatchesTagExact(ClickedInventoryItem->GetItemManifest().GetItemType());
}

void UInv_InventoryGrid::SwapWithHoverItem(UInv_InventoryItem* ClickedInventoryItem, const int32 GridIndex)
{
	if (!IsValid(HoverItem))
	{
		return;
	}

	if (!IsValid(ClickedInventoryItem) || !GridSlots.IsValidIndex(GridIndex))
	{
		return;
	}

	const int32 UpperLeftIndex = GridSlots[GridIndex]->GetUpperLeftIndex() != INDEX_NONE
		? GridSlots[GridIndex]->GetUpperLeftIndex()
		: GridIndex;

	if (!GridSlots.IsValidIndex(UpperLeftIndex))
	{
		return;
	}

	UInv_InventoryItem* TempInventoryItem = HoverItem->GetInventoryItem();
	const int32 TempStackCount = HoverItem->GetStackCount();
	const bool bTempIsStackable = HoverItem->IsStackable();

	if (!IsValid(TempInventoryItem))
	{
		return;
	}

	AssignHoverItem(ClickedInventoryItem, UpperLeftIndex, HoverItem->GetPreviousGridIndex());
	RemoveItemFromGrid(ClickedInventoryItem, UpperLeftIndex);

	if (!GridSlots.IsValidIndex(ItemDropIndex))
	{
		return;
	}

	AddItemAtIndex(TempInventoryItem, ItemDropIndex, bTempIsStackable, TempStackCount);
	UpdateGridSlot(TempInventoryItem, ItemDropIndex, bTempIsStackable, TempStackCount);
}

bool UInv_InventoryGrid::ShouldSwapStackCounts(const int32 RoomInClickedSlot, const int32 HoveredStackCount,
	const int32 MaxStackSize) const
{
	return RoomInClickedSlot == 0 && HoveredStackCount < MaxStackSize;
}

void UInv_InventoryGrid::SwapStackCounts(const int32 ClickedStackCount, const int32 HoveredStackCount,
	const int32 Index)
{
	UInv_GridSlot* GridSlot = GridSlots[Index];
	GridSlot->SetStackCount(HoveredStackCount);
	
	UInv_SlottedItem* ClickedSlottedItem = SlottedItems.FindChecked(Index);
	ClickedSlottedItem->UpdateStackCount(HoveredStackCount);
	
	HoverItem->UpdateStackCount(ClickedStackCount);
}

bool UInv_InventoryGrid::ShouldConsumeHoverItemStacks(const int32 HoveredStackCount,
	const int32 RoomInClickedSlot) const
{
	return RoomInClickedSlot >= HoveredStackCount;
}

void UInv_InventoryGrid::ConsumeHoverItemStacks(const int32 ClickedStackCount, const int32 HoveredStackCount,
	const int32 Index)
{
	const int32 AmountToTransfer = HoveredStackCount;
	const int32 NewClickedStackCount = ClickedStackCount + AmountToTransfer;
	
	GridSlots[Index]->SetStackCount(NewClickedStackCount);
	SlottedItems.FindChecked(Index)->UpdateStackCount(NewClickedStackCount);
	ClearHoverItem();
	ShowCursor();
	
	const FInv_GridFragment* GridFragment = GridSlots[Index]->GetInventoryItem()->GetItemManifest().GetFragmentOfType<FInv_GridFragment>();
	const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1,1);
	HighlightSlots(Index, Dimensions);
}

bool UInv_InventoryGrid::ShouldFillInStack(const int32 RoomInClickedSlot, const int32 HoveredStackCount) const
{
	return RoomInClickedSlot < HoveredStackCount;
}

void UInv_InventoryGrid::FillInStack(const int32 FillAmount, const int32 Remainder, const int32 Index)
{
	UInv_GridSlot* GridSlot = GridSlots[Index];
	const int32 NewStackCount = GridSlot->GetStackCount() + FillAmount;
	
	GridSlot->SetStackCount(NewStackCount);
	
	UInv_SlottedItem* ClickedSlottedItem = SlottedItems.FindChecked(Index);
	ClickedSlottedItem->UpdateStackCount(NewStackCount);
	
	HoverItem->UpdateStackCount(Remainder);
}

void UInv_InventoryGrid::CreateItemPopUp(const int32 GridIndex)
{
	UInv_InventoryItem* RightClickedItem = GridSlots[GridIndex]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	if (IsValid(GridSlots[GridIndex]->GetItemPopUp())) return;
	
	ItemPopUp = CreateWidget<UInv_ItemPopUp>(this, ItemPopUpClass);
	GridSlots[GridIndex]->SetItemPopUp(ItemPopUp);
	
	OwningCanvasPanel->AddChild(ItemPopUp);
	UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ItemPopUp);
	const FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());
	CanvasSlot->SetPosition(MousePosition);
	CanvasSlot->SetSize(ItemPopUp->GetBoxSize());
	
	const int32 SliderMax = GridSlots[GridIndex]->GetStackCount() - 1;
	if (RightClickedItem->IsStackable() && SliderMax > 0)
	{
		ItemPopUp->OnSplit.BindDynamic(this, &ThisClass::OnPopUpMenuSplit);
		ItemPopUp->SetSliderParams(SliderMax, FMath::Max(1, GridSlots[GridIndex]->GetStackCount() / 2));
	}
	else
	{
		ItemPopUp->CollapseSplitButton();
	}
	
	ItemPopUp->OnDrop.BindDynamic(this, &ThisClass::OnPopUpMenuDrop);
	
	if (RightClickedItem->IsConsumable())
	{
		ItemPopUp->OnConsume.BindDynamic(this, &ThisClass::OnPopUpMenuConsume);
	}
	else
	{
		ItemPopUp->CollapseConsumeButton();
	}
}

void UInv_InventoryGrid::ClearItemPopUp()
{
	if (IsValid(ItemPopUp))
	{
		ItemPopUp->RemoveFromParent();
		ItemPopUp = nullptr;
	}

	for (UInv_GridSlot* GridSlot : GridSlots)
	{
		if (IsValid(GridSlot) && IsValid(GridSlot->GetItemPopUp()))
		{
			GridSlot->GetItemPopUp()->RemoveFromParent();
			GridSlot->SetItemPopUp(nullptr);
		}
	}
}

void UInv_InventoryGrid::PutHoverItemBack()
{
	if (!IsValid(HoverItem)) return;
	
	FInv_SlotAvailabilityResult Result = HasRoomForItem(HoverItem->GetInventoryItem(), HoverItem->GetStackCount());
	Result.Item = HoverItem->GetInventoryItem();
	
	AddStacks(Result);
	ClearHoverItem();
}

void UInv_InventoryGrid::DropItem()
{
	if (!IsValid(HoverItem)) return;
	if (!IsValid(HoverItem->GetInventoryItem())) return;
	
	InventoryComponent->Server_DropItem(HoverItem->GetInventoryItem(), HoverItem->GetStackCount());
	
	ClearHoverItem();
	ShowCursor();
	
}

bool UInv_InventoryGrid::HasHoverItem() const
{
	return IsValid(HoverItem);
}

UInv_HoverItem* UInv_InventoryGrid::GetHoverItem() const
{
	return HoverItem;
}

void UInv_InventoryGrid::ShowCursor()
{
	if (!IsValid(GetOwningPlayer())) return;
	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default, GetVisibleCursorWidget());
}

void UInv_InventoryGrid::HideCursor()
{
	if (!IsValid(GetOwningPlayer())) return;
	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default, GetHiddenCursorWidget());
}

void UInv_InventoryGrid::SetOwningCanvas(UCanvasPanel* OwningCanvas)
{
	OwningCanvasPanel = OwningCanvas;
}

void UInv_InventoryGrid::AssignHoverItem(UInv_InventoryItem* InventoryItem)
{
	if (!IsValid(InventoryItem)) return;

	if (!IsValid(HoverItem))
	{
		HoverItem = CreateWidget<UInv_HoverItem>(GetOwningPlayer(), HoverItemClass);
	}

	if (!IsValid(HoverItem)) return;
	
	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(InventoryItem, FragmentTags::GridFragment);
	const FInv_ImageFragment* ImageFragment = GetFragment<FInv_ImageFragment>(InventoryItem, FragmentTags::IconFragment);
	if (!GridFragment || !ImageFragment) return;
	
	const FVector2D DrawSize = GetDrawSize(GridFragment);
	
	FSlateBrush IconBrush;
	IconBrush.SetResourceObject(ImageFragment->GetIcon());
	IconBrush.DrawAs = ESlateBrushDrawType::Image;
	IconBrush.ImageSize = DrawSize * UWidgetLayoutLibrary::GetViewportScale(this);

	HoverItem->SetInventoryItem(InventoryItem);
	HoverItem->SetGridDimensions(GridFragment->GetGridSize());
	HoverItem->SetImageBrush(IconBrush);
	HoverItem->SetIsStackable(InventoryItem->IsStackable());
	HoverItem->SetItemType(InventoryItem->GetItemManifest().GetItemTag());
	
	GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default, HoverItem);
}

void UInv_InventoryGrid::OnHide()
{
	PutHoverItemBack();
}

void UInv_InventoryGrid::AddStacks(const FInv_SlotAvailabilityResult& Result)
{
	if (!MatchesCategory(Result.Item.Get())) return;
	
	for (const auto& Availability : Result.SlotAvailabilities)
	{
		if (Availability.bItemAtIndex)
		{
			if (!GridSlots.IsValidIndex(Availability.Index)) return;

			UInv_GridSlot* GridSlot = GridSlots[Availability.Index];
			if (!IsValid(GridSlot)) return;

			TObjectPtr<UInv_SlottedItem>* FoundSlottedItem = SlottedItems.Find(Availability.Index);
			if (!FoundSlottedItem || !IsValid(FoundSlottedItem->Get())) return;

			const int32 NewStackCount = GridSlot->GetStackCount() + Availability.AmountToFill;

			GridSlot->SetStackCount(NewStackCount);
			FoundSlottedItem->Get()->UpdateStackCount(NewStackCount);
		}
		else
		{
			AddItemAtIndex(Result.Item.Get(), Availability.Index, Result.bStackable, Availability.AmountToFill);
			UpdateGridSlot(Result.Item.Get(), Availability.Index, Result.bStackable, Availability.AmountToFill);
		}
	}
}

void UInv_InventoryGrid::OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (!GridSlots.IsValidIndex(GridIndex))
	{
		return;
	}

	const int32 UpperLeftIndex = GridSlots[GridIndex]->GetUpperLeftIndex() != INDEX_NONE
		? GridSlots[GridIndex]->GetUpperLeftIndex()
		: GridIndex;

	if (!GridSlots.IsValidIndex(UpperLeftIndex))
	{
		return;
	}

	UInv_InventoryItem* ClickedInventoryItem = GridSlots[UpperLeftIndex]->GetInventoryItem().Get();

	if (!IsValid(ClickedInventoryItem))
	{
		if (TObjectPtr<UInv_SlottedItem>* FoundSlottedItem = SlottedItems.Find(UpperLeftIndex))
		{
			if (IsValid(FoundSlottedItem->Get()))
			{
				FoundSlottedItem->Get()->RemoveFromParent();
			}

			SlottedItems.Remove(UpperLeftIndex);
		}

		return;
	}

	if (!IsValid(HoverItem) && IsLeftClick(MouseEvent))
	{
		PickUp(ClickedInventoryItem, UpperLeftIndex);
		return;
	}

	if (IsRightClick(MouseEvent))
	{
		CreateItemPopUp(UpperLeftIndex);
		return;
	}

	if (!IsValid(HoverItem))
	{
		return;
	}

	if (IsSameStackable(ClickedInventoryItem))
	{
		const int32 ClickedStackCount = GridSlots[UpperLeftIndex]->GetStackCount();

		const FInv_StackableFragment* StackableFragment =
			ClickedInventoryItem->GetItemManifest().GetFragmentOfType<FInv_StackableFragment>();

		if (!StackableFragment)
		{
			return;
		}

		const int32 MaxStackSize = StackableFragment->GetMaxStackSize();
		const int32 RoomInClickedSlot = MaxStackSize - ClickedStackCount;
		const int32 HoveredStackCount = HoverItem->GetStackCount();

		if (ShouldSwapStackCounts(RoomInClickedSlot, HoveredStackCount, MaxStackSize))
		{
			SwapStackCounts(ClickedStackCount, HoveredStackCount, UpperLeftIndex);
			return;
		}

		if (ShouldConsumeHoverItemStacks(HoveredStackCount, RoomInClickedSlot))
		{
			ConsumeHoverItemStacks(ClickedStackCount, HoveredStackCount, UpperLeftIndex);
			return;
		}

		if (ShouldFillInStack(RoomInClickedSlot, HoveredStackCount))
		{
			FillInStack(RoomInClickedSlot, HoveredStackCount - RoomInClickedSlot, UpperLeftIndex);
			return;
		}

		if (RoomInClickedSlot == 0)
		{
			return;
		}
	}
	if (CurrecntQueryResult.ValidItem.IsValid())
	{
		SwapWithHoverItem(ClickedInventoryItem, UpperLeftIndex);
	}
}

void UInv_InventoryGrid::OnGridSlotClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (!IsValid(HoverItem))
	{
		return;
	}

	if (!GridSlots.IsValidIndex(ItemDropIndex))
	{
		return;
	}

	if (CurrecntQueryResult.ValidItem.IsValid() && GridSlots.IsValidIndex(CurrecntQueryResult.UpperLeftIndex))
	{
		OnSlottedItemClicked(CurrecntQueryResult.UpperLeftIndex, MouseEvent);
		return;
	}
	
	if (!IsInGridBounds(ItemDropIndex, HoverItem->GetGridDimensions())) return;

	UInv_GridSlot* GridSlot = GridSlots[ItemDropIndex];
	if (!IsValid(GridSlot))
	{
		return;
	}

	if (!GridSlot->GetInventoryItem().IsValid())
	{
		PutDownOnIndex(ItemDropIndex);
	}
}

void UInv_InventoryGrid::OnGridSlotHovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (IsValid(HoverItem)) return;
	
	UInv_GridSlot* GridSlot = GridSlots[GridIndex];
	if (GridSlot->IsAvailable())
	{
		GridSlot->SetOccupiedTexture();
	}
}

void UInv_InventoryGrid::OnGridSlotUnHovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	if (IsValid(HoverItem)) return;
	
	UInv_GridSlot* GridSlot = GridSlots[GridIndex];
	if (GridSlot->IsAvailable())
	{
		GridSlot->SetUnoccupideTexture();
	}
}

void UInv_InventoryGrid::OnPopUpMenuSplit(int32 SplitAmount, int32 Index)
{
	UInv_InventoryItem* RightClickedItem = GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	if (!RightClickedItem->IsStackable()) return;
	
	const int32 UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	UInv_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
	const int32 StackCount = UpperLeftGridSlot->GetStackCount();
	const int32 NewStackCount = StackCount - SplitAmount;
	
	UpperLeftGridSlot->SetStackCount(NewStackCount);
	SlottedItems.FindChecked(UpperLeftIndex)->UpdateStackCount(NewStackCount);
	
	AssignHoverItem(RightClickedItem, UpperLeftIndex, UpperLeftIndex);
	HoverItem->UpdateStackCount(SplitAmount);
}

void UInv_InventoryGrid::OnPopUpMenuDrop(int32 Index)
{
	UInv_InventoryItem* RightClickedItem = GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	
	PickUp(RightClickedItem, Index);
	DropItem();
}

void UInv_InventoryGrid::OnPopUpMenuConsume(int32 Index)
{
	UInv_InventoryItem* RightClickedItem = GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	
	const int32 UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	UInv_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
	const int32 NewStackCount = UpperLeftGridSlot->GetStackCount() - 1;
	
	UpperLeftGridSlot->SetStackCount(NewStackCount);
	SlottedItems.FindChecked(UpperLeftIndex)->UpdateStackCount(NewStackCount);
	
	InventoryComponent->Server_ConsumeItem(RightClickedItem);
	
	
	if (NewStackCount <= 0)
	{
		RemoveItemFromGrid(RightClickedItem, Index);
	}
}

void UInv_InventoryGrid::OnInventoryMenuToggled(bool bOpen)
{
	if (!bOpen)
	{
		PutHoverItemBack();
	}
}

void UInv_InventoryGrid::AddItem(UInv_InventoryItem* Item)
{
	if (!IsValid(Item))
	{
		return;
	}

	// Безпечний доступ до Manifest
	const FInv_ItemManifest& Manifest = Item->GetItemManifest();

	if (Manifest.GetItemCategory() != ItemCategory)
	{
		return;
	}

	FInv_SlotAvailabilityResult Result = HasRoomForItem(Manifest);

	AddItemToIndices(Result, Item);
}

void UInv_InventoryGrid::ConstructGrid()
{
	if (!CanvasPanel)
	{
		return;
	}

	if (!GridSlotClass)
	{
		return;
	}

	GridSlots.Reserve(Rows * Columns);
	
	for (int32 j = 0; j < Rows; j++)
	{
		for (int32 i = 0; i < Columns; i++)
		{
			UInv_GridSlot* GridSlot = CreateWidget<UInv_GridSlot>(this, GridSlotClass);

			if (!IsValid(GridSlot))
			{
				continue;
			}

			CanvasPanel->AddChild(GridSlot);
			
			const FIntPoint TilePosition(i, j);
			GridSlot->SetTileIndex(UInv_WidgetUtils::GetIndexFromPosition(TilePosition, Columns));
			
			UCanvasPanelSlot* GridCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(GridSlot);
			if (!GridCPS) continue;

			GridCPS->SetSize(FVector2D(TileSize));
			GridCPS->SetPosition(TilePosition * TileSize);
			
			GridSlots.Add(GridSlot);
			GridSlot->GridSlotClicked.AddDynamic(this, &ThisClass::OnGridSlotClicked);
			GridSlot->GridSlotHovered.AddDynamic(this, &ThisClass::OnGridSlotHovered);
			GridSlot->GridSlotUnhovered.AddDynamic(this, &ThisClass::OnGridSlotUnHovered);
		}
	}
}

void UInv_InventoryGrid::AddItemToIndices(const FInv_SlotAvailabilityResult& Result, UInv_InventoryItem* NewItem)
{
	for (const auto& Availability : Result.SlotAvailabilities)
	{
		AddItemAtIndex(NewItem, Availability.Index, Result.bStackable, Availability.AmountToFill);
		UpdateGridSlot(NewItem, Availability.Index, Result.bStackable, Availability.AmountToFill);
	}
}

FVector2D UInv_InventoryGrid::GetDrawSize(const FInv_GridFragment* GridFragment) const
{
	const float IconTileWidth = TileSize - GridFragment->GetGridPadding() * 2;
	return GridFragment->GetGridSize() * IconTileWidth;
}

void UInv_InventoryGrid::SetSlottedItemImage(const UInv_SlottedItem* SlottedItem, const FInv_GridFragment* GridFragment,
	const FInv_ImageFragment* ImageFragment) const
{
	FSlateBrush Brush;
	Brush.SetResourceObject(ImageFragment->GetIcon());
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = GetDrawSize(GridFragment);
	SlottedItem->SetImageBrush(Brush);
}

void UInv_InventoryGrid::AddItemAtIndex(UInv_InventoryItem* Item, const int32 Index, const bool bStackable,
	const int32 StackAmount)
{
	if (!IsValid(Item))
	{
		return;
	}

	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(Item, FragmentTags::GridFragment);
	const FInv_ImageFragment* ImageFragment = GetFragment<FInv_ImageFragment>(Item, FragmentTags::IconFragment);
	if (!GridFragment || !ImageFragment) return;

	if (!SlottedItemClass) return;
	
	
	UInv_SlottedItem* SlottedItem = CreateSlottedItem(Item, bStackable, StackAmount, GridFragment, ImageFragment, Index);

	if (!IsValid(SlottedItem))
	{
		UE_LOG(LogTemp, Error, TEXT("AddItemAtIndex: SlottedItem INVALID"));
		return;
	}

	if (!CanvasPanel)
	{
		UE_LOG(LogTemp, Error, TEXT("AddItemAtIndex: CanvasPanel NULL"));
		return;
	}

	CanvasPanel->AddChild(SlottedItem);

	UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(SlottedItem);

	if (!CanvasSlot)
	{
		UE_LOG(LogTemp, Error, TEXT("AddItemAtIndex: CanvasSlot NULL"));
		return;
	}

	CanvasSlot->SetSize(GetDrawSize(GridFragment));

	const FVector2D DrawPos = UInv_WidgetUtils::GetPositionFromIndex(Index, Columns) * TileSize;
	const FVector2D DrawPosWithPadding = DrawPos + FVector2D(GridFragment->GetGridPadding());

	CanvasSlot->SetPosition(DrawPosWithPadding);

	SlottedItems.Add(Index, SlottedItem);
}

UInv_SlottedItem* UInv_InventoryGrid::CreateSlottedItem(UInv_InventoryItem* Item, const bool bStackable,
	const int32 StackAmount, const FInv_GridFragment* GridFragment, const FInv_ImageFragment* ImageFragment,
	const int32 Index)
{
	if (!IsValid(GetOwningPlayer()))
	{
		return nullptr;
	}

	UInv_SlottedItem* SlottedItem = CreateWidget<UInv_SlottedItem>(GetOwningPlayer(), SlottedItemClass);

	if (!IsValid(SlottedItem))
	{
		return nullptr;
	}

	SlottedItem->SetInventoryItem(Item);
	SetSlottedItemImage(SlottedItem, GridFragment, ImageFragment);
	SlottedItem->SetGridIndex(Index);
	SlottedItem->SetIsStackable(bStackable);
	const int32 StackUpdateAmount = bStackable ? StackAmount : 0;
	SlottedItem->UpdateStackCount(StackUpdateAmount);
	SlottedItem->OnSLottedItemClicked.AddDynamic(this, &ThisClass::OnSlottedItemClicked);

	return SlottedItem;
}

void UInv_InventoryGrid::UpdateGridSlot(UInv_InventoryItem* NewItem, const int32 Index, bool bStackableItem, const int32 StackAmount)
{
	check(GridSlots.IsValidIndex(Index));
	
	if (bStackableItem)
	{
		GridSlots[Index]->SetStackCount(StackAmount);
	}
	
	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(NewItem, FragmentTags::GridFragment);
	const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1,1);
	
	UInv_InventoryStatics::ForEach2D(GridSlots, Index, Dimensions, Columns, [&](UInv_GridSlot* GridSlot)
	{
		GridSlot->SetInventoryItem(NewItem);
		GridSlot->SetUpperLeftIndex(Index);
		GridSlot->SetOccupiedTexture();
		GridSlot->SetAvailable(false);
	});
}

bool UInv_InventoryGrid::IsIndexClaimed(const TSet<int32>& CheckedIndices, const int32 Index) const
{
	return CheckedIndices.Contains(Index);
}

bool UInv_InventoryGrid::MatchesCategory(const UInv_InventoryItem* Item) const
{
	if (!IsValid(Item))
	{
		return false;
	}

	return Item->GetItemManifest().GetItemCategory() == ItemCategory;
}
