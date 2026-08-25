#pragma once
#include "../BaseScene/BaseScene.h"

class Player;
class GameUI;

class GameScene : public BaseScene
{
public:
	GameScene() { Init(); }
	~GameScene() { /*KdAudioManager::Instance().SoundReset();*/ }

private:
	void Event() override;
	void Init()  override;

	// =============================================================
	// ★ 出現管理用の構造体と関数を追加
	// =============================================================
	struct RoomSpawnData {
		std::vector<Math::Vector3> spawnPositions; // 各部屋の敵の座標リスト
	};

	std::vector<RoomSpawnData> m_rooms; // 全部屋のデータを格納する配列

	// 指定した部屋番号の敵を「一括出現」させる共通関数
	void SpawnEnemiesInRoom(int roomIndex);
	// =============================================================

	std::shared_ptr<Player> m_player;
	std::shared_ptr<GameUI> m_UI;

	bool m_hasSpawnedEnemies = false;

	bool m_hasHiddenKeyGuide = false;
};