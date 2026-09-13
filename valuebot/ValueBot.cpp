#include "ValueBot.h"
#include <fstream>
#include <map>

#ifdef _WIN32
#include <Windows.h>
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

using namespace BWAPI;

void ValueBot::onStart()
{
  Broodwar->setCommandOptimizationLevel(2);
  Broodwar->sendText("ValueBot online. gl hf!");
  std::ofstream log(logPath, std::ios::trunc);
  log << "frame,myMin,myGas,enemyVisibleCount,enemyMin,enemyGas\n";
}

void ValueBot::mineWithIdleWorkers()
{
  for (auto& u : Broodwar->self()->getUnits())
  {
    if (!u->exists()) continue;
    if (u->getType() != UnitTypes::Zerg_Drone) continue;
    if (!u->isIdle()) continue;
    if (u->isCarryingMinerals() || u->isCarryingGas()) continue;
    Unit closest = nullptr;
    int best = 1 << 30;
    for (auto& m : Broodwar->getMinerals())
    {
      if (!m->exists()) continue;
      int d = u->getDistance(m);
      if (d < best) { best = d; closest = m; }
    }
    if (closest) u->gather(closest);
  }
}

void ValueBot::doBuildOrder()
{
  Player self = Broodwar->self();
  int minerals = self->minerals();
  int supplyUsed = self->supplyUsed() / 2;
  int supplyTotal = self->supplyTotal() / 2;

  bool hasPool = false, poolMorphing = false;
  for (auto& u : self->getUnits())
  {
    if (!u->exists()) continue;
    if (u->getType() == UnitTypes::Zerg_Spawning_Pool) { hasPool = true; break; }
    if (!u->isCompleted() && u->getBuildType() == UnitTypes::Zerg_Spawning_Pool) { poolMorphing = true; break; }
  }

  // 서플라이 막히기 전 오버로드 추가 (저글링 러시 유지용)
  if (supplyTotal < 200 && supplyUsed + 2 >= supplyTotal && minerals >= 100)
  {
    for (auto& u : self->getUnits())
    {
      if (u->exists() && u->getType() == UnitTypes::Zerg_Larva)
      {
        if (u->morph(UnitTypes::Zerg_Overlord)) return;
      }
    }
  }

  // 9서플 오버로드
  if (supplyUsed >= 9 && supplyTotal == 9 && minerals >= 100)
  {
    for (auto& u : self->getUnits())
    {
      if (u->exists() && u->getType() == UnitTypes::Zerg_Larva)
      {
        if (u->morph(UnitTypes::Zerg_Overlord)) return;
      }
    }
  }

  // 스포닝 풀 (드론 1기로 건설)
  if (!hasPool && !poolMorphing && minerals >= 200 && poolStartedFrame < 0)
  {
    for (auto& u : self->getUnits())
    {
      if (u->exists() && u->getType() == UnitTypes::Zerg_Drone && !u->isConstructing())
      {
        TilePosition around = Broodwar->self()->getStartLocation();
        if (u->build(UnitTypes::Zerg_Spawning_Pool, Broodwar->getBuildLocation(UnitTypes::Zerg_Spawning_Pool, around)))
        {
          poolStartedFrame = Broodwar->getFrameCount();
          return;
        }
      }
    }
  }

  // 드론 추가 (최대 12기까지)
  int drones = 0;
  for (auto& u : self->getUnits())
    if (u->exists() && u->getType() == UnitTypes::Zerg_Drone) drones++;
  if (drones < 12 && minerals >= 50 && supplyUsed < supplyTotal)
  {
    for (auto& u : self->getUnits())
    {
      if (u->exists() && u->getType() == UnitTypes::Zerg_Larva)
      {
        if (u->morph(UnitTypes::Zerg_Drone)) return;
      }
    }
  }

  // 풀 완성 후 저글링 양산
  if (hasPool && minerals >= 50 && supplyUsed < supplyTotal)
  {
    for (auto& u : self->getUnits())
    {
      if (u->exists() && u->getType() == UnitTypes::Zerg_Larva)
      {
        if (u->morph(UnitTypes::Zerg_Zergling)) return;
      }
    }
  }
}

