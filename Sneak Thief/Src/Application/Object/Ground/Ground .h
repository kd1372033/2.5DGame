#pragma once

class Ground : public KdGameObject
{
public:
	Ground() { Init(); }
	virtual ~Ground() override = default;

	void Init() override;
	void Update() override;
	void DrawLit() override;
	void DrawUnLit() override;

	void SetPlayer(const std::shared_ptr<KdGameObject>& spPlayer) { m_wpPlayer = spPlayer; }

private:
	std::shared_ptr<KdModelData> m_front;
	std::shared_ptr<KdModelData> m_stage1;
	std::shared_ptr<KdModelData> m_stage2;
	std::shared_ptr<KdModelData> m_stage3;

	// 各モデル用のカラー（RGBA）。初期値はすべて白色・不透明(1, 1, 1, 1)
	Math::Color m_frontColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	Math::Color m_stage1Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	Math::Color m_stage2Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	Math::Color m_stage3Color = { 1.0f, 1.0f, 1.0f, 1.0f };

	std::weak_ptr<KdGameObject> m_wpPlayer;
};