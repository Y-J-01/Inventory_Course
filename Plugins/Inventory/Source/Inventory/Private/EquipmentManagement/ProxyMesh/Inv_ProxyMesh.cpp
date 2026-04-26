// Fill out your copyright notice in the Description page of Project Settings.


#include "EquipmentManagement/ProxyMesh/Inv_ProxyMesh.h"

#include "EquipmentManagement/Components/Inv_EquipmentComponent.h"
#include "GameFramework/Character.h"


// Sets default values
AInv_ProxyMesh::AInv_ProxyMesh()
{
	
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(false);
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(FName("Root"));
	
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(FName("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	
	EquipmentComponent = CreateDefaultSubobject<UInv_EquipmentComponent>(FName("Equipment"));
	EquipmentComponent->SetOwningSkeletalMesh(Mesh);
	EquipmentComponent->SetIsProxy(true);
}


void AInv_ProxyMesh::BeginPlay()
{
	Super::BeginPlay();
	DelayedInitializeOwner();
}

void AInv_ProxyMesh::DelayedInitializeOwner()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		DelayedInitialization();
		return;
	}
	
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!IsValid(PC))
	{
		DelayedInitialization();
		return;
	}
	
	ACharacter* Character = Cast<ACharacter>(PC->GetPawn());
	if (!IsValid(Character))
	{
		DelayedInitialization();
		return;
	}
	
	USkeletalMeshComponent* CharacterMesh = Character->GetMesh();
	if (!IsValid(CharacterMesh))
	{
		DelayedInitialization();
		return;
	}
	
	SourceMesh = CharacterMesh;
	Mesh->SetSkeletalMesh(SourceMesh->GetSkeletalMeshAsset());
	Mesh->SetAnimInstanceClass(SourceMesh->GetAnimInstance()->GetClass());
	
	EquipmentComponent->InitializeOwner(PC);
}

void AInv_ProxyMesh::DelayedInitialization()
{
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &ThisClass::DelayedInitializeOwner);
	GetWorld()->GetTimerManager().SetTimerForNextTick(TimerDelegate);
}


