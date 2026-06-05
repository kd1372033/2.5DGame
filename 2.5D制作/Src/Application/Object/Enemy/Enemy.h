#pragma once

class Enemy : public KdGameObject
{
public:
	Enemy() { Init(); }
	~Enemy() {}

	void Init()			override;
	void Update()		override;
	void PostUpdate()	override;
	void GenerateDepthMapFromLight()override;
	void DrawLit()		override;

private:

	std::shared_ptr<KdSquarePolygon> m_polygon;

	int m_dirID = 0; // 0:下, 1:左, 2:右, 3:上
	Math::Vector3 m_pos;
	Math::Vector3 m_dir;

	float m_anime = 0;
	float m_speed = 0.01f;
	float m_gravity = 0;

	const int Run[4][8] = {
		{ 16,17,18,19,20,21,22,23 }, // 0:下
		{ 8,9,10,11,12,13,14,15 },   // 1:左
		{ 0,1,2,3,4,5,6,7 },         // 2:右
		{ 24,25,26,27,28,29,30,31 }  // 3:上
	};

	const int Wait[4][4] = {
		{ 32,33,34,35 }, // 0:下
		{ 40,41,42,43 }, // 1:左
		{ 36,37,38,39 }, // 2:右
		{ 44,45,46,47 }  // 3:上
	};
};