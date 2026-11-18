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

    enum class EState : uint8 { Ready, Playing, Lost, Won };
    EState State = EState::Ready;
    int32 Score = 0, Lives = 3, Wave = 0, BossHP = 0;
    int32 Power = 1, Pulses = 2;
    static constexpr int32 MaxBossHP = 100;
    bool bFocus = false;

private:
    enum class EVisual : uint8 { Player, Scout, Heavy, Boss, Laser, Plasma, Spark, Star, Planet, Count };
    struct FEnemy { FVector Pos; float StartX, Age, Cooldown; int32 HP, Kind; };
    struct FShot { FVector Pos, Vel; bool bEnemy; };
    struct FStar { FVector Pos; double Speed, Size; };
    TArray<FTransform> Frames[static_cast<int32>(EVisual::Count)];
    float ShipScales[4] = {};
    FVector ShipCenters[4] = {};
    struct FSpark { FVector Pos, Vel; float Life; };

    UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Visuals;
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
    TArray<FEnemy> Enemies;
    TArray<FShot> Shots;
    TArray<FStar> Stars;
    TArray<FSpark> Sparks;
    TArray<FVector> PowerUps;
    FVector Player = FVector::ZeroVector;
    float Time = 0, Fire = 0, Shield = 0, StepRemainder = 0;
    int32 Pattern = 0, Kills = 0;

    void Reset();
    void Advance(float Dt, FVector Move, bool bFire);
    void Pulse();
    void EnemiesTick(float Dt);
    void ShotsTick(float Dt);
    void Burst(FVector Pos, int32 Count);
    void Render();
    void Draw(EVisual Type, FVector Pos, FVector Scale, float Yaw = 0);
};

UCLASS()
class ASpaceHUD : public AHUD
{
    GENERATED_BODY()
    void DrawHUD() override;
};
