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

private:

	std::shared_ptr<KdSquarePolygon> m_polygon;

	Math::Vector3 m_pos;
	Math::Vector3 m_dir;
	int m_dirID = 0; // 0:下, 1:左, 2:右, 3:上

	float m_anime = 0;
	float m_speed = 0.01f;
	float m_gravity = 0;

};