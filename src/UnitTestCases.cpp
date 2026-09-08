#include "UnitTestRunner.h"
#include "GameConstants.h"
#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/Challenge.h"
#include "Lawn/GridItem.h"
#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/Zombie.h"
#include "Lawn/System/PlayerInfo.h"
#include "PvzpLib/Reanimator.h"
#include <array>
#include <algorithm>
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

std::array<PlantID, 11> healingPlants;
bool savedHealing;

template<bool enabled>
void SetupHealing(UnitTestRunner& runner, LawnApp& app)
{
	savedHealing = ENABLE_PLANTERN_HEALING;
	ENABLE_PLANTERN_HEALING = enabled;
	app.mGameMode = GameMode::GAMEMODE_ADVENTURE;
	app.mPlayerInfo->mLevel = 11;
	app.MakeNewBoard();
	Board& board = *app.mBoard;
	board.InitLevel();
	app.mGameScene = GameScenes::SCENE_PLAYING;
	board.mMouseVisible = false;
	int index = 0;
	auto add = [&](int col, int row, SeedType seed, int missingHealth)
	{
		Plant* plant = board.AddPlant(col, row, seed);
		plant->mPlantHealth = plant->mPlantMaxHealth - missingHealth;
		healingPlants[index++] = static_cast<PlantID>(board.mPlants.DataArrayGetID(plant));
		return plant;
	};
	for (int row = 1; row <= 3; ++row)
		for (int col = 3; col <= 5; ++col)
			if (col != 4 || row != 2)
				add(col, row, SeedType::SEED_WALLNUT, index == 0 ? 10 : 100);
	Plant* plantern = add(4, 2, SeedType::SEED_PLANTERN, 100);
	add(4, 2, SeedType::SEED_PUMPKINSHELL, 100);
	add(6, 2, SeedType::SEED_WALLNUT, 100);
	runner.Check(plantern->mLaunchCounter == 100, "Plantern starts with a 100-tick healing timer");
}

template<bool enabled>
void UpdateHealing(UnitTestRunner& runner, Board& board)
{
	const int tick = runner.Tick();
	for (int i = 0; i < static_cast<int>(healingPlants.size()); ++i)
	{
		Plant* plant = board.mPlants.DataArrayTryToGet(static_cast<unsigned int>(healingPlants[i]));
		const int healed = enabled && i < 8 ? 45 * (tick / 100) : 0;
		const int missingHealth = std::max(0, (i == 0 ? 10 : 100) - healed);
		runner.Check(plant && !plant->mDead && plant->mPlantHealth == plant->mPlantMaxHealth - missingHealth,
			std::format("Healing {}: plant {} health at tick {}", enabled, i, tick));
		if (i == 8)
			runner.Check(plant && plant->mLaunchCounter == (enabled ? 100 - tick % 100 : 100),
				"Plantern timer counts down and resets only when enabled");
	}
	if (tick == 200)
	{
		ENABLE_PLANTERN_HEALING = savedHealing;
		runner.Finish();
	}
}

