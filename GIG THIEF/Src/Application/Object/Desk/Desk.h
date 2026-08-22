#pragma once

class Player;

class Desk : public KdGameObject
{
public:
	Desk() {}
	~Desk() override {}

	void Init() override;
	void Update() override;
	void DrawLit() override;
	void DrawBright() override;

	void SetPlayer(const std::shared_ptr<Player>& player) { m_wpPlayer = player; }

	void SetPos(const Math::Vector3& pos) override { m_pos = pos; }
	void SetRot(const float rot) { m_rot = rot; }

	Math::Vector3 GetPos() const override { return m_pos; }

private:
	void UpdateAnimation();

	std::shared_ptr<KdModelData> m_model = nullptr;
	KdModelWork m_modelWork; // ★ 追加：アニメーション計算用の作業用モデルデータ
	std::shared_ptr<KdAnimator> m_spAnimator = nullptr;
	std::weak_ptr<Player> m_wpPlayer;

	Math::Vector3 m_pos = { 0.0f, 0.0f, 0.0f }; // ★ 追加：位置情報
	float m_rot = 0.0f; // ★ 追加：回転情報

	bool m_hasItem = true;
	bool m_isOpen = false;
	float m_interactRange = 0.3f;
};