#include "SpaceGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

void ASpaceHUD::DrawHUD()
{
    Super::DrawHUD();
    const auto* Game = Cast<ASpaceGameMode>(GetWorld()->GetAuthGameMode());
    if (!Game || !Canvas) return;
    const float W = FMath::Min(float(Canvas->SizeX), Canvas->SizeY * 1.6f);
    const float H = FMath::Min(float(Canvas->SizeY), Canvas->SizeX / 1.6f);
    const float X = (Canvas->SizeX - W) * .5f;
    const float Y = (Canvas->SizeY - H) * .5f;
    const float S = FMath::Max(.45f, W / 1120);
    auto* Font = GEngine->GetLargeFont();
    const FLinearColor Ice(.22f,.9f,1), White(.9f,.95f,1), Pink(1,.25f,.46f), Dim(.47f,.61f,.76f);
    const auto Center = [&](const FString& Text, float Row, float Scale, FLinearColor Color)
    {
        float TW, TH;
        GetTextSize(Text, TW, TH, Font, S * Scale);
        DrawText(Text, Color, X + (W - TW) * .5f, Y + H * Row, Font, S * Scale);
    };
    DrawRect(FLinearColor(.014f,.027f,.068f,.88f), X, Y, W, 53 * S);
    DrawRect(Ice, X, Y + 52 * S, W, S);
    DrawText(FString::Printf(TEXT("SCORE  %07d"), Game->Score), White, X + 20 * S, Y + 14 * S, Font, .85f * S);
    Center(Game->Wave == 9 ? TEXT("DREADNOUGHT") : FString::Printf(TEXT("SECTOR  %02d / 08"), Game->Wave), 14 * S / H, .78f, Ice);
    DrawText(FString::Printf(TEXT("SHIPS  %d"), Game->Lives), White, X + W - 119 * S, Y + 14 * S, Font, .85f * S);
    DrawText(FString::Printf(TEXT("POWER %d/3    PULSE %d"), Game->Power, Game->Pulses), Ice, X + 20 * S, Y + 64 * S, Font, .65f * S);
    if (Game->BossHP > 0)
    {
        DrawRect(FLinearColor(.15f,.035f,.08f), X + W * .26f, Y + 64 * S, W * .48f, 7 * S);
        DrawRect(Pink, X + W * .26f, Y + 64 * S, W * .48f * Game->BossHP / ASpaceGameMode::MaxBossHP, 7 * S);
    }
    if (Game->State == ASpaceGameMode::EState::Playing) return;
    DrawRect(FLinearColor(.012f,.019f,.053f,.88f), X + W*.12f, Y + H*.26f, W*.76f, H*.44f);
    DrawRect(Ice, X + W*.12f, Y + H*.26f, W*.76f, 2 * S);
    const bool bReady = Game->State == ASpaceGameMode::EState::Ready;
    const bool bWon = Game->State == ASpaceGameMode::EState::Won;
    Center(TEXT("ORBITAL DEFENSE   /   ARCADE FLIGHT"), .31f, .77f, Dim);
    Center(bReady ? TEXT("STELLAR THUNDER") : bWon ? TEXT("SECTOR CLEARED") : TEXT("SIGNAL LOST"), .385f, 2.1f, bReady || bWon ? Ice : Pink);
    Center(bReady ? TEXT("SURVIVE EIGHT WAVES. DESTROY THE DREADNOUGHT.") : FString::Printf(TEXT("FINAL SCORE   %07d"), Game->Score), .49f, .72f, White);
    Center(bReady ? TEXT("SPACE  //  LAUNCH & FIRE") : TEXT("R  //  RETURN TO LAUNCH"), .56f, 1.0f, Ice);
    Center(TEXT("WASD / ARROWS  MOVE    SHIFT  FOCUS    Z  PULSE    R  RESTART"), .635f, .64f, Dim);
}
