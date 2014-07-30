#ifndef YUV2RGB_MATERIAL_H
#define YUV2RGB_MATERIAL_H

#include <QSGMaterial>
#include <QSGTexture>

class YUV2RGBMaterial : public QSGMaterial
{
public:
	YUV2RGBMaterial();

	virtual QSGMaterialType *type() const;
	virtual QSGMaterialShader *createShader() const;
	virtual int compare(const QSGMaterial *other) const;

	void setTexture(QSGTexture *texture);
	QSGTexture *texture() const { return m_texture; }

private:
	QSGTexture *m_texture;
};

#endif // YUV2RGB_MATERIAL_H
