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
};
