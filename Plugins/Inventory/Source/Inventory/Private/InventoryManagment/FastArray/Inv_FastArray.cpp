#include "InventoryManagment/FastArray/Inv_FastArray.h"
#include "InventoryManagment/Components/Inv_InventoryComponent.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Components/Inv_ItemComponent.h"

TArray<UInv_InventoryItem*> FInv_InventoryFastArray::GetAllItems() const
{
	TArray<UInv_InventoryItem*> Results;
	Results.Reserve(Entries.Num());
	for (const auto& Entry : Entries)
	{
		if (!IsValid(Entry.Item)) continue;
		Results.Add(Entry.Item);
	}
	return Results;
}

void FInv_InventoryFastArray::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	UE_LOG(LogTemp, Warning, TEXT("PreReplicatedRemove START. RemovedIndices Num=%d FinalSize=%d OwnerComponent=%s"),
		RemovedIndices.Num(),
		FinalSize,
		*GetNameSafe(OwnerComponent));

	UInv_InventoryComponent* IC = Cast<UInv_InventoryComponent>(OwnerComponent);
	if (!IsValid(IC))
	{
		UE_LOG(LogTemp, Warning, TEXT("PreReplicatedRemove EXIT: InventoryComponent invalid"));
		return;
	}
	
	for (int32 Index : RemovedIndices)
	{
		UE_LOG(LogTemp, Warning, TEXT("PreReplicatedRemove broadcasting removed item=%s at index=%d"),
			*GetNameSafe(Entries[Index].Item),
			Index);

		IC->OnItemRemoved.Broadcast(Entries[Index].Item);
	}
}

void FInv_InventoryFastArray::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	UE_LOG(LogTemp, Warning, TEXT("PostReplicatedAdd START. AddedIndices Num=%d FinalSize=%d OwnerComponent=%s"),
		AddedIndices.Num(),
		FinalSize,
		*GetNameSafe(OwnerComponent));

	UInv_InventoryComponent* IC = Cast<UInv_InventoryComponent>(OwnerComponent);
	if (!IsValid(IC))
	{
		UE_LOG(LogTemp, Warning, TEXT("PostReplicatedAdd EXIT: InventoryComponent invalid"));
		return;
	}
	
	for (int32 Index : AddedIndices)
	{
		UE_LOG(LogTemp, Warning, TEXT("PostReplicatedAdd broadcasting item=%s at index=%d"),
			*GetNameSafe(Entries[Index].Item),
			Index);

		IC->OnItemAdded.Broadcast(Entries[Index].Item);
	}

	UE_LOG(LogTemp, Warning, TEXT("PostReplicatedAdd END"));
}

UInv_InventoryItem* FInv_InventoryFastArray::AddEntry(UInv_ItemComponent* ItemComponent)
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	UInv_InventoryComponent* IC = Cast<UInv_InventoryComponent>(OwnerComponent);
	if (!IsValid(IC)) return nullptr;
	
	FInv_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Item = ItemComponent->GetItemManifest().Manifest(OwningActor);
	
	IC->AddRepSubObj(NewEntry.Item);
	MarkItemDirty(NewEntry);
	
	return NewEntry.Item;
}

UInv_InventoryItem* FInv_InventoryFastArray::AddEntry(UInv_InventoryItem* Item)
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());
	
	FInv_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Item = Item;
	
	MarkItemDirty(NewEntry);
	return Item;
}	

void FInv_InventoryFastArray::RemoveEntry(UInv_InventoryItem* Item)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FInv_InventoryEntry& Entry = *EntryIt;
		if (Entry.Item == Item)
		{
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}

UInv_InventoryItem* FInv_InventoryFastArray::FindFirstItemByType(const FGameplayTag& ItemType)
{
	auto* FoundItem = Entries.FindByPredicate([Type = ItemType](const FInv_InventoryEntry& Entry)
	{
		return IsValid(Entry.Item) && Entry.Item->GetItemManifest().GetItemType().MatchesTagExact(Type);
	});
	return FoundItem ? FoundItem->Item : nullptr;
}
