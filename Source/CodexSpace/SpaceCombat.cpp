#include "SpaceGameMode.h"

namespace
{
struct FSector
{
    const TCHAR* Name;
    float WaveGap, Spacing, Stagger, Sway, Cadence;
    int32 BossHealth;
};
constexpr FSector Sectors[] = {
    {TEXT("OUTER RIM"),       4.8f, 174.f,  26.f, 0.78f, 1.12f,  44},
    {TEXT("ASTEROID BELT"),   4.5f, 150.f,  52.f, 1.24f, 1.00f,  66},
    {TEXT("ION NEBULA"),      4.3f, 146.f, -49.f, 1.40f, 0.94f,  88},
    {TEXT("SIEGE LINE"),      4.0f, 155.f,  35.f, 0.66f, 0.88f, 114},
    {TEXT("DREADNOUGHT CORE"),3.8f, 138.f,  41.f, 1.55f, 0.82f, 150},
};
static_assert(UE_ARRAY_COUNT(Sectors) == ASpaceGameMode::SectorCount);

FVector Direction(float Degrees)
{
    float Sin, Cos;
    FMath::SinCos(&Sin, &Cos, FMath::DegreesToRadians(Degrees));
    return FVector(Sin, -Cos, 0);
}
}

const TCHAR* ASpaceGameMode::SectorName() const
{
    return Sectors[Sector - 1].Name;
}

void ASpaceGameMode::StartNextSector()
{
    ++Sector;
    Wave = Pattern = BossHP = BossMaxHP = 0;
    Time = Fire = StepRemainder = 0;
    Shield = 1.2f;
    Player = FVector(0, -350, 0);
    Enemies.Reset();
    Shots.Reset();
    PowerUps.Reset();
    Pulses = FMath::Min(Pulses + 1, 2);
    State = EState::Playing;
}

FLinearColor ASpaceGameMode::AmmoTint(EAmmo Color)
{
    constexpr FLinearColor Tints[] = {{1.f,.76f,.06f}, {1.f,.12f,.035f}, {.07f,.46f,1.f}};
    return Tints[static_cast<int32>(Color)];
}

ASpaceGameMode::EVisual ASpaceGameMode::AmmoVisual(EAmmo Color)
{
    return static_cast<EVisual>(static_cast<int32>(EVisual::Yellow) + static_cast<int32>(Color));
}

void ASpaceGameMode::Advance(float Dt, FVector Move, bool bFire)
{
    Time += Dt;
    Fire -= Dt;
    Shield -= Dt;
    Player += Move * (bFocus ? 235 : 545) * Dt;
    ClampPlayerToView();
    if (bFire && Fire <= 0)
    {
        Fire = bFocus ? .095f : .13f;
        FireVolley();
    }
    EnemiesTick(Dt);
    ShotsTick(Dt);
    if (State != EState::Playing) return;
    for (int32 Index = PowerUps.Num() - 1; Index >= 0; --Index)
    {
        auto& Pickup = PowerUps[Index];
        Pickup.Pos.Y -= 122 * Dt;
        if (FVector::DistSquared2D(Player, Pickup.Pos) < FMath::Square(39))
        {
            if (ActiveAmmo != Pickup.Color)
            {
                for (auto& Level : AmmoLevels) Level = 0;
                ActiveAmmo = Pickup.Color;
            }
            auto& Level = AmmoLevels[static_cast<int32>(ActiveAmmo)];
            Level = FMath::Min(Level + 1, MaxAmmoLevel);
            Score += 250;
            Burst(Pickup.Pos, 12);
            PowerUps.RemoveAtSwap(Index, EAllowShrinking::No);
        }
        else if (Pickup.Pos.Y < ProjectileY.X) PowerUps.RemoveAtSwap(Index, EAllowShrinking::No);
    }
}

void ASpaceGameMode::FireVolley()
{
    struct FProfile { float SpreadPerLevel; int32 DamagePerLevel; };
    constexpr FProfile Profiles[] = {{7.f, 0}, {2.f, 3}, {4.f, 1}};
    static_assert(UE_ARRAY_COUNT(Profiles) == static_cast<int32>(EAmmo::Count));
    const int32 Index = static_cast<int32>(ActiveAmmo);
    const auto& Profile = Profiles[Index];
    const int32 Level = AmmoLevels[Index];
    const int32 Damage = 1 + Level * Profile.DamagePerLevel;
    const float AngleStep = Profile.SpreadPerLevel * (bFocus ? .67f : 1.f);
    const FVector Muzzle = Player + FVector(0, 45, 10);
    const uint32 Volley = ++VolleyId;
    for (int32 Lane = -Level; Lane <= Level; ++Lane)
        Shots.Add({Muzzle, Direction(180.f - Lane * AngleStep) * 1150, false, ActiveAmmo, Damage, Volley});
}

