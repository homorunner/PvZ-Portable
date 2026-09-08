#include "UnitTestRunner.h"
#include "LawnApp.h"
#include "Lawn/Board.h"
#include "Lawn/System/PlayerInfo.h"
#include "Lawn/System/ProfileMgr.h"
#include "PvzpLib/EffectSystem.h"
#include "Resources.h"
#include "graphics/Graphics.h"
#include <SDL.h>
#include <format>
#include <stdexcept>

UnitTestRunner* UnitTestRunner::active = nullptr;

UnitTestRunner::UnitTestRunner()
{
	char* base = SDL_GetBasePath();
	if (!base) throw std::runtime_error("Cannot locate executable for unittest.log");
	auto logPath = std::filesystem::u8path(base) / "unittest.log";
	SDL_free(base);
	mLog.open(logPath, std::ios::trunc);
	if (!mLog) throw std::runtime_error("Cannot open unittest.log beside executable");
	const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
	for (int i = 0; ; ++i)
	{
		mSandbox = std::filesystem::temp_directory_path() / std::format("pvz-unittest-{}-{}", nonce, i);
		if (std::filesystem::create_directory(mSandbox)) break;
	}
	RegisterLawnTests(*this);
	active = this;
	Log(std::format("UNITTEST: {} cases; 10 ms simulation ticks", mCases.size()));
}

UnitTestRunner::~UnitTestRunner()
{
	active = nullptr;
	std::error_code error;
	std::filesystem::remove_all(mSandbox, error);
}

void UnitTestRunner::Configure(LawnApp& app)
{
	// Called after CLI parsing and before registry/profile initialization.
	auto path = mSandbox.generic_u8string();
	app.mCustomSaveDir.assign(reinterpret_cast<const char*>(path.data()), path.size());
}

void UnitTestRunner::Start(LawnApp& app)
{
	mStarted = true;
	if (!app.mPlayerInfo) app.mPlayerInfo = app.mProfileMgr->AddProfile("UnitTest");
	mTick = 0;
	mCaseFailed = false;
	mFinished = false;
	Log(std::format("START {}", mCases[mIndex].name));
	mCases[mIndex].setup(*this, app);
}

void UnitTestRunner::Update(Board& board)
{
	if (mFinished) return;
	++mTick;
	board.mApp->mEffectSystem->Update();
	board.UpdateGameObjects();
	mCases[mIndex].update(*this, board);
	if (!mFinished && mTick >= mCases[mIndex].timeoutTicks)
	{
		Check(false, "Case timed out");
		Finish();
	}
}

void UnitTestRunner::AfterFrame(LawnApp& app)
{
	if (!mFinished || mCompleted || app.mShutdown) return;
	// Board replacement and shutdown must wait until widget updates have unwound.
	if (mIndex + 1 < mCases.size())
	{
		++mIndex;
		Start(app);
	}
	else if (std::chrono::steady_clock::now() - mHoldStart >= std::chrono::seconds(5))
	{
		mCompleted = true;
		Log("Final scene hold complete; shutting down");
		app.Shutdown();
	}
}

void UnitTestRunner::ProjectileCreated(const Projectile& projectile)
{
	if (mStarted && !mFinished && mCases[mIndex].projectile)
		mCases[mIndex].projectile(*this, projectile);
}

void UnitTestRunner::Log(const std::string& message)
{
	mLog << message << std::endl;
}

void UnitTestRunner::Check(bool condition, const std::string& message)
{
	++mChecks;
	mCaseFailed |= !condition;
	Log(std::format("{} tick={}: {}", condition ? "OK" : "FAIL", mTick, message));
}

void UnitTestRunner::Finish()
{
	if (mFinished) return;
	mFinished = true;
	(mCaseFailed ? mFailed : mPassed)++;
	Log(std::format("{} {} ({} ticks)", mCaseFailed ? "FAIL" : "PASS", mCases[mIndex].name, mTick));
	mHoldStart = std::chrono::steady_clock::now();
	if (mIndex + 1 == mCases.size())
		Log(std::format("TOTAL {} passed, {} failed, {} checks; holding final scene 5 seconds", mPassed, mFailed, mChecks));
}

int UnitTestRunner::ExitStatus()
{
	bool complete = mCompleted;
	if (!complete) Log("FAIL: runner interrupted before suite completion");
	return complete && mFailed == 0 && mLog.good() ? 0 : 1;
}

void UnitTestRunner::Draw(Sexy::Graphics* g)
{
	Sexy::Graphics overlay(*g);
	overlay.SetColor(Sexy::Color(0, 0, 0, 220));
	overlay.FillRect(4, 4, 620, 64);
	overlay.SetFont(Sexy::FONT_PICO129);
	overlay.SetColor(Sexy::Color(255, 255, 255));
	overlay.DrawString(std::format("UNITTEST {}/{}  PASS {}  FAIL {}  Checks {}", mIndex + 1, mCases.size(), mPassed, mFailed, mChecks), 12, 27);
	overlay.DrawString(std::format("{}  tick {}{}", mCases[mIndex].name, mTick, mFinished ? "  [finished]" : ""), 12, 53);
}
