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

namespace
{
const FVector ChaseOffset(0, -1120, 1900);
constexpr double MaxPanX = 210, MaxPanY = 150;
}

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
    Camera = GetWorld()->SpawnActor<ACameraActor>(ChaseOffset, FRotator(-61, 90, 0));
    auto* Lens = Camera->GetCameraComponent();
    Lens->SetProjectionMode(ECameraProjectionMode::Perspective);
    Lens->SetFieldOfView(47);
    Lens->SetAspectRatio(1.6f);
    Lens->bConstrainAspectRatio = true;
    ConfigureViewBounds();
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
    const TCHAR* Ships[] = {
        TEXT("/Game/SpaceShooter/PlayerCraft/Vanguard.Vanguard"),
        TEXT("/Game/SpaceShooter/Hostiles/Scout.Scout"),
        TEXT("/Game/SpaceShooter/Hostiles/Heavy.Heavy"),
        TEXT("/Game/SpaceShooter/Hostiles/Boss.Boss")
    };
    const FLinearColor Colors[] = { {0.08f,1,1}, {1,0.1f,0.38f}, {1,0.55f,0.12f}, {0.37f,0.52f,0.85f}, {0.06f,0.17f,0.46f} };
    const float ShipRadii[] = { 51, 32, 48, 88 };
    auto* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SpaceShooter/M_SpaceGlow.M_SpaceGlow"));
    auto* PlanetMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SpaceShooter/M_Player.M_Player"));
    for (int32 Index = 0; Index <= static_cast<int32>(EVisual::Planet); ++Index)
    {
        if (Index < UE_ARRAY_COUNT(Ships))
        {
            auto* Mesh = LoadObject<UStaticMesh>(nullptr, Ships[Index]);
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
    for (int32 Index = 0; Index < static_cast<int32>(EAmmo::Count); ++Index)
    {
        const auto Color = static_cast<EAmmo>(Index);
        auto* Mesh = Visuals[static_cast<int32>(AmmoVisual(Color))].Get();
        Mesh->SetStaticMesh(Sphere);
        if (auto* Material = Mesh->CreateDynamicMaterialInstance(0, Glow))
            Material->SetVectorParameterValue(TEXT("Color"), AmmoTint(Color) * 3.f);
    }
    auto* Rocks = Visuals[static_cast<int32>(EVisual::Asteroid)].Get();
    if (auto* Rock = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/SpaceShooter/Rocks/Asteroid.Asteroid")))
    {
        Rocks->SetStaticMesh(Rock);
        RockScale = FVector(1.0 / FMath::Max(0.01, Rock->GetBounds().BoxExtent.GetMax()));
    }
    auto* Nebula = Visuals[static_cast<int32>(EVisual::Nebula)].Get();
    if (auto* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SpaceShooter/Backdrop/M_Nebula.M_Nebula")))
    {
        Nebula->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
        Nebula->SetMaterial(0, Material);
        Nebula->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0, 930, -1800), FVector(53, 34, 1)));
    }
    FRandomStream Random(173);
    Stars.Reserve(180);
    for (int32 Index = 0; Index < 180; ++Index)
        Stars.Add({FVector(Random.FRandRange(-1260, 1260), Random.FRandRange(-780, 780), Random.FRandRange(-580, -270)), Random.FRandRange(24, 135), Random.FRandRange(1.1f, 3.4f)});
    Asteroids.Reserve(23);
    for (int32 Index = 0; Index < 23; ++Index)
    {
        const double X = Random.FRandRange(510, 1900) * (Index % 2 ? 1 : -1);
        Asteroids.Add({FVector(X, Random.FRandRange(-2200, 2400), Random.FRandRange(-1130, -460)),
            Random.FRandRange(45, 140), Random.FRandRange(23, 107), Random.FRandRange(-26, 26), Random.FRandRange(0, 360)});
    }
    Shots.Reserve(420);
    Enemies.Reserve(28);
    Sparks.Reserve(150);
    Reset();
}