bool ASpaceGameMode::FEnemy::RegisterHit(uint32 Volley)
{
    // Active shots leave the arena in under two seconds; 64 volley IDs cover their lifetime.
    if (Volley > LatestVolley)
    {
        const uint32 Delta = Volley - LatestVolley;
        VolleyHits = Delta < 64 ? VolleyHits << Delta : 0;
        LatestVolley = Volley;
    }
    const uint32 VolleyAge = LatestVolley - Volley;
    if (VolleyAge >= 64) return false;
    const uint64 Bit = uint64(1) << VolleyAge;
    if (VolleyHits & Bit) return false;
    VolleyHits |= Bit;
    return true;
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
    const auto& Profile = Sectors[Sector - 1];
    if (Wave < WavesPerSector && Time >= 1.2f + Wave * Profile.WaveGap)
    {
        const int32 Count = FMath::Min(4 + (Wave + Sector - 1) % 3 + (Sector >= 4), 7);
        for (int32 Index = 0; Index < Count; ++Index)
        {
            const float Slot = Index - (Count - 1) * .5f;
            const float Shift = Sector == 3 ? (Wave % 2 ? 130.f : -130.f) : 0.f;
            const float X = Slot * Profile.Spacing + Shift;
            const int32 Heavy = Wave + Sector >= 4 && (Index + Wave) % (Sector >= 4 ? 2 : 3) == 0;
            Enemies.Add({FVector(X, 560 + Profile.Stagger * FMath::Abs(Slot), 0), X, 0, .85f + Index * .20f, Heavy ? 7 : 3, Heavy});
        }
        ++Wave;
    }
    if (Time >= 2.2f + WavesPerSector * Profile.WaveGap && Wave == WavesPerSector)
    {
        ++Wave;
        BossHP = BossMaxHP = Profile.BossHealth;
        Enemies.Add({FVector(0,600,0), 0, 0, 1.5f, BossHP, 2});
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
            Enemy.Pos.X = Enemy.StartX + Profile.Sway * (Enemy.Kind == 1 ? 70 : 130) * FMath::Sin(Enemy.Age * 1.7f + Enemy.StartX * .004f);
        }
        if (Enemy.Pos.Y < ProjectileY.X)
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
            const bool bRage = Enemy.HP <= BossMaxHP / 2;
            Enemy.Cooldown = (bRage ? .58f : .87f) * Profile.Cadence;
            ++Pattern;
            const auto Fan = [&](FVector Source, FVector Heading, int32 Wings, float Step, float Speed)
            {
                for (int32 I = -Wings; I <= Wings; ++I)
                    Shots.Add({Source, Heading.RotateAngleAxis(I * Step, FVector::UpVector) * Speed, true});
            };
            const auto Ring = [&](int32 Count, float Speed)
            {
                for (int32 I = 0; I < Count; ++I)
                    Shots.Add({Origin, Direction(360.f * I / Count + Pattern * 17.f) * Speed, true});
            };
            switch (Sector)
            {
            case 1: Fan(Origin, Aim, bRage ? 3 : 1, 18, 320); break;
            case 2: if ((Pattern & 1) == 0) Ring(bRage ? 16 : 12, 275); else Fan(Origin, Aim, 2, 15, 330); break;
            case 3: Fan(Origin, Direction(50.f * FMath::Sin(Pattern * .64f)), bRage ? 4 : 2, 12, 340); break;
            case 4:
                for (float Side : {-1.f, 1.f})
                    Fan(Origin + FVector(Side * 54, 0, 0), Direction(Side * 25), bRage ? 3 : 2, 12, 355);
                break;
            default:
                if (Pattern % 3 == 0) Ring(bRage ? 22 : 16, bRage ? 330 : 275);
                else Fan(Origin, Aim.RotateAngleAxis(Pattern % 2 ? 15 : -15, FVector::UpVector), bRage ? 4 : 3, 13, 390);
                break;
            }
        }
        else
        {
            Enemy.Cooldown = Profile.Cadence * (Enemy.Kind == 1 ? 1.13f : 1.48f);
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
        if (--Lives == 0) State = EState::Lost;
    };
    for (int32 I = Shots.Num() - 1; I >= 0; --I)
    {
        auto& Shot = Shots[I];
        Shot.Pos += Shot.Vel * Dt;
        bool bRemove = FMath::Abs(Shot.Pos.X) > ProjectileHalfWidth || Shot.Pos.Y < ProjectileY.X || Shot.Pos.Y > ProjectileY.Y;
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
                if (!Enemy.RegisterHit(Shot.Volley)) continue;
                Enemy.HP = FMath::Max(0, Enemy.HP - Shot.Damage);
                if (Enemy.Kind == 2) BossHP = Enemy.HP;
                if (Enemy.HP == 0)
                {
                    Score += Enemy.Kind == 2 ? 5000 : Enemy.Kind == 1 ? 350 : 100;
                    if (Enemy.Kind != 2 && FMath::FRand() < .32f)
                        PowerUps.Add({Enemy.Pos, static_cast<EAmmo>(FMath::RandHelper(static_cast<int32>(EAmmo::Count)))});
                    Burst(Enemy.Pos, Enemy.Kind == 2 ? 80 : 18);
                    if (Enemy.Kind == 2)
                    {
                        State = Sector == SectorCount ? EState::Won : EState::SectorClear;
                        Intermission = 2.6f;
                    }
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
