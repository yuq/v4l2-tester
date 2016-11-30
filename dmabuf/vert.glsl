attribute vec3 positionIn;
attribute vec2 texcoordIn;

varying vec2 texcoord;

void main()
{
    gl_Position = vec4(positionIn, 1);
	texcoord = texcoordIn;
}
