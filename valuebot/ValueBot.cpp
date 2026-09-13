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
  log << "frame,myMin,myGas,myLings,myDrones,enemyVisibleCount,enemyMin,enemyGas,enemyKills,myLosses\n";
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

  // 가스통 완성됐으면 드론 3기 배치
  for (auto& g : Broodwar->self()->getUnits())
  {
    if (!g->exists() || g->getType() != UnitTypes::Zerg_Extractor || !g->isCompleted()) continue;
    int gasWorkers = 0;
    for (auto& u : Broodwar->self()->getUnits())
    {
      if (!u->exists() || u->getType() != UnitTypes::Zerg_Drone) continue;
      if (u->isGatheringGas() || (u->isCarryingGas())) gasWorkers++;
    }
    for (auto& u : Broodwar->self()->getUnits())
    {
      if (gasWorkers >= 3) break;
      if (!u->exists() || u->getType() != UnitTypes::Zerg_Drone) continue;
      if (u->isGatheringGas() || u->isCarryingGas() || u->isConstructing()) continue;
      if (u->gather(g)) gasWorkers++;
    }
    break; // 첫 번째 가스통만 관리
  }
}

void ValueBot::doBuildOrder()
{
  Player self = Broodwar->self();
  int minerals = self->minerals();
  int gas = self->gas();
  int supplyUsed = self->supplyUsed() / 2;
  int supplyTotal = self->supplyTotal() / 2;
  const bool ALL_IN = ValueBot::ALL_IN; // 5풀 저글링 올인: 드론 6기, 가스/확장 없음

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

  // 스포닝 풀 (올인: 미네랄 200 모이는 즉시 건설)
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

  // 드론 추가 (올인 시 5기에서 중단)
  int drones = 0;
  for (auto& u : self->getUnits())
    if (u->exists() && u->getType() == UnitTypes::Zerg_Drone) drones++;
  if (drones < (ALL_IN ? 5 : 16) && minerals >= 50 && supplyUsed < supplyTotal)
  {
    for (auto& u : self->getUnits())
    {
      if (u->exists() && u->getType() == UnitTypes::Zerg_Larva)
      {
        if (u->morph(UnitTypes::Zerg_Drone)) return;
      }
    }
  }

  // 두 번째 해처리 (매크로용, 본진에 추가)
  int hatchCount = 0;
  bool hatchMorphing = false;
  for (auto& u : self->getUnits())
  {
    if (!u->exists()) continue;
    if (u->getType() == UnitTypes::Zerg_Hatchery ||
        u->getType() == UnitTypes::Zerg_Lair ||
        u->getType() == UnitTypes::Zerg_Hive) hatchCount++;
    if (!u->isCompleted() && u->getBuildType() == UnitTypes::Zerg_Hatchery) hatchMorphing = true;
  }
  if (!ALL_IN && hasPool && hatchCount + (hatchMorphing ? 1 : 0) < 2 && minerals >= 300)
  {
    for (auto& u : self->getUnits())
    {
      if (u->exists() && u->getType() == UnitTypes::Zerg_Drone && !u->isConstructing())
      {
        TilePosition around = Broodwar->self()->getStartLocation();
        if (u->build(UnitTypes::Zerg_Hatchery, Broodwar->getBuildLocation(UnitTypes::Zerg_Hatchery, around)))
          return;
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

  // 가스: 엑스트랙터 (드론 12기 이상, 미네랄 50 이상)
  bool hasExtractor = false;
  for (auto& u : self->getUnits())
  {
    if (!u->exists()) continue;
    if (u->getType() == UnitTypes::Zerg_Extractor) { hasExtractor = true; break; }
    if (!u->isCompleted() && u->getBuildType() == UnitTypes::Zerg_Extractor) { hasExtractor = true; break; }
  }
  if (!ALL_IN && hasPool && !hasExtractor && drones >= 12 && minerals >= 50)
  {
    for (auto& u : self->getUnits())
    {
      if (u->exists() && u->getType() == UnitTypes::Zerg_Drone && !u->isConstructing())
      {
        // 가장 가까운 가스통 찾기
        Unit geyser = nullptr;
        int best = 1 << 30;
        for (auto& g : Broodwar->getGeysers())
        {
          if (!g->exists()) continue;
          int d = u->getDistance(g);
          if (d < best) { best = d; geyser = g; }
        }
        if (geyser && u->build(UnitTypes::Zerg_Extractor, TilePosition(geyser->getPosition()))) return;
      }
    }
  }

  // 히드라 덴 (가스 100 이상)
  bool hasDen = false;
  for (auto& u : self->getUnits())
  {
    if (!u->exists()) continue;
    if (u->getType() == UnitTypes::Zerg_Hydralisk_Den) { hasDen = true; break; }
    if (!u->isCompleted() && u->getBuildType() == UnitTypes::Zerg_Hydralisk_Den) { hasDen = true; break; }
  }
  if (!ALL_IN && hasPool && hasExtractor && !hasDen && minerals >= 100 && gas >= 100)
  {
    for (auto& u : self->getUnits())
    {
      if (u->exists() && u->getType() == UnitTypes::Zerg_Drone && !u->isConstructing())
      {
        TilePosition around = Broodwar->self()->getStartLocation();
        if (u->build(UnitTypes::Zerg_Hydralisk_Den, Broodwar->getBuildLocation(UnitTypes::Zerg_Hydralisk_Den, around)))
          return;
      }
    }
  }

  // 덴 완성 후 히드라 양산 (가스 있을 때 우선, 올인 시 스킵)
  if (!ALL_IN && hasDen && minerals >= 75 && gas >= 25 && supplyUsed + 2 <= supplyTotal)
  {
    for (auto& u : self->getUnits())
    {
      if (u->exists() && u->getType() == UnitTypes::Zerg_Larva)
      {
        if (u->morph(UnitTypes::Zerg_Hydralisk)) return;
      }
    }
  }
}

void ValueBot::attackWithArmy()
{
  const bool ALL_IN = ValueBot::ALL_IN;
  int lings = 0, hydras = 0;
  for (auto& u : Broodwar->self()->getUnits())
  {
    if (!u->exists() || !u->isCompleted()) continue;
    if (u->getType() == UnitTypes::Zerg_Zergling) lings++;
    if (u->getType() == UnitTypes::Zerg_Hydralisk) hydras++;
  }
  int armyPoints = lings + hydras * 3;

  Position home = Position(Broodwar->self()->getStartLocation());

  // 병력 부족하면 본진에 집결 (올인 시에는 6기부터 전진)
  if (armyPoints < (ALL_IN ? 6 : 18))
  {
    for (auto& u : Broodwar->self()->getUnits())
    {
      if (!u->exists() || !u->isCompleted()) continue;
      if (u->getType() != UnitTypes::Zerg_Zergling && u->getType() != UnitTypes::Zerg_Hydralisk) continue;
      if (u->isIdle() && u->getDistance(home) > 200) u->move(home);
    }
    return;
  }

  // 모든 시작 위치를 순회하며 수색+공격 (적 시작 위치를 모르기 때문)
  // 순서: 보이는 적 > 정찰된 적 본진 > 500프레임마다 다음 후보지
  Position target = Positions::None;
  for (auto& u : Broodwar->enemy()->getUnits())
  {
    if (u->exists() && u->isVisible()) { target = u->getPosition(); break; }
  }
  if (!target.isValid() && enemyBase.isValid()) target = enemyBase;
  if (!target.isValid())
  {
    // 고착식 수색: 목표 근처에 아군이 도착했거나 3000프레임 지났을 때만 변경
    bool needNew = !huntTarget.isValid();
    if (!needNew)
    {
      bool arrived = false;
      for (auto& u : Broodwar->self()->getUnits())
      {
        if (!u->exists() || !u->isCompleted()) continue;
        if (u->getType() != UnitTypes::Zerg_Zergling && u->getType() != UnitTypes::Zerg_Hydralisk) continue;
        if (u->getDistance(huntTarget) < 400) { arrived = true; break; }
      }
      if (arrived || Broodwar->getFrameCount() - huntTargetFrame > 3000) needNew = true;
    }
    if (needNew)
    {
      auto starts = Broodwar->getStartLocations();
      // 자기 본진 제외하고 아직 안 가본 곳 우선
      for (size_t tries = 0; tries < starts.size(); tries++)
      {
        TilePosition t = starts[scoutIdx % starts.size()];
        scoutIdx++;
        if (t == Broodwar->self()->getStartLocation()) continue;
        if (Position(t) == huntTarget) continue;
        huntTarget = Position(t);
        break;
      }
      huntTargetFrame = Broodwar->getFrameCount();
    }
    target = huntTarget;
  }
  if (!target.isValid()) target = Position(Broodwar->mapWidth() * 16, Broodwar->mapHeight() * 16);

  for (auto& u : Broodwar->self()->getUnits())
  {
    if (!u->exists() || !u->isCompleted()) continue;
    if (u->getType() != UnitTypes::Zerg_Zergling && u->getType() != UnitTypes::Zerg_Hydralisk) continue;
    if (u->isIdle() || u->isGatheringMinerals()) u->attack(target);
  }
  // 캐논 있으면 일꾼부터 점사 (방어 무시하고 경제 마비)
  {
    bool staticD = false;
    for (auto& e : Broodwar->enemy()->getUnits())
    {
      if (!e->exists() || !e->isVisible()) continue;
      auto t = e->getType();
      if (t == UnitTypes::Protoss_Photon_Cannon || t == UnitTypes::Zerg_Sunken_Colony ||
          t == UnitTypes::Zerg_Spore_Colony || t == UnitTypes::Terran_Bunker ||
          t == UnitTypes::Terran_Missile_Turret) { staticD = true; break; }
    }
    if (staticD)
    {
      for (auto& u : Broodwar->self()->getUnits())
      {
        if (!u->exists() || !u->isCompleted()) continue;
        if (u->getType() != UnitTypes::Zerg_Zergling && u->getType() != UnitTypes::Zerg_Hydralisk) continue;
        BWAPI::Unit best = nullptr;
        int bestD = 1 << 30;
        for (auto& e : Broodwar->enemy()->getUnits())
        {
          if (!e->exists() || !e->isVisible()) continue;
          if (!e->getType().isWorker()) continue;
          int d = u->getDistance(e);
          if (d < bestD) { bestD = d; best = e; }
        }
        if (best) u->attack(best);
      }
    }
  }
  // 올인: 드론도 전부 공격 합류 (일꾼 러시 병행)
  if (ALL_IN)
  {
    for (auto& u : Broodwar->self()->getUnits())
    {
      if (!u->exists() || !u->isCompleted()) continue;
      if (u->getType() != UnitTypes::Zerg_Drone) continue;
      if (u->isConstructing()) continue;
      if (u == scoutDrone) continue;
      u->attack(target);
    }
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
  int myLings = 0, myDrones = 0;
  for (auto& u : Broodwar->self()->getUnits())
  {
    if (!u->exists() || !u->isCompleted()) continue;
    if (u->getType() == UnitTypes::Zerg_Zergling) myLings++;
    if (u->getType() == UnitTypes::Zerg_Drone) myDrones++;
  }
  log << Broodwar->getFrameCount() << ","
      << Broodwar->self()->minerals() << ","
      << Broodwar->self()->gas() << ","
      << myLings << "," << myDrones << ","
      << enemyCount << "," << enemyMin << "," << enemyGas << ","
      << enemyKills << "," << myLosses << "\n";

  Broodwar->drawTextScreen(10, 10, "Enemy visible value: %d min + %d gas (%d units)",
                           enemyMin, enemyGas, enemyCount);
}

void ValueBot::updateScout()
{
  // 적 건물 발견 시 본진 위치 기억
  if (!enemyBase.isValid() && Broodwar->enemy())
  {
    for (auto& u : Broodwar->enemy()->getUnits())
    {
      if (u->exists() && u->isVisible() && u->getType().isBuilding())
      {
        enemyBase = u->getPosition();
        break;
      }
    }
  }
  if (enemyBase.isValid()) return;

  // 300프레임부터 드론 1기 정찰
  if (Broodwar->getFrameCount() < 300) return;
  if (!scoutDrone || !scoutDrone->exists())
  {
    scoutDrone = nullptr;
    for (auto& u : Broodwar->self()->getUnits())
    {
      if (u->exists() && u->getType() == UnitTypes::Zerg_Drone && !u->isConstructing())
      {
        scoutDrone = u;
        break;
      }
    }
    if (!scoutDrone) return;
  }
  if (scoutDrone->isIdle())
  {
    auto starts = Broodwar->getStartLocations();
    if (starts.empty()) return;
    // 자기 시작 위치는 건너뛰기
    for (size_t tries = 0; tries < starts.size(); tries++)
    {
      TilePosition t = starts[scoutIdx % starts.size()];
      scoutIdx++;
      if (t == Broodwar->self()->getStartLocation()) continue;
      scoutDrone->move(Position(t));
      break;
    }
  }
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
  updateScout();
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

void ValueBot::onUnitDestroy(Unit unit)
{
  if (!unit) return;
  if (unit->getPlayer() == Broodwar->enemy())
  {
    enemyKills++;
    if (firstBloodFrame < 0)
    {
      firstBloodFrame = Broodwar->getFrameCount();
      Broodwar->sendText("First blood at %d!", firstBloodFrame);
    }
  }
  else if (unit->getPlayer() == Broodwar->self())
  {
    myLosses++;
  }
}

void ValueBot::onEnd(bool isWinner)
{
  std::ofstream log(logPath, std::ios::app);
  log << "# GAME END winner=" << (isWinner ? 1 : 0)
      << " frames=" << Broodwar->getFrameCount()
      << " enemyKills=" << enemyKills << " myLosses=" << myLosses
      << " firstBlood=" << firstBloodFrame << "\n";
}

extern "C" DLLEXPORT void gameInit(BWAPI::Game* game) { BWAPI::BroodwarPtr = game; }
extern "C" DLLEXPORT BWAPI::AIModule* newAIModule() { return new ValueBot(); }
