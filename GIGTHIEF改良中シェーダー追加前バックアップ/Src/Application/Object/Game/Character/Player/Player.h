#pragma once

class Item;

class Player : public KdGameObject
{
public:
	Player() { Init(); }
	~Player() {}

	void Init()                       override;
	void Update()                     override;
	void PostUpdate()                 override;
	void GenerateDepthMapFromLight() override;
	void DrawLit()                    override;
	void DrawUnLit()                  override;

	void PickUpItem(); // 拾う処理
	void ThrowItem();  // 投げる処理

	// ★ 投擲アイテムの追加・所持数取得関数
	void AddThrowableItem(int count = 1)
	{
		m_throwableCount += count;

		// 獲得SEを鳴らす（必要に応じてパスを調整してください）
		auto se = KdAudioManager::Instance().Play("Asset/Sounds/Get.wav", false);
		if (se)
		{
			se->SetVolume(0.2f);
		}
	}

	int GetThrowableCount() const { return m_throwableCount; }

	void SetHasJewelry(bool _flag)
	{
		if (m_hasJewelry == _flag)
		{
			return;
		}

		m_hasJewelry = _flag;

		// false から true に変わった「獲得した瞬間」だけ音を鳴らす
		if (m_hasJewelry == true)
		{
			// 【サウンド再生】音量0.2fで再生
			auto se = KdAudioManager::Instance().Play("Asset/Sounds/Get.wav", false);
			if (se)
			{
				se->SetVolume(0.2f);
			}
		}
	}

	// 外部（Deskなど）から持たせるアイテムを直接セットする
	void SetHeldItem(const std::shared_ptr<Item>& item)
	{
		m_pHeldItem = item;
	}

	// 現在持っているかを確認する関数
	bool HasJewelry() const { return m_hasJewelry; }

	// 手持ちアイテムを持っているか取得
	std::shared_ptr<Item> GetHeldItem() const { return m_pHeldItem; }

private:
	// =============================================================
	// ★ 可読性向上のための分割関数
	// =============================================================
	void UpdateInput(bool& outIsMoving); // キー入力と移動ベクトルの設定
	void UpdateAnimation(bool isMoving);  // 移動・待機のUVアニメーション更新
	void UpdateItemActions();            // アイテムの自動拾い・投げ入力の監視

	void CheckRayCollision();            // 地面とのレイ判定（高さ補正・着地）
	void CheckSphereCollision();         // 壁とのスフィア判定（押し戻し）

	std::shared_ptr<KdSquarePolygon> m_polygon;
	std::shared_ptr<KdSoundInstance> m_walkSound;
	std::shared_ptr<Item> m_pHeldItem = nullptr;

	Math::Vector3 m_pos;
	Math::Vector3 m_dir;
	int m_dirID = 2; // 0:下, 1:左, 2:右, 3:上

	float m_anime = 0;
	float m_speed = 0.01f;
	float m_gravity = 0;

	bool m_hasJewelry = false; // 宝石を獲得したか？
	bool m_showDebugWire = false;

	// ★ 投擲アイテムの所持数
	int m_throwableCount = 0;
};