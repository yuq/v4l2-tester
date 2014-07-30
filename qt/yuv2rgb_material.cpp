#include "yuv2rgb_material.h"

class YUV2RGBMaterialShader : public QSGMaterialShader
{
public:
	YUV2RGBMaterialShader();

	virtual void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect);
	virtual char const *const *attributeNames() const;
	virtual void initialize();
	static QSGMaterialType type;

private:
	int m_matrix_id;
	int m_opacity_id;
};

QSGMaterialType YUV2RGBMaterialShader::type;

YUV2RGBMaterialShader::YUV2RGBMaterialShader()
	: QSGMaterialShader()
{
	setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/yuv2rgb.vert"));
	setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/yuv2rgb.frag"));
}

char const *const *YUV2RGBMaterialShader::attributeNames() const
{
	static char const *const attr[] = { "qt_VertexPosition", "qt_VertexTexCoord", 0 };
	return attr;
}

void YUV2RGBMaterialShader::initialize()
{
	m_matrix_id = program()->uniformLocation("qt_Matrix");
	m_opacity_id = program()->uniformLocation("opacity");
}

void YUV2RGBMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
	Q_ASSERT(oldEffect == 0 || newEffect->type() == oldEffect->type());

	YUV2RGBMaterial *tx = static_cast<YUV2RGBMaterial *>(newEffect);
	YUV2RGBMaterial *oldTx = static_cast<YUV2RGBMaterial *>(oldEffect);

	QSGTexture *t = tx->texture();

	if (oldTx == 0 || oldTx->texture()->textureId() != t->textureId())
		t->bind();
	else
		t->updateBindOptions();

	if (state.isMatrixDirty())
		program()->setUniformValue(m_matrix_id, state.combinedMatrix());

	if (state.isOpacityDirty())
		program()->setUniformValue(m_opacity_id, state.opacity());
}


YUV2RGBMaterial::YUV2RGBMaterial()
	: m_texture(0)
{

}

QSGMaterialType *YUV2RGBMaterial::type() const
{
	return &YUV2RGBMaterialShader::type;
}

QSGMaterialShader *YUV2RGBMaterial::createShader() const
{
	return new YUV2RGBMaterialShader;
}

void YUV2RGBMaterial::setTexture(QSGTexture *texture)
{
	m_texture = texture;
	setFlag(Blending, m_texture ? m_texture->hasAlphaChannel() : false);
}

int YUV2RGBMaterial::compare(const QSGMaterial *o) const
{
	Q_ASSERT(o && type() == o->type());
	const YUV2RGBMaterial *other = static_cast<const YUV2RGBMaterial *>(o);
	return m_texture->textureId() - other->texture()->textureId();
}