void ValueBot::attackWithArmy()
{
  int lings = 0;
  for (auto& u : Broodwar->self()->getUnits())
    if (u->exists() && u->getType() == UnitTypes::Zerg_Zergling && u->isCompleted()) lings++;

  if (lings < 6) return; // 6기 모이면 출발

  // 모든 시작 위치를 순회하며 수색+공격 (적 시작 위치를 모르기 때문)
  // 500프레임마다 다음 후보지로 갱신, 보이는 적이 있으면 그쪽 우선
  Position target = Positions::None;
  for (auto& u : Broodwar->enemy()->getUnits())
  {
    if (u->exists() && u->isVisible()) { target = u->getPosition(); break; }
  }
  if (!target.isValid())
  {
    auto starts = Broodwar->getStartLocations();
    if (!starts.empty())
    {
      size_t idx = (Broodwar->getFrameCount() / 500) % starts.size();
      target = Position(starts[idx]);
    }
  }
  if (!target.isValid()) target = Position(Broodwar->mapWidth() * 16, Broodwar->mapHeight() * 16);

  for (auto& u : Broodwar->self()->getUnits())
  {
    if (!u->exists() || !u->isCompleted()) continue;
    if (u->getType() != UnitTypes::Zerg_Zergling) continue;
    if (u->isIdle() || u->isGatheringMinerals()) u->attack(target);
  }
}

void ValueBot::logEnemyValue()
{
  if (Broodwar->getFrameCount() % 100 != 0) return;

  int enemyMin = 0, enemyGas = 0, enemyCount = 0;
  std::map<std::string, int> countByType;

  auto addUnit = [&](Unit u) {
    if (!u || !u->exists() || !u->isVisible()) return;
    if (!u->isDetected()) return;
    if (u->isHallucination()) return;
    UnitType t = u->getType();
    if (t == UnitTypes::Unknown) return;
    if (t.isNeutral() || t.isCritter() || t.isMineralField()
        || t.isSpell() || t.isBeacon() || t.isPowerup()) return;
    if (!u->isCompleted())
    {
      t = u->getBuildType();
      if (t == UnitTypes::Unknown || t == UnitTypes::None) return;
    }
    enemyMin += t.mineralPrice();
    enemyGas += t.gasPrice();
    enemyCount++;
    countByType[t.getName()]++;
  };

  if (Broodwar->enemy())
    for (auto u : Broodwar->enemy()->getUnits()) addUnit(u);
  else
    for (auto p : Broodwar->enemies())
      for (auto u : p->getUnits()) addUnit(u);

  std::ofstream log(logPath, std::ios::app);
  log << Broodwar->getFrameCount() << ","
      << Broodwar->self()->minerals() << ","
      << Broodwar->self()->gas() << ","
      << enemyCount << "," << enemyMin << "," << enemyGas << "\n";

  Broodwar->drawTextScreen(10, 10, "Enemy visible value: %d min + %d gas (%d units)",
                           enemyMin, enemyGas, enemyCount);
}

void ValueBot::onFrame()
{
  if (Broodwar->isReplay()) return;
  if (Broodwar->getFrameCount() >= 20000)
  {
    Broodwar->sendText("gg, frame limit");
    Broodwar->leaveGame();
    return;
  }
  mineWithIdleWorkers();
  doBuildOrder();
  attackWithArmy();
  logEnemyValue();
}

void ValueBot::onUnitComplete(Unit unit)
{
  if (!unit || !unit->exists()) return;
  // 완성된 드론은 바로 미네랄로
  if (unit->getType() == UnitTypes::Zerg_Drone && unit->getPlayer() == Broodwar->self())
  {
    Unit closest = nullptr;
    int best = 1 << 30;
    for (auto& m : Broodwar->getMinerals())
    {
      if (!m->exists()) continue;
      int d = unit->getDistance(m);
      if (d < best) { best = d; closest = m; }
    }
    if (closest) unit->gather(closest);
  }
}

void ValueBot::onEnd(bool isWinner)
{
  std::ofstream log(logPath, std::ios::app);
  log << "# GAME END winner=" << (isWinner ? 1 : 0)
      << " frames=" << Broodwar->getFrameCount() << "\n";
}

extern "C" DLLEXPORT void gameInit(BWAPI::Game* game) { BWAPI::BroodwarPtr = game; }
extern "C" DLLEXPORT BWAPI::AIModule* newAIModule() { return new ValueBot(); }
