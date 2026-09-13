#pragma once
#include <BWAPI.h>

class ValueBot : public BWAPI::AIModule
{
public:
  virtual void onStart();
  virtual void onFrame();
  virtual void onEnd(bool isWinner);
  virtual void onUnitComplete(BWAPI::Unit unit);

private:
  void mineWithIdleWorkers();
  void doBuildOrder();
  void attackWithArmy();
  void logEnemyValue(); // 시야 내 적 유닛 자원가치 집계 + CSV 기록
  std::string logPath = "valuebot_log.csv";
  int poolStartedFrame = -1;
  static constexpr bool ALL_IN = true; // 5풀 저글링 올인
  int enemyKills = 0, myLosses = 0, firstBloodFrame = -1;
  virtual void onUnitDestroy(BWAPI::Unit unit);
  BWAPI::Position enemyBase = BWAPI::Positions::Invalid;
  BWAPI::Unit scoutDrone = nullptr;
  size_t scoutIdx = 0;
  void updateScout();
  // 고착식 수색 목표: 한 번 정하면 도착하거나 타임아웃까지 유지
  BWAPI::Position huntTarget = BWAPI::Positions::Invalid;
  int huntTargetFrame = 0;
};
