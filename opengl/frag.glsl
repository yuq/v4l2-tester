
precision mediump float;

uniform sampler2D texMap;

varying vec2 texcoord;

void main() {
	vec3 color = texture2D(texMap, texcoord).rgb;
	float y = color.r;
	float u = color.g - 0.5;
	float v = color.b - 0.5;
	float r = clamp(y + 1.402 * v, 0.0, 1.0);
	float g = clamp(y - 0.344 * u - 0.714 * v, 0.0, 1.0);
	float b = clamp(y + 1.772 * u, 0.0, 1.0);
	gl_FragColor = vec4(r, g, b, 1);
	//gl_FragColor = vec4(0.5, 0.5, 0.0, 1);
}
