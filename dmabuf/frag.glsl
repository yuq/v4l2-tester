
precision mediump float;

uniform sampler2D texMap;

varying vec2 texcoord;

void main() {
	vec4 color = texture2D(texMap, texcoord).argb;
	float y = (color.a + color.g) * 0.5;
	float u = color.b - 0.5;
	float v = color.r - 0.5;
	float r = clamp(y + 1.402 * v, 0.0, 1.0);
	float g = clamp(y - 0.344 * u - 0.714 * v, 0.0, 1.0);
	float b = clamp(y + 1.772 * u, 0.0, 1.0);
	gl_FragColor = vec4(r, g, b, 1);
}