template<bool enabled>
void SetupGreenVases(UnitTestRunner& runner, LawnApp& app)
{
	const bool saved = ENABLE_PLANTERN_GREEN_VASE;
	ENABLE_PLANTERN_GREEN_VASE = enabled;
	app.mGameMode = GameMode::GAMEMODE_SCARY_POTTER_ENDLESS;
	app.MakeNewBoard();
	Board& board = *app.mBoard;
	board.InitLevel();
	app.mGameScene = GameScenes::SCENE_PLAYING;
	board.mMouseVisible = false;
	for (int stage : {0, 1, 8, 9})
	{
		// InitLevel populates stage zero; isolate each real population call.
		board.mGridItems.DataArrayFreeAll();
		board.mChallenge->mSurvivalStage = stage;
		board.mChallenge->ScaryPotterPopulate();
		std::array<int, 8> seeds{};
		constexpr std::array seedTypes{SeedType::SEED_LEFTPEATER, SeedType::SEED_SNOWPEA,
			SeedType::SEED_PEASHOOTER, SeedType::SEED_THREEPEATER, SeedType::SEED_SQUASH,
			SeedType::SEED_POTATOMINE, SeedType::SEED_WALLNUT, SeedType::SEED_PLANTERN};
		int total = 0, green = 0, greenPlantern = 0, sun = 0;
		int normal = 0, garg = 0, pail = 0, jack = 0;
		bool valid = true;
		bool occupied[9][5]{};
		for (GridItem* pot : board.mGridItems)
		{
			++total;
			valid &= !pot->mDead && pot->mGridItemType == GridItemType::GRIDITEM_SCARY_POT;
			if (pot->mGridX < 2 || pot->mGridX > 8 || pot->mGridY < 0 || pot->mGridY > 4)
				valid = false;
			else
			{
				valid &= !occupied[pot->mGridX][pot->mGridY];
				occupied[pot->mGridX][pot->mGridY] = true;
			}
			const bool leaf = pot->mGridItemState == GridItemState::GRIDITEM_STATE_SCARY_POT_LEAF;
			green += leaf;
			valid &= leaf || pot->mGridItemState == GridItemState::GRIDITEM_STATE_SCARY_POT_QUESTION;
			valid &= !leaf || pot->mScaryPotType == ScaryPotType::SCARYPOT_SEED;
			if (pot->mScaryPotType == ScaryPotType::SCARYPOT_SEED)
			{
				valid &= pot->mZombieType == ZombieType::ZOMBIE_INVALID;
				for (int i = 0; i < static_cast<int>(seedTypes.size()); ++i)
					seeds[i] += pot->mSeedType == seedTypes[i];
				greenPlantern += leaf && pot->mSeedType == SeedType::SEED_PLANTERN;
			}
			else if (pot->mScaryPotType == ScaryPotType::SCARYPOT_ZOMBIE)
			{
				valid &= pot->mSeedType == SeedType::SEED_NONE;
				normal += pot->mZombieType == ZombieType::ZOMBIE_NORMAL;
				garg += pot->mZombieType == ZombieType::ZOMBIE_GARGANTUAR;
				pail += pot->mZombieType == ZombieType::ZOMBIE_PAIL;
				jack += pot->mZombieType == ZombieType::ZOMBIE_JACK_IN_THE_BOX;
			}
			else
			{
				++sun;
				valid &= pot->mScaryPotType == ScaryPotType::SCARYPOT_SUN &&
					pot->mSunCount >= 1 && pot->mSunCount <= 3 &&
					pot->mSeedType == SeedType::SEED_NONE && pot->mZombieType == ZombieType::ZOMBIE_INVALID;
			}
		}
		runner.Check(valid && total == 35 && board.mChallenge->mScaryPotterPots == 35 && green == 2,
			std::format("Green vases {} stage {}: 35 unique valid vases, exactly two green", enabled, stage));
		runner.Check(seeds == std::array{6, 2, 1, 2, 5, 1, 1, 1} && sun == 1 && pail == 5 && jack == 1 &&
			garg == 1 + std::min(stage, 8) && normal == 8 - std::min(stage, 8),
			std::format("Stage {}: original contents and capped endless difficulty", stage));
		if constexpr (enabled)
			runner.Check(greenPlantern == 1, std::format("Stage {}: existing Plantern is green", stage));
	}
	ENABLE_PLANTERN_GREEN_VASE = saved;
	runner.Finish();
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
		runner.Check(plant && !plant->mDead && plant->mLaunchCounter == 120,
			"Launch counter reset to 120 at tick 50");
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
		runner.Check(plant && !plant->mDead && plant->mLaunchCounter == 119,
			"Launch counter decremented to 119 at tick 51");
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
	runner.Register({"plantern healing", SetupHealing<true>, UpdateHealing<true>, nullptr, 250});
	runner.Register({"plantern healing disabled", SetupHealing<false>, UpdateHealing<false>, nullptr, 250});
	runner.Register({"plantern green vases", SetupGreenVases<true>, nullptr, nullptr, 1});
	runner.Register({"plantern green vases disabled", SetupGreenVases<false>, nullptr, nullptr, 1});
}
