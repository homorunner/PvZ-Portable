#pragma once
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

class LawnApp;
class Board;
class Projectile;
namespace Sexy { class Graphics; }
class UnitTestRunner;
struct UnitTestCase
{
	const char* name;
	void (*setup)(UnitTestRunner&, LawnApp&);
	void (*update)(UnitTestRunner&, Board&);
	void (*projectile)(UnitTestRunner&, const Projectile&);
	int timeoutTicks;
};

class UnitTestRunner
{
public:
	static UnitTestRunner* active;
	UnitTestRunner();
	~UnitTestRunner();
	void Configure(LawnApp& app);
	void Start(LawnApp& app);
	void Update(Board& board);
	void AfterFrame(LawnApp& app);
	void Draw(Sexy::Graphics* g);
	void ProjectileCreated(const Projectile& projectile);
	void Check(bool condition, const std::string& message);
	void Log(const std::string& message);
	void Finish();
	int ExitStatus();
	int Tick() const { return mTick; }
	void Register(UnitTestCase test) { mCases.push_back(test); }
private:
	std::vector<UnitTestCase> mCases;
	std::ofstream mLog;
	std::filesystem::path mSandbox;
	size_t mIndex = 0;
	int mTick = 0, mPassed = 0, mFailed = 0, mChecks = 0;
	bool mCaseFailed = false, mFinished = false, mStarted = false;
	bool mCompleted = false;
	std::chrono::steady_clock::time_point mHoldStart;
};
void RegisterLawnTests(UnitTestRunner& runner);
