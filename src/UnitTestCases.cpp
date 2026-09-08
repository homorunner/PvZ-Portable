#include "UnitTestRunner.h"
#include "GameConstants.h"
#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/Zombie.h"
#include "Lawn/System/PlayerInfo.h"
#include "PvzpLib/Reanimator.h"
#include <array>
#include <format>

namespace
{
ZombieID target;
PlantID shooter;
float targetX;
int shots, lastHealth;
bool sawDeath;
constexpr std::array shotTicks{0, 17, 33, 50};

void SetupFixture(UnitTestRunner& runner, LawnApp& app)
{
	shots = 0;
	lastHealth = 80;
	sawDeath = false;
	app.mGameMode = GameMode::GAMEMODE_ADVENTURE;
	app.mPlayerInfo->mLevel = 11;
	app.MakeNewBoard();
	Board& board = *app.mBoard;
	board.InitLevel();
	app.mGameScene = GameScenes::SCENE_PLAYING;
	board.mMouseVisible = false;
	runner.Check(board.mBackground == BackgroundType::BACKGROUND_2_NIGHT, "Night lawn");
	Zombie* zombie = board.AddZombieInRow(ZombieType::ZOMBIE_NORMAL, 2, 0);
	target = static_cast<ZombieID>(board.mZombies.DataArrayGetID(zombie));
	targetX = static_cast<float>(board.GridToPixelX(7, 2));
	zombie->mPosX = targetX;
	zombie->mX = static_cast<int>(targetX);
	zombie->mVelX = 0;
	zombie->mOriginalAnimRate = 0;
	app.ReanimationTryToGet(zombie->mBodyReanimID)->mAnimRate = 0;
	zombie->mBodyHealth = zombie->mBodyMaxHealth = 80;
	zombie->mDroppedLoot = true;
	Plant* plant = board.AddPlant(8, 2, SeedType::SEED_LEFTPEATER);
	shooter = static_cast<PlantID>(board.mPlants.DataArrayGetID(plant));
}

void SetupBurst(UnitTestRunner& runner, LawnApp& app)
{
	ENABLE_LEFTPEATER_PLANTING_BURST = true;
	SetupFixture(runner, app);
	runner.Check(shots == 1, "Immediate planting shot at tick 0");
}

void SetupDisabledBurst(UnitTestRunner& runner, LawnApp& app)
{
	ENABLE_LEFTPEATER_PLANTING_BURST = false;
	SetupFixture(runner, app);
	Plant* plant = app.mBoard->mPlants.DataArrayTryToGet(static_cast<unsigned int>(shooter));
	runner.Check(shots == 0, "Disabled: no immediate planting shot");
	runner.Check(plant->mStateCountdown == 0 && plant->mShootingCounter == 0,
		"Disabled: no planting burst or shooting animation scheduled");
	runner.Check(plant->mLaunchCounter >= 0 && plant->mLaunchCounter <= 150,
		"Disabled: original random initial launch counter range");
	// Control only the random initial delay; keep normal targeting and shooting intact.
	plant->mLaunchCounter = 150;
}

void DisabledShot(UnitTestRunner& runner, const Projectile& projectile)
{
	++shots;
	runner.Check(shots <= 2 && runner.Tick() == 150 + 25 * (shots - 1),
		std::format("Disabled: normal shot {} at tick {}", shots, runner.Tick()));
	runner.Check(projectile.mRow == 2 && projectile.mProjectileType == ProjectileType::PROJECTILE_PEA,
		"Disabled: normal attack fires a pea in row 3");
}

void UpdateDisabledBurst(UnitTestRunner& runner, Board& board)
{
	Zombie* zombie = board.ZombieTryToGet(target);
	Plant* plant = board.mPlants.DataArrayTryToGet(static_cast<unsigned int>(shooter));
	if (runner.Tick() == 50)
	{
		runner.Check(shots == 0 && zombie && zombie->mBodyHealth == 80,
			"Disabled: no bonus damage during the planting burst window");
		runner.Check(plant && plant->mLaunchCounter == 100 && plant->mStateCountdown == 0,
			"Disabled: ordinary countdown runs without burst freeze or reset");
	}
	if (runner.Tick() == 200)
	{
		runner.Check(shots == 2, "Disabled: normal double shot remains functional");
		runner.Check(zombie && !zombie->mDead && zombie->mBodyHealth == 40 && zombie->mPosX == targetX,
			"Disabled: stationary target survives with 40 health after normal double shot");
		ENABLE_LEFTPEATER_PLANTING_BURST = true;
		runner.Finish();
	}
}

void Shot(UnitTestRunner& runner, const Projectile& projectile)
{
	++shots;
	runner.Check(shots <= 4 && runner.Tick() == shotTicks[shots <= 4 ? shots - 1 : 3],
		std::format("Shot {} created (expected ticks 0,17,33,50)", shots));
	runner.Check(projectile.mRow == 2 && projectile.mProjectileType == ProjectileType::PROJECTILE_PEA,
		"Pea fired in row 3");
}

void UpdateBurst(UnitTestRunner& runner, Board& board)
{
	Zombie* zombie = board.ZombieTryToGet(target);
	for (Projectile* projectile : board.mProjectiles)
		if (projectile->mProjectileAge == 1)
			runner.Check(projectile->mMotionType == ProjectileMotion::MOTION_BACKWARDS, "Pea travels left");
	if (zombie)
	{
		if (zombie->mPosX != targetX)
		{
			runner.Check(false, "Zombie must remain stationary in column 8");
			runner.Finish();
			return;
		}
		if (zombie->mBodyHealth != lastHealth)
		{
			runner.Log(std::format("DAMAGE tick={} health {} -> {}", runner.Tick(), lastHealth, zombie->mBodyHealth));
			lastHealth = zombie->mBodyHealth;
		}
		sawDeath |= zombie->mDead || zombie->mBodyHealth <= 0;
	}
	else sawDeath = true;
	if (runner.Tick() == 1 || runner.Tick() == 17 || runner.Tick() == 33)
		runner.Check(zombie && zombie->mBodyHealth == 80 - 20 * shots && !sawDeath,
			std::format("Shot {} dealt 20 damage; target still alive", shots));
	if (runner.Tick() == 50)
	{
		runner.Check(shots == 4 && zombie && zombie->mBodyHealth == 0,
			"Fourth shot killed the target at tick 50");
		Plant* plant = board.mPlants.DataArrayTryToGet(static_cast<unsigned int>(shooter));
		runner.Check(plant && !plant->mDead && plant->mLaunchCounter == 150,
			"Launch counter reset to 150 at tick 50");
		Reanimation* head = plant ? board.mApp->ReanimationTryToGet(plant->mHeadReanimID) : nullptr;
		int frameStart = -1, frameCount = -1;
		if (head && head->TrackExists("anim_shooting"))
			head->GetFramesForLayer("anim_shooting", frameStart, frameCount);
		runner.Check(head && head->mFrameStart == frameStart && head->mFrameCount == frameCount &&
			head->mLoopType == ReanimLoopType::REANIM_PLAY_ONCE_AND_HOLD && head->mAnimRate == 45.0f,
			"Fourth shot plays the shooting animation");
	}
	if (runner.Tick() == 51)
	{
		Plant* plant = board.mPlants.DataArrayTryToGet(static_cast<unsigned int>(shooter));
		runner.Check(plant && !plant->mDead && plant->mLaunchCounter == 149,
			"Launch counter decremented to 149 at tick 51");
	}
	// Observe past the burst and projectile travel, but before the next normal volley.
	if (runner.Tick() == 140)
	{
		runner.Check(shots == 4, "Exactly four shots, no extra burst projectiles");
		runner.Check(sawDeath, "80-health zombie died");
		runner.Finish();
	}
}
}

void RegisterLawnTests(UnitTestRunner& runner)
{
	runner.Register({"leftpeater planting burst", SetupBurst, UpdateBurst, Shot, 300});
	runner.Register({"leftpeater planting burst disabled", SetupDisabledBurst, UpdateDisabledBurst, DisabledShot, 300});
}
