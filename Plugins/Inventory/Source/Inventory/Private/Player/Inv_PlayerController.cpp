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
	UE_LOG(LogTemp, Warning, TEXT("ToggleInventory called. InventoryComponent=%s"),
		*GetNameSafe(InventoryComponent.Get()));
	
	if (!InventoryComponent.IsValid()) return;
	
	InventoryComponent->ToggleInventoryMenu();
}

void AInv_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("PlayerController BeginPlay START"));

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	UE_LOG(LogTemp, Warning, TEXT("EnhancedInput Subsystem = %s"), *GetNameSafe(Subsystem));
	
	if (IsValid(Subsystem))
	{
		for (UInputMappingContext* CurrentContext : DefaultIMCs)
		{
			UE_LOG(LogTemp, Warning, TEXT("Adding Mapping Context: %s"), *GetNameSafe(CurrentContext));
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}
	
	InventoryComponent = FindComponentByClass<UInv_InventoryComponent>();
	
	CreateHUDWidget();
	
	UE_LOG(LogTemp, Warning, TEXT("PlayerController BeginPlay"));
	UE_LOG(LogTemp, Warning, TEXT("Controller InventoryComponent = %s"), *GetNameSafe(FindComponentByClass<UInv_InventoryComponent>()));
	UE_LOG(LogTemp, Warning, TEXT("Stored InventoryComponent = %s"), *GetNameSafe(InventoryComponent.Get()));
	UE_LOG(LogTemp, Warning, TEXT("Pawn = %s"), *GetNameSafe(GetPawn()));
	UE_LOG(LogTemp, Warning, TEXT("HUDWidget = %s"), *GetNameSafe(HUDWidget));
	UE_LOG(LogTemp, Warning, TEXT("PlayerController BeginPlay END"));
}

void AInv_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UE_LOG(LogTemp, Warning, TEXT("SetupInputComponent called. InputComponent=%s"),
		*GetNameSafe(InputComponent));
	
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);
	UE_LOG(LogTemp, Warning, TEXT("EnhancedInputComponent=%s"), *GetNameSafe(EnhancedInputComponent));
	UE_LOG(LogTemp, Warning, TEXT("PrimaryInputAction=%s"), *GetNameSafe(PrimaryInputAction));
	UE_LOG(LogTemp, Warning, TEXT("ToggleInventorytAction=%s"), *GetNameSafe(ToggleInventorytAction));
	
	EnhancedInputComponent->BindAction(PrimaryInputAction, ETriggerEvent::Started, this, &AInv_PlayerController::PrimaryInteract);
	EnhancedInputComponent->BindAction(ToggleInventorytAction, ETriggerEvent::Started, this, &AInv_PlayerController::ToggleInventory);
}

void AInv_PlayerController::PrimaryInteract()
{
	UE_LOG(LogTemp, Warning, TEXT("PrimaryInteract START. ThisActor=%s InventoryComponent=%s"),
		*GetNameSafe(ThisActor.Get()),
		*GetNameSafe(InventoryComponent.Get()));
	
	if (!ThisActor.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("PrimaryInteract EXIT: ThisActor invalid"));
		return;
	}
	
	UInv_ItemComponent* ItemComp = ThisActor->FindComponentByClass<UInv_ItemComponent>();
	UE_LOG(LogTemp, Warning, TEXT("PrimaryInteract ItemComp=%s"), *GetNameSafe(ItemComp));
	
	if (!IsValid(ItemComp))
	{
		UE_LOG(LogTemp, Warning, TEXT("PrimaryInteract EXIT: ItemComp invalid"));
		return;
	}
	
	if (!InventoryComponent.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("PrimaryInteract EXIT: InventoryComponent invalid"));
		return;
	}
	 
	InventoryComponent->TryAddItem(ItemComp);
	
	UE_LOG(LogTemp, Warning, TEXT("PrimaryInteract END. ThisActor=%s InventoryComponent=%s"),
		*GetNameSafe(ThisActor.Get()),
		*GetNameSafe(InventoryComponent.Get()));
}

void AInv_PlayerController::CreateHUDWidget()
{
	UE_LOG(LogTemp, Warning, TEXT("CreateHUDWidget called. IsLocalController=%d HUDWidgetClass=%s"),
		IsLocalController() ? 1 : 0,
		*GetNameSafe(HUDWidgetClass));
	
	if (!IsLocalController()) return;
	
	HUDWidget = CreateWidget<UInv_HUDWidget>(this, HUDWidgetClass);
	UE_LOG(LogTemp, Warning, TEXT("CreateHUDWidget created HUDWidget=%s"), *GetNameSafe(HUDWidget));
	
	if (IsValid(HUDWidget))
	{
		HUDWidget->AddToViewport();
		UE_LOG(LogTemp, Warning, TEXT("HUDWidget added to viewport"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateHUDWidget FAILED: HUDWidget invalid"));
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