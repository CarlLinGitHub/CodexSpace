#include "SpaceGameMode.h"

namespace
{
constexpr float HalfWidth = 680, HalfHeight = 430;
constexpr float BossAt = 52;
constexpr int32 WaveCount = 8;

FVector Direction(float Degrees)
{
    float Sin, Cos;
    FMath::SinCos(&Sin, &Cos, FMath::DegreesToRadians(Degrees));
    return FVector(Sin, -Cos, 0);
}
}

void ASpaceGameMode::Advance(float Dt, FVector Move, bool bFire)
{
    Time += Dt;
    Fire -= Dt;
    Shield -= Dt;
    Player += Move * (bFocus ? 235 : 545) * Dt;
    Player.X = FMath::Clamp(Player.X, -HalfWidth, HalfWidth);
    Player.Y = FMath::Clamp(Player.Y, -HalfHeight, HalfHeight);
    if (bFire && Fire <= 0)
    {
        Fire = bFocus ? .095f : .13f;
        const float Spread = bFocus ? 0.f : 100.f;
        Shots.Add({Player + FVector(-16,28,0), FVector(-Spread,1100,0), false});
        Shots.Add({Player + FVector(16,28,0), FVector(Spread,1100,0), false});
        if (Power >= 2)
            Shots.Add({Player + FVector(0,34,0), FVector(0,1210,0), false});
        if (Power == 3)
        {
            Shots.Add({Player + FVector(-26,20,0), FVector(-Spread - 75,1040,0), false});
            Shots.Add({Player + FVector(26,20,0), FVector(Spread + 75,1040,0), false});
        }
    }
    EnemiesTick(Dt);
    ShotsTick(Dt);
    if (State != EState::Playing) return;
    for (int32 Index = PowerUps.Num() - 1; Index >= 0; --Index)
    {
        auto& Pickup = PowerUps[Index];
        Pickup.Y -= 122 * Dt;
        if (FVector::DistSquared2D(Player, Pickup) < FMath::Square(39))
        {
            Power = FMath::Min(Power + 1, 3);
            Score += 250;
            Burst(Pickup, 12);
            PowerUps.RemoveAtSwap(Index, EAllowShrinking::No);
        }
        else if (Pickup.Y < -520) PowerUps.RemoveAtSwap(Index, EAllowShrinking::No);
    }
}

void ASpaceGameMode::Pulse()
{
    if (Pulses == 0) return;
    --Pulses;
    for (int32 Index = Shots.Num() - 1; Index >= 0; --Index)
    {
        if (!Shots[Index].bEnemy) continue;
        Burst(Shots[Index].Pos, 2);
        Shots.RemoveAtSwap(Index, EAllowShrinking::No);
    }
    Burst(Player, 45);
    Shield = FMath::Max(Shield, .5f);
}

void ASpaceGameMode::EnemiesTick(float Dt)
{
    if (Wave < WaveCount && Time >= 1.2f + Wave * 6.0f)
    {
        const int32 Count = 4 + Wave % 3;
        for (int32 Index = 0; Index < Count; ++Index)
        {
            const float X = (Index - (Count - 1) * .5f) * 176;
            const int32 Heavy = Wave >= 2 && (Index + Wave) % 3 == 0;
            Enemies.Add({FVector(X, 560 + (Index % 2) * 52, 0), X, 0, .95f + Index * .23f, Heavy ? 7 : 3, Heavy});
        }
        ++Wave;
    }
    if (Time >= BossAt && Wave == WaveCount)
    {
        Wave++;
        BossHP = MaxBossHP;
        Enemies.Add({FVector(0,600,0), 0, 0, 1.5f, MaxBossHP, 2});
    }
    for (int32 Index = Enemies.Num() - 1; Index >= 0; --Index)
    {
        auto& Enemy = Enemies[Index];
        Enemy.Age += Dt;
        const bool bBoss = Enemy.Kind == 2;
        if (bBoss)
        {
            Enemy.Pos.Y = FMath::Max(295.0, Enemy.Pos.Y - 210 * Dt);
            Enemy.Pos.X = 320 * FMath::Sin(Enemy.Age * .65f);
        }
        else
        {
            Enemy.Pos.Y -= (Enemy.Kind == 1 ? 133 : 205) * Dt;
            Enemy.Pos.X = Enemy.StartX + (Enemy.Kind == 1 ? 70 : 150) * FMath::Sin(Enemy.Age * 1.7f + Enemy.StartX * .004f);
        }
        if (Enemy.Pos.Y < -530)
        {
            Enemies.RemoveAtSwap(Index, EAllowShrinking::No);
            continue;
        }
        Enemy.Cooldown -= Dt;
        if (Enemy.Cooldown > 0 || Enemy.Pos.Y > 410 || (!bBoss && Enemy.Pos.Y < -285)) continue;
        const FVector Origin = Enemy.Pos - FVector(0, bBoss ? 82 : 25, 0);
        const FVector Aim = (Player - Origin).GetSafeNormal();
        if (bBoss)
        {
            const bool bRage = Enemy.HP <= MaxBossHP / 2;
            Enemy.Cooldown = bRage ? .38f : .59f;
            ++Pattern;
            if (Pattern % 3 == 0)
            {
                const int32 Count = bRage ? 22 : 15;
                for (int32 I = 0; I < Count; ++I)
                    Shots.Add({Origin, Direction(360.f * I / Count + Pattern * 13.f) * (bRage ? 326 : 250), true});
            }
            else
            {
                const int32 Wings = bRage ? 4 : 2;
                for (int32 I = -Wings; I <= Wings; ++I)
                    Shots.Add({Origin, Aim.RotateAngleAxis(I * 13.f, FVector::UpVector) * 400, true});
            }
        }
        else
        {
            Enemy.Cooldown = Enemy.Kind == 1 ? 1.13f : 1.48f;
            const int32 Wings = Enemy.Kind == 1 ? 1 : 0;
            for (int32 I = -Wings; I <= Wings; ++I)
                Shots.Add({Origin, Aim.RotateAngleAxis(I * 16.f, FVector::UpVector) * 315, true});
        }
    }
}