void ASpaceGameMode::Reset()
{
    Player = FVector(0, -350, 0);
    CameraAnchor = FVector(0, Player.Y * .25, 0);
    Bank = 0;
    Score = Wave = BossHP = BossMaxHP = Pattern = 0;
    Sector = 1;
    VolleyId = 0;
    Intermission = 0;
    Lives = 3;
    ActiveAmmo = EAmmo::Yellow;
    for (auto& Level : AmmoLevels) Level = 0;
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
    if (PC->WasInputKeyJustPressed(EKeys::R) || PC->WasInputKeyJustPressed(EKeys::Gamepad_Special_Right)) Reset();
    const auto Trigger = [PC](const FKey& Key) { return PC->GetInputAnalogKeyState(Key) >= .3f; };
    const bool bFire = Down(EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom) || Trigger(EKeys::Gamepad_RightTriggerAxis);
    bFocus = Down(EKeys::LeftShift, EKeys::RightShift) || Trigger(EKeys::Gamepad_LeftTriggerAxis);
    auto Axis = [PC](const FKey& Key) { return PC->GetInputAnalogKeyState(Key); };
    FVector Stick(-Axis(EKeys::Gamepad_LeftX), Axis(EKeys::Gamepad_LeftY), 0);
    constexpr double DeadZone = .2;
    const double Magnitude = Stick.Size();
    Stick = Magnitude > DeadZone ? Stick * ((FMath::Min(Magnitude, 1.0) - DeadZone) / ((1 - DeadZone) * Magnitude)) : FVector::ZeroVector;
    const FVector Digital(
        int(Down(EKeys::A,EKeys::Left) || PC->IsInputKeyDown(EKeys::Gamepad_DPad_Left)) - int(Down(EKeys::D,EKeys::Right) || PC->IsInputKeyDown(EKeys::Gamepad_DPad_Right)),
        int(Down(EKeys::W,EKeys::Up) || PC->IsInputKeyDown(EKeys::Gamepad_DPad_Up)) - int(Down(EKeys::S,EKeys::Down) || PC->IsInputKeyDown(EKeys::Gamepad_DPad_Down)), 0);
    const FVector Move = Digital.IsNearlyZero() ? Stick : Digital.GetClampedToMaxSize(1);
    if (State == EState::Ready && bFire) State = EState::Playing;
    if (State == EState::SectorClear && (Intermission -= DeltaSeconds) <= 0)
        StartNextSector();
    if (State == EState::Playing)
    {
        if (PC->WasInputKeyJustPressed(EKeys::Z) || PC->WasInputKeyJustPressed(EKeys::Gamepad_LeftShoulder)) Pulse();
        constexpr float Step = 1.0f / 120.0f;
        StepRemainder += FMath::Min(DeltaSeconds, 0.075f);
        while (StepRemainder >= Step && State == EState::Playing)
        {
            Advance(Step, Move, bFire);
            StepRemainder -= Step;
        }
    }
    UpdatePresentation(DeltaSeconds, State == EState::Playing ? Move : FVector::ZeroVector);
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

void ASpaceGameMode::ConfigureViewBounds()
{
    // Intersect the top and bottom of the perspective frustum with the XY flight plane.
    const auto* Lens = Camera->GetCameraComponent();
    const double TanX = FMath::Tan(FMath::DegreesToRadians(Lens->FieldOfView * .5));
    const double TanY = TanX / Lens->AspectRatio;
    const FVector Forward = Camera->GetActorForwardVector();
    const FVector Up = Camera->GetActorUpVector();
    const auto PlaneY = [&](double Vertical)
    {
        const FVector Ray = Forward + Up * (TanY * Vertical);
        return ChaseOffset.Y - ChaseOffset.Z * Ray.Y / Ray.Z;
    };
    // Keep the nose, wings and HUD clear without restricting the usable playfield.
    constexpr double HullPadding = 66;
    ViewY = FVector2D(PlaneY(-1) + HullPadding, PlaneY(.89) - HullPadding);
    const auto HalfWidthAtY = [&](double Y)
    {
        const double Depth = FVector::DotProduct(FVector(0, Y, 0) - ChaseOffset, Forward);
        return Depth * TanX - HullPadding;
    };
    ViewHalfWidth = FVector2D(HalfWidthAtY(ViewY.X), HalfWidthAtY(ViewY.Y));
    ProjectileY = FVector2D(PlaneY(-1) - MaxPanY - 160, PlaneY(1) + MaxPanY + 160);
    ProjectileHalfWidth = HalfWidthAtY(PlaneY(1)) + MaxPanX + HullPadding + 160;
}

void ASpaceGameMode::ClampPlayerToView()
{
    const double LocalY = FMath::Clamp(Player.Y - CameraAnchor.Y, ViewY.X, ViewY.Y);
    const double AlongView = (LocalY - ViewY.X) / (ViewY.Y - ViewY.X);
    const double HalfWidth = FMath::Lerp(ViewHalfWidth.X, ViewHalfWidth.Y, AlongView);
    Player.X = FMath::Clamp(Player.X, CameraAnchor.X - HalfWidth, CameraAnchor.X + HalfWidth);
    Player.Y = CameraAnchor.Y + LocalY;
}

void ASpaceGameMode::UpdatePresentation(float Dt, const FVector& Move)
{
    const FVector Target(FMath::Clamp(Player.X * .28, -MaxPanX, MaxPanX),
        FMath::Clamp(Player.Y * .25, -MaxPanY, MaxPanY), 0);
    CameraAnchor = FMath::VInterpTo(CameraAnchor, Target, Dt, 5);
    Camera->SetActorLocation(CameraAnchor + ChaseOffset);
    ClampPlayerToView();
    Bank = FMath::FInterpTo(Bank, Move.X * 25, Dt, 9);
    for (auto& Rock : Asteroids)
    {
        Rock.Pos.Y -= Rock.Speed * Dt;
        if (Rock.Pos.Y < -2200) Rock.Pos.Y += 4600;
        Rock.Angle = FMath::Fmod(Rock.Angle + Dt * Rock.Spin, 360.0);
    }
}

void ASpaceGameMode::Draw(EVisual Type, FVector Pos, FVector Scale, float Yaw, float BankAngle)
{
    const auto Index = static_cast<int32>(Type);
    FQuat Rotation(FRotator(BankAngle, Yaw, 0));
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
    Draw(EVisual::Planet, FVector(950, 1040, -1120), FVector(5.7));
    Draw(EVisual::Planet, FVector(-1140, 1850, -1410), FVector(1.6));
    if (Visuals[static_cast<int32>(EVisual::Asteroid)]->GetStaticMesh())
        for (const auto& Rock : Asteroids)
            Draw(EVisual::Asteroid, Rock.Pos, RockScale * Rock.Size, Rock.Angle, Rock.Angle * .63);
    if (State != EState::Lost && (Shield <= 0 || FMath::Fmod(Shield, 0.16f) < 0.08f))
    {
        Draw(EVisual::Player, Player, FVector(1.14), 0, Bank);
        Draw(EVisual::Laser, Player + FVector(-15,-43,-4), FVector(.075,.27,.07));
        Draw(EVisual::Laser, Player + FVector(15,-43,-4), FVector(.075,.27,.07));
        if (bFocus) Draw(EVisual::Laser, Player + FVector(0,0,34), FVector(.075));
    }
    for (const auto& Enemy : Enemies)
        Draw(static_cast<EVisual>(static_cast<int32>(EVisual::Scout) + Enemy.Kind), Enemy.Pos, FVector(1), 180);
    for (const auto& Shot : Shots)
    {
        const float Yaw = Shot.Vel.Rotation().Yaw - 90;
        Draw(Shot.bEnemy ? EVisual::Plasma : AmmoVisual(Shot.Color), Shot.Pos,
            Shot.bEnemy ? FVector(.135) : FVector(.075,.29,.075), Yaw);
    }
    for (const auto& Pickup : PowerUps)
        Draw(AmmoVisual(Pickup.Color), Pickup.Pos, FVector(.24, .24, .34) * (1 + .17 * FMath::Sin(Time * 12)));
    for (const auto& Spark : Sparks)
        Draw(EVisual::Spark, Spark.Pos, FVector(Spark.Life * .35f));
    for (int32 Index = 0; Index < static_cast<int32>(EVisual::Nebula); ++Index)
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
