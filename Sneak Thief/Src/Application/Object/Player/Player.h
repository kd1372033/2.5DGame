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

	void SetHasJewelry(bool _flag) { m_hasJewelry = _flag; }
	// 必要に応じて：現在持っているかを確認する関数
	bool HasJewelry() const { return m_hasJewelry; }

private:

	std::shared_ptr<KdSquarePolygon> m_polygon;

	std::shared_ptr<KdSoundInstance> m_walkSound;

	Math::Vector3 m_pos;
	Math::Vector3 m_dir;
	int m_dirID = 0; // 0:下, 1:左, 2:右, 3:上

	float m_anime = 0;
	float m_speed = 0.01f;
	float m_gravity = 0;

	bool m_hasJewelry = false; // 宝石を獲得したか？

};