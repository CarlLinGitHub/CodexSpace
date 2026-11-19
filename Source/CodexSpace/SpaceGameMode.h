#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "SpaceGameMode.generated.h"

class ACameraActor;
class UInstancedStaticMeshComponent;
class UStaticMesh;

UCLASS()
class ASpaceGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ASpaceGameMode();
    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;

    enum class EState : uint8 { Ready, Playing, SectorClear, Lost, Won };
    EState State = EState::Ready;
    int32 Score = 0, Lives = 3, Wave = 0, BossHP = 0;
    enum class EAmmo : uint8 { Yellow, Red, Blue, Count };
    static constexpr int32 MaxAmmoLevel = 5;
    static FLinearColor AmmoTint(EAmmo Color);
    EAmmo ActiveAmmo = EAmmo::Yellow;
    int32 AmmoLevels[static_cast<int32>(EAmmo::Count)] = {};
    int32 Pulses = 2;
    static constexpr int32 SectorCount = 5, WavesPerSector = 4;
    int32 Sector = 1, BossMaxHP = 0;
    const TCHAR* SectorName() const;
    bool bFocus = false;

private:
    enum class EVisual : uint8 { Player, Scout, Heavy, Boss, Laser, Plasma, Spark, Star, Planet, Asteroid, Yellow, Red, Blue, Nebula, Count };
    struct FEnemy
    {
        FVector Pos;
        float StartX, Age, Cooldown;
        int32 HP, Kind;
        uint32 LatestVolley = 0;
        uint64 VolleyHits = 0;
        bool RegisterHit(uint32 Volley);
    };
    struct FShot { FVector Pos, Vel; bool bEnemy; EAmmo Color = EAmmo::Yellow; int32 Damage = 1; uint32 Volley = 0; };
    struct FPickup { FVector Pos; EAmmo Color; };
    struct FStar { FVector Pos; double Speed, Size; };
    struct FAsteroid { FVector Pos; double Speed, Size, Spin, Angle; };
    TArray<FTransform> Frames[static_cast<int32>(EVisual::Count)];
    float ShipScales[4] = {};
    FVector ShipCenters[4] = {};
    FVector RockScale = FVector::OneVector;
    FVector CameraAnchor = FVector::ZeroVector;
    FVector2D ViewY = FVector2D::ZeroVector;
    FVector2D ViewHalfWidth = FVector2D::ZeroVector;
    FVector2D ProjectileY = FVector2D::ZeroVector;
    double ProjectileHalfWidth = 0;
    double Bank = 0;
    struct FSpark { FVector Pos, Vel; float Life; };

    UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Visuals;
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
    TArray<FEnemy> Enemies;
    TArray<FShot> Shots;
    TArray<FStar> Stars;
    TArray<FAsteroid> Asteroids;
    TArray<FSpark> Sparks;
    TArray<FPickup> PowerUps;
    FVector Player = FVector::ZeroVector;
    float Time = 0, Fire = 0, Shield = 0, StepRemainder = 0;
    float Intermission = 0;
    int32 Pattern = 0;
    uint32 VolleyId = 0;

    void Reset();
    void StartNextSector();
    void Advance(float Dt, FVector Move, bool bFire);
    void FireVolley();
    static EVisual AmmoVisual(EAmmo Color);
    void Pulse();
    void EnemiesTick(float Dt);
    void ShotsTick(float Dt);
    void Burst(FVector Pos, int32 Count);
    void Render();
    void ConfigureViewBounds();
    void ClampPlayerToView();
    void UpdatePresentation(float Dt, const FVector& Move);
    void Draw(EVisual Type, FVector Pos, FVector Scale, float Yaw = 0, float BankAngle = 0);
};

UCLASS()
class ASpaceHUD : public AHUD
{
    GENERATED_BODY()
    void DrawHUD() override;
};
