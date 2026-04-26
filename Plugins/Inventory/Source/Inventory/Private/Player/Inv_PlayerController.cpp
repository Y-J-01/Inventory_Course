#include "Player/Inv_PlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Interaction/Inv_Highlightable.h"
#include "InventoryManagment/Components/Inv_InventoryComponent.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "InputMappingContext.h"
#include "Widgets/Hud/Inv_HUDWidget.h"

AInv_PlayerController::AInv_PlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	TraceLength = 500.0;
	
	ItemTraceChannel = ECC_GameTraceChannel1;
}

void AInv_PlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	TraceForItem();
}

void AInv_PlayerController::ToggleInventory()
{
	
	if (!InventoryComponent.IsValid()) return;
	
	InventoryComponent->ToggleInventoryMenu();
	
	if (InventoryComponent->IsMenuOpen())
	{
		HUDWidget->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		HUDWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void AInv_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	
	if (IsValid(Subsystem))
	{
		for (UInputMappingContext* CurrentContext : DefaultIMCs)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}
	
	InventoryComponent = FindComponentByClass<UInv_InventoryComponent>();
	
	CreateHUDWidget();
}

void AInv_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);
	
	EnhancedInputComponent->BindAction(PrimaryInputAction, ETriggerEvent::Started, this, &AInv_PlayerController::PrimaryInteract);
	EnhancedInputComponent->BindAction(ToggleInventorytAction, ETriggerEvent::Started, this, &AInv_PlayerController::ToggleInventory);
}

void AInv_PlayerController::PrimaryInteract()
{
	
	if (!ThisActor.IsValid())
	{
		return;
	}
	
	UInv_ItemComponent* ItemComp = ThisActor->FindComponentByClass<UInv_ItemComponent>();
	
	if (!IsValid(ItemComp))
	{
		return;
	}
	
	if (!InventoryComponent.IsValid())
	{
		return;
	}
	 
	InventoryComponent->TryAddItem(ItemComp);
	
}

void AInv_PlayerController::CreateHUDWidget()
{
	if (!IsLocalController()) return;
	HUDWidget = CreateWidget<UInv_HUDWidget>(this, HUDWidgetClass);
	if (IsValid(HUDWidget))
	{
		HUDWidget->AddToViewport();
	}
	
}

void AInv_PlayerController::TraceForItem()
{
	if (!IsValid(GEngine) || !IsValid(GEngine->GameViewport))
	{
		return;
	}
	
	if (!GetWorld())
	{
		return;
	}
	
	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	const FVector2D ViewportCenter = ViewportSize / 2.f;
	
	FVector TraceStart;
	FVector Forward;
	
	if (!UGameplayStatics::DeprojectScreenToWorld(this, ViewportCenter, TraceStart, Forward))
	{
		UE_LOG(LogTemp, Warning, TEXT("TraceForItem EXIT: DeprojectScreenToWorld failed"));
		return;
	}
	
	const FVector TraceEnd = TraceStart + Forward * TraceLength;
	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ItemTraceChannel);
	
	LastActor = ThisActor;
	ThisActor = HitResult.GetActor();

	if (ThisActor != LastActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("TraceForItem actor changed. LastActor=%s NewActor=%s"),
			*GetNameSafe(LastActor.Get()),
			*GetNameSafe(ThisActor.Get()));
	}
	
	if (!ThisActor.IsValid())
	{
		if (IsValid(HUDWidget))
		{
			HUDWidget->HidePickUpMessage();
			UE_LOG(LogTemp, Warning, TEXT("TraceForItem: No actor hit, HidePickUpMessage called"));
		}
	}
	
	if (ThisActor == LastActor) return;
	
	if (ThisActor.IsValid())
	{
		if (UActorComponent* Highlightable = ThisActor->FindComponentByInterface(UInv_Highlightable::StaticClass()); IsValid(Highlightable))
		{
			UE_LOG(LogTemp, Warning, TEXT("TraceForItem: Highlighting actor=%s Component=%s"),
				*GetNameSafe(ThisActor.Get()),
				*GetNameSafe(Highlightable));
			IInv_Highlightable::Execute_Highlight(Highlightable);
		}
		
		UInv_ItemComponent* ItemComponent = ThisActor->FindComponentByClass<UInv_ItemComponent>();
		UE_LOG(LogTemp, Warning, TEXT("TraceForItem: ItemComponent on hit actor=%s"),
			*GetNameSafe(ItemComponent));
		
		if (IsValid(ItemComponent))
		{
			if (IsValid(HUDWidget))
			{
				HUDWidget->ShowPickUpMessage(ItemComponent->GetPickUpMessage());
				UE_LOG(LogTemp, Warning, TEXT("TraceForItem: ShowPickUpMessage called"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("TraceForItem: HUDWidget invalid, cannot show pickup message"));
			}
		}
	}
	
	if (LastActor.IsValid())
	{
		if (UActorComponent* Highlightable = LastActor->FindComponentByInterface(UInv_Highlightable::StaticClass()); IsValid(Highlightable))
		{
			UE_LOG(LogTemp, Warning, TEXT("TraceForItem: UnHighlight actor=%s Component=%s"),
				*GetNameSafe(LastActor.Get()),
				*GetNameSafe(Highlightable));
			IInv_Highlightable::Execute_UnHighlight(Highlightable);
		}
	}
}