void ASpaceGameMode::ShotsTick(float Dt)
{
    const auto HitPlayer = [this]()
    {
        Burst(Player, 22);
        Shield = 1.55f;
        Power = FMath::Max(Power - 1, 1);
        if (--Lives == 0) State = EState::Lost;
    };
    for (int32 I = Shots.Num() - 1; I >= 0; --I)
    {
        auto& Shot = Shots[I];
        Shot.Pos += Shot.Vel * Dt;
        bool bRemove = FMath::Abs(Shot.Pos.X) > 810 || FMath::Abs(Shot.Pos.Y) > 610;
        if (!bRemove && Shot.bEnemy && Shield <= 0 && FVector::DistSquared2D(Player, Shot.Pos) < FMath::Square(bFocus ? 11 : 16))
        {
            HitPlayer();
            bRemove = true;
        }
        if (!bRemove && !Shot.bEnemy)
        {
            for (int32 J = Enemies.Num() - 1; J >= 0; --J)
            {
                auto& Enemy = Enemies[J];
                const float Radius = Enemy.Kind == 2 ? 76 : Enemy.Kind == 1 ? 41 : 30;
                if (FVector::DistSquared2D(Shot.Pos, Enemy.Pos) > FMath::Square(Radius)) continue;
                --Enemy.HP;
                if (Enemy.Kind == 2) BossHP = Enemy.HP;
                if (Enemy.HP == 0)
                {
                    Score += Enemy.Kind == 2 ? 5000 : Enemy.Kind == 1 ? 350 : 100;
                    if (Enemy.Kind != 2 && ++Kills % 4 == 0) PowerUps.Add(Enemy.Pos);
                    Burst(Enemy.Pos, Enemy.Kind == 2 ? 80 : 18);
                    if (Enemy.Kind == 2 && State == EState::Playing) State = EState::Won;
                    Enemies.RemoveAtSwap(J, EAllowShrinking::No);
                }
                else Burst(Shot.Pos, 2);
                bRemove = true;
                break;
            }
        }
        if (bRemove) Shots.RemoveAtSwap(I, EAllowShrinking::No);
        if (State != EState::Playing) return;
    }
    if (Shield <= 0)
    {
        for (const auto& Enemy : Enemies)
        {
            if (FVector::DistSquared2D(Player, Enemy.Pos) >= FMath::Square(Enemy.Kind == 2 ? 81 : 37)) continue;
            HitPlayer();
            break;
        }
    }
}

void ASpaceGameMode::Burst(FVector Pos, int32 Count)
{
    while (Count-- > 0 && Sparks.Num() < 150)
        Sparks.Add({Pos + FVector(0,0,20), Direction(FMath::FRandRange(0.f,360.f)) * FMath::FRandRange(40.f,320.f), FMath::FRandRange(.16f,.54f)});
}
