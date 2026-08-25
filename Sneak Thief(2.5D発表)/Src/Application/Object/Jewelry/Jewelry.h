#pragma once

class Player;

class Jewelry :public KdGameObject
{
public:
	Jewelry() { Init(); }
	~Jewelry() {}

	void Init() override;
	void Update() override;
	void PostUpdate() override;
	void DrawLit() override;
	void GenerateDepthMapFromLight() override;
	void DrawBright() override;

	void OnHit() override;

	void SetTargetPlayer(const std::shared_ptr<Player>& _pPlayer)
	{
		m_wpplayer = _pPlayer; // shared_ptr を weak_ptr に代入（カウントは増えない）
	}

private:
	std::shared_ptr<KdSquarePolygon> m_polygon;

	std::weak_ptr<Player>m_wpplayer;

	Math::Vector3 m_pos;

	// フェードアウト用
	bool          m_isFadeOut = false;
	float m_alpha;
};