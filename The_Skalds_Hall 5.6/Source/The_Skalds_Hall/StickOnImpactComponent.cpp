#include "StickOnImpactComponent.h"
#include "ThrowTargetComponent.h"
#include "HandGrabComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

UStickOnImpactComponent::UStickOnImpactComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UStickOnImpactComponent::BeginPlay()
{
	Super::BeginPlay();
	Mesh = UHandGrabComponent::FindMainMesh(GetOwner());
	if (!Mesh) return;
	Mesh->SetNotifyRigidBodyCollision(true);
	Mesh->OnComponentHit.AddDynamic(this, &UStickOnImpactComponent::HandleHit);
}

void UStickOnImpactComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Velocita' prima dell'urto: nel callback e' gia' stata modificata dalla fisica.
	if (Mesh && Mesh->IsSimulatingPhysics())
	{
		LastVelocity = Mesh->GetPhysicsLinearVelocity();
	}
}

void UStickOnImpactComponent::HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!Mesh || !Mesh->IsSimulatingPhysics() || !OtherComp || !OtherActor) return;
	if (!OtherActor->ActorHasTag(TargetTag) && !OtherComp->ComponentHasTag(TargetTag)) return;

	const float Speed = LastVelocity.Size();
	if (Speed < MinStickSpeed) return;

	const FVector Dir = LastVelocity / Speed;
	const FVector Blade = Mesh->GetComponentTransform().TransformVectorNoScale(BladeAxis).GetSafeNormal();
	if (MinBladeDot > -1.f && FVector::DotProduct(Blade, Dir) < MinBladeDot) return;

	TWeakObjectPtr<UStaticMeshComponent> WeakMesh = Mesh;
	TWeakObjectPtr<UPrimitiveComponent> WeakTarget = OtherComp;
	TWeakObjectPtr<AActor> WeakActor = OtherActor;
	const FVector ImpactPoint = Hit.ImpactPoint;
	const float Depth = PenetrationDepth;
	USoundBase* Sound = StickSound;

	// Le modifiche alla fisica dentro il callback d'urto vanno rimandate al frame dopo.
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,
		[WeakMesh, WeakTarget, WeakActor, Dir, ImpactPoint, Depth, Sound, this]()
		{
			if (!WeakMesh.IsValid() || !WeakTarget.IsValid()) return;
			UStaticMeshComponent* M = WeakMesh.Get();
			if (M->GetAttachParent() && !M->IsSimulatingPhysics()) return;

			M->SetSimulatePhysics(false);
			M->AddWorldOffset(Dir * Depth);
			M->AttachToComponent(WeakTarget.Get(), FAttachmentTransformRules::KeepWorldTransform);
			if (Sound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, Sound, ImpactPoint);
			}
			if (WeakActor.IsValid())
			{
				if (UThrowTargetComponent* T = WeakActor->FindComponentByClass<UThrowTargetComponent>())
				{
					T->RegisterHit(ImpactPoint);
				}
			}
		}));
}
