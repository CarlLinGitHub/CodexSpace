#include "SpaceGameMode.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"

ASpaceGameMode::ASpaceGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = nullptr;
    HUDClass = ASpaceHUD::StaticClass();
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Arena")));
    for (int32 Index = 0; Index < static_cast<int32>(EVisual::Count); ++Index)
    {
        auto* Mesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(*FString::Printf(TEXT("Visual%d"), Index));
        Mesh->SetupAttachment(GetRootComponent());
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false);
        Mesh->SetCanEverAffectNavigation(false);
        Visuals.Add(Mesh);
    }
}

void ASpaceGameMode::BeginPlay()
{
    Super::BeginPlay();
    SetActorHiddenInGame(false);
    Camera = GetWorld()->SpawnActor<ACameraActor>(FVector(0, 0, 2600), FRotator(-90, 90, 0));
    auto* Lens = Camera->GetCameraComponent();
    Lens->SetProjectionMode(ECameraProjectionMode::Perspective);
    Lens->SetFieldOfView(33);
    Lens->SetAspectRatio(1.6f);
    Lens->bConstrainAspectRatio = true;
    Lens->PostProcessBlendWeight = 1;
    Lens->PostProcessSettings.bOverride_AutoExposureMethod = true;
    Lens->PostProcessSettings.AutoExposureMethod = AEM_Manual;
    Lens->PostProcessSettings.bOverride_AutoExposureBias = true;
    Lens->PostProcessSettings.AutoExposureBias = 0;
    Lens->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Lens->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
    const auto Light = [this](FRotator Rotation, FColor Color, float Power, int32 Priority)
    {
        auto* Actor = GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector, Rotation);
        auto* Component = CastChecked<UDirectionalLightComponent>(Actor->GetLightComponent());
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetLightColor(Color);
        Component->SetIntensity(Power);
        Component->SetForwardShadingPriority(Priority);
    };
    Light(FRotator(-55, 35, 0), FColor(185, 215, 255), 5, 1);
    Light(FRotator(-30, 210, 0), FColor(137, 82, 255), 3, 0);

    auto* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    const TCHAR* Ships[] = { TEXT("Player"), TEXT("Scout"), TEXT("Heavy"), TEXT("Boss") };
    const FLinearColor Colors[] = { {0.08f,1,1}, {1,0.1f,0.38f}, {1,0.55f,0.12f}, {0.37f,0.52f,0.85f}, {0.06f,0.17f,0.46f} };
    const float ShipRadii[] = { 43, 32, 48, 88 };
    auto* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SpaceShooter/M_SpaceGlow.M_SpaceGlow"));
    auto* PlanetMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SpaceShooter/M_Player.M_Player"));
    for (int32 Index = 0; Index < Visuals.Num(); ++Index)
    {
        if (Index < UE_ARRAY_COUNT(Ships))
        {
            auto* Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/SpaceShooter/Ships/%s.%s"), Ships[Index], Ships[Index]));
            if (!ensure(Mesh)) continue;
            Visuals[Index]->SetStaticMesh(Mesh);
            const auto Bounds = Mesh->GetBounds();
            ShipScales[Index] = ShipRadii[Index] / FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Z);
            ShipCenters[Index] = Bounds.Origin;
        }
        else
        {
            Visuals[Index]->SetStaticMesh(Sphere);
            auto* Material = Index == static_cast<int32>(EVisual::Planet) ? PlanetMaterial : Glow;
            if (auto* Mat = Visuals[Index]->CreateDynamicMaterialInstance(0, Material))
                Mat->SetVectorParameterValue(TEXT("Color"), Colors[Index - UE_ARRAY_COUNT(Ships)] * (Index <= static_cast<int32>(EVisual::Spark) ? 3.0f : 1.0f));
        }
    }
    FRandomStream Random(173);
    Stars.Reserve(180);
    for (int32 Index = 0; Index < 180; ++Index)
        Stars.Add({FVector(Random.FRandRange(-1260, 1260), Random.FRandRange(-780, 780), Random.FRandRange(-580, -270)), Random.FRandRange(24, 135), Random.FRandRange(1.1f, 3.4f)});
    Shots.Reserve(420);
    Enemies.Reserve(28);
    Sparks.Reserve(150);
    Reset();
}

void ASpaceGameMode::Reset()
{
    Player = FVector(0, -350, 0);
    Score = Wave = BossHP = Pattern = Kills = 0;
    Lives = 3;
    Power = 1;
    Pulses = 2;
    Time = Fire = Shield = StepRemainder = 0;
    Enemies.Reset();
    Shots.Reset();
    Sparks.Reset();
    PowerUps.Reset();
    State = EState::Ready;
}

void ASpaceGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    auto* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !Camera) return;
    if (PC->GetViewTarget() != Camera) PC->SetViewTarget(Camera);
    const auto Down = [PC](const FKey& First, const FKey& Second) { return PC->IsInputKeyDown(First) || PC->IsInputKeyDown(Second); };
    if (PC->WasInputKeyJustPressed(EKeys::R)) Reset();
    const bool bFire = Down(EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom);
    bFocus = Down(EKeys::LeftShift, EKeys::RightShift);
    if (State == EState::Ready && bFire) State = EState::Playing;
    if (State == EState::Playing)
    {
        if (PC->WasInputKeyJustPressed(EKeys::Z)) Pulse();
        const FVector Move = FVector(int(Down(EKeys::A,EKeys::Left)) - int(Down(EKeys::D,EKeys::Right)), int(Down(EKeys::W,EKeys::Up)) - int(Down(EKeys::S,EKeys::Down)), 0).GetClampedToMaxSize(1);
        constexpr float Step = 1.0f / 120.0f;
        StepRemainder += FMath::Min(DeltaSeconds, 0.075f);
        while (StepRemainder >= Step && State == EState::Playing)
        {
            Advance(Step, Move, bFire);
            StepRemainder -= Step;
        }
    }
    for (int32 Index = Sparks.Num() - 1; Index >= 0; --Index)
    {
        auto& Spark = Sparks[Index];
        Spark.Pos += Spark.Vel * DeltaSeconds;
        Spark.Life -= DeltaSeconds;
        if (Spark.Life <= 0) Sparks.RemoveAtSwap(Index, EAllowShrinking::No);
    }
    for (auto& Star : Stars)
    {
        Star.Pos.Y -= Star.Speed * DeltaSeconds;
        if (Star.Pos.Y < -780) Star.Pos.Y += 1560;
    }
    Render();
}

void ASpaceGameMode::Draw(EVisual Type, FVector Pos, FVector Scale, float Yaw)
{
    const auto Index = static_cast<int32>(Type);
    FQuat Rotation(FRotator(0, Yaw, 0));
    if (Index < UE_ARRAY_COUNT(ShipScales))
    {
        Rotation *= FQuat(FVector::ForwardVector, -UE_HALF_PI);
        Scale *= ShipScales[Index];
        Pos -= Rotation.RotateVector(ShipCenters[Index] * Scale);
    }
    Frames[Index].Emplace(Rotation, Pos, Scale);
}

void ASpaceGameMode::Render()
{
    for (auto& Frame : Frames) Frame.Reset();
    for (const auto& Star : Stars)
        Draw(EVisual::Star, Star.Pos, FVector(Star.Size, Star.Size * (1 + Star.Speed / 100), Star.Size) / 100);
    Draw(EVisual::Planet, FVector(570, 420, -540), FVector(6));
    Draw(EVisual::Planet, FVector(-670, -330, -730), FVector(2));
    if (State != EState::Lost && (Shield <= 0 || FMath::Fmod(Shield, 0.16f) < 0.08f))
    {
        Draw(EVisual::Player, Player, FVector(1));
        Draw(EVisual::Laser, Player + FVector(0,-32,-6), FVector(.08,.24,.08));
        if (bFocus) Draw(EVisual::Laser, Player + FVector(0,0,34), FVector(.075));
    }
    for (const auto& Enemy : Enemies)
        Draw(static_cast<EVisual>(static_cast<int32>(EVisual::Scout) + Enemy.Kind), Enemy.Pos, FVector(1), 180);
    for (const auto& Shot : Shots)
        Draw(Shot.bEnemy ? EVisual::Plasma : EVisual::Laser, Shot.Pos, Shot.bEnemy ? FVector(.135) : FVector(.075,.29,.075));
    for (const auto& Pickup : PowerUps)
        Draw(EVisual::Spark, Pickup, FVector(.18, .18, .25) * (1 + .2 * FMath::Sin(Time * 12)));
    for (const auto& Spark : Sparks)
        Draw(EVisual::Spark, Spark.Pos, FVector(Spark.Life * .35f));
    for (int32 Index = 0; Index < Visuals.Num(); ++Index)
    {
        auto* Mesh = Visuals[Index].Get();
        auto& Frame = Frames[Index];
        int32 Count = Mesh->GetInstanceCount();
        while (Count > Frame.Num()) Mesh->RemoveInstance(--Count);
        if (Count > 0)
            Mesh->BatchUpdateInstancesTransforms(0, MakeArrayView(static_cast<const FTransform*>(Frame.GetData()), Count), false, true, true);
        for (; Count < Frame.Num(); ++Count) Mesh->AddInstance(Frame[Count]);
    }
}
