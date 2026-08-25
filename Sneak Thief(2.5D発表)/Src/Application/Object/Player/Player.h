#pragma once

class Player : public KdGameObject
{
public:
	Player() { Init(); }
	~Player() {}

	void Init()			override;
	void Update()		override;
	void PostUpdate()	override;
	void GenerateDepthMapFromLight()override;
	void DrawLit()		override;

	void ThrowItem();

	void SetHasJewelry(bool _flag)
	{
		if (m_hasJewelry == _flag) return;

		m_hasJewelry = _flag;

		// false から true に変わった「獲得した瞬間」だけ音を鳴らす
		if (m_hasJewelry == true)
		{
			// 【サウンド再生】音量0.2fで再生（音量はお好みで調整してください）
			auto se = KdAudioManager::Instance().Play("Asset/Sounds/Get.wav", false);
			if (se)
			{
				se->SetVolume(0.2f);
			}
		}
	}
	// 必要に応じて：現在持っているかを確認する関数
	bool HasJewelry() const { return m_hasJewelry; }

private:

	std::shared_ptr<KdSquarePolygon> m_polygon;

	std::shared_ptr<KdSoundInstance> m_walkSound;

	Math::Vector3 m_pos;
	Math::Vector3 m_dir;
	int m_dirID = 2; // 0:下, 1:左, 2:右, 3:上

	float m_anime = 0;
	float m_speed = 0.01f;
	float m_gravity = 0;

	bool m_hasJewelry = false; // 宝石を獲得したか？

	bool m_showDebugWire = false;

};