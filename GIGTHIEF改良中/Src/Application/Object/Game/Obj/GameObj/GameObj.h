#pragma once

class Furniture : public KdGameObject
{
public:
	Furniture() { Init(); }
	virtual ~Furniture() override = default;

	void Init() override;
	void Update() override;
	void DrawLit() override;

	void SetPlayer(const std::shared_ptr<KdGameObject>& spPlayer) { m_wpPlayer = spPlayer; }

private:
	std::shared_ptr<KdModelData> m_model1;
	std::shared_ptr<KdModelData> m_model2;
	std::shared_ptr<KdModelData> m_model3;

	std::weak_ptr<KdGameObject> m_wpPlayer;

	// 半透明フラグ
	bool m_isModel1ColorEnable = false;
	bool m_isModel2ColorEnable = false;
	bool m_isModel3ColorEnable = false;
};