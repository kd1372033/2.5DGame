#pragma once

class Ground : public KdGameObject
{
public:
	Ground() { Init(); }
	~Ground() {}

	void Init() override;
	void DrawLit() override;

private:

	std::shared_ptr<KdModelData> m_model;
};