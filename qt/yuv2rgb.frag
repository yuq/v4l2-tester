varying highp vec2 qt_TexCoord;

uniform sampler2D qt_Texture;
uniform lowp float opacity;

void main()
{
	vec4 color = texture2D(qt_Texture, qt_TexCoord);
	float y = color.r;
	float u = color.g - 0.5;
	float v = color.b - 0.5;
	float r = clamp(y + 1.402 * v, 0.0, 1.0);
	float g = clamp(y - 0.344 * u - 0.714 * v, 0.0, 1.0);
	float b = clamp(y + 1.772 * u, 0.0, 1.0);
	gl_FragColor = vec4(r, g, b, color.a) * opacity;
}
