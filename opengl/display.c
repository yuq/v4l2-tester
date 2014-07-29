#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <sys/time.h>
#include <sys/stat.h>

static EGLDisplay display;
static EGLSurface surface;

static void render_target_init(EGLNativeWindowType nativeWindow)
{
	assert((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) != EGL_NO_DISPLAY);

    EGLint majorVersion;
    EGLint minorVersion;
    assert(eglInitialize(display, &majorVersion, &minorVersion) == EGL_TRUE);

    EGLConfig config;
    EGLint numConfigs;
    const EGLint configAttribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_NONE
    };
    assert(eglChooseConfig(display, configAttribs, &config, 1, &numConfigs) == EGL_TRUE);

    const EGLint attribList[] = {
		EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
		EGL_NONE
    };
    assert((surface = eglCreateWindowSurface(display, config, nativeWindow, attribList)) != EGL_NO_SURFACE);

    EGLContext context;
    const EGLint contextAttribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
    };
    assert((context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs)) != EGL_NO_CONTEXT);

    assert(eglMakeCurrent(display, surface, surface, context) == EGL_TRUE);
}

static GLuint LoadShader(const char *name, GLenum type)
{
    FILE *f;
    int size;
    char *buff;
    GLuint shader;
    GLint compiled;
    const char *source[1];

    assert((f = fopen(name, "r")) != NULL);

    // get file size
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    assert((buff = malloc(size)) != NULL);
    assert(fread(buff, 1, size, f) == size);
    source[0] = buff;
    fclose(f);
    shader = glCreateShader(type);
    glShaderSource(shader, 1, source, &size);
    glCompileShader(shader);
    free(buff);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
		GLint infoLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char *infoLog = malloc(infoLen);
			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			fprintf(stderr, "Error compiling shader %s:\n%s\n", name, infoLog);
			free(infoLog);
		}
		glDeleteShader(shader);
		return 0;
    }

    return shader;
}

GLuint program;

static void init_GLES(int width, int height)
{
    GLint linked;
    GLuint vertexShader;
    GLuint fragmentShader;
    assert((vertexShader = LoadShader("vert.glsl", GL_VERTEX_SHADER)) != 0);
    assert((fragmentShader = LoadShader("frag.glsl", GL_FRAGMENT_SHADER)) != 0);
    assert((program = glCreateProgram()) != 0);
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
		GLint infoLen = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char *infoLog = malloc(infoLen);
			glGetProgramInfoLog(program, infoLen, NULL, infoLog);
			fprintf(stderr, "Error linking program:\n%s\n", infoLog);
			free(infoLog);
		}
		glDeleteProgram(program);
		exit(1);
    }

    glClearColor(0.15f, 0.15f, 0.15f, 0.15f);
    glViewport(0, 0, width, height);
	// this make index fail!!!!
	//glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glUseProgram(program);

//*
	GLfloat vertex[] = {
		1, 0, 0,
		0, 1, 0,
		-1, 0, 0,
		0, -1, 0,
	};

	GLfloat texcoord[] = {
		1, 1,
		0, 1,
		0, 0,
		1, 0,
	};

	GLushort index[] = {
		0, 1, 2,
		0, 3, 2,
	};

	GLuint VBO[3];
	glGenBuffers(3, VBO);

	GLint pos = glGetAttribLocation(program, "positionIn");
	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(GLfloat) * 3, vertex, GL_STATIC_DRAW);
	glEnableVertexAttribArray(pos);
	glVertexAttribPointer(pos, 3, GL_FLOAT, 0, 0, 0);

	GLint tex = glGetAttribLocation(program, "texcoordIn");
	glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(GLfloat) * 2, texcoord, GL_STATIC_DRAW);
    glEnableVertexAttribArray(tex);
    glVertexAttribPointer(tex, 2, GL_FLOAT, 0, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLushort), index, GL_STATIC_DRAW);
//*/

	GLuint texid;
	GLint texMap = glGetUniformLocation(program, "texMap");
	glUniform1i(texMap, 0); // GL_TEXTURE0
    glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D, texid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	assert(glGetError() == 0);
}

#ifdef _X_WINDOW_SYSTEM_

#include <X11/Xlib.h>

static EGLNativeWindowType CreateNativeWindow(void)
{
    assert((display = XOpenDisplay(NULL)) != NULL);

    int screen = DefaultScreen(display);
    Window root = DefaultRootWindow(display);
    Window window =  XCreateWindow(display, root, 0, 0, 600, 480, 0,
			 DefaultDepth(display, screen), InputOutput,
			 DefaultVisual(display, screen), 
			 0, NULL);
    XMapWindow(display, window);
    XFlush(display);
    return window;
}

#endif

int display_init(int width, int height)
{
#ifdef _X_WINDOW_SYSTEM_
	render_target_init(CreateNativeWindow());
#else
	struct mali_native_window window;
	window.width = width;
	window.height = height;
	render_target_init(&window);
#endif

	init_GLES(width, height);
}

void display_exit(void)
{

}

void render_frame(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

/*
	GLfloat vertex[] = {
		1, 0, 0,
		0, 1, 0,
		-1, 0, 0,
		0, -1, 0,
	};

	GLfloat texcoord[] = {
		1, 1,
		0, 1,
		0, 0,	
		1, 0,
	};

	GLushort index[] = {
		0, 1, 2,
		0, 3, 2,
	};

	GLint pos = glGetAttribLocation(program, "positionIn");
	glEnableVertexAttribArray(pos);
	glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, vertex);

	GLint tex = glGetAttribLocation(program, "texcoordIn");
    glEnableVertexAttribArray(tex);
    glVertexAttribPointer(tex, 2, GL_FLOAT, GL_FALSE, 0, texcoord);
*/

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	//glDrawArrays(GL_TRIANGLES, 0, 3);
	//glDrawArrays(GL_TRIANGLES, 3, 3);

	eglSwapBuffers(display, surface);
}

static uint8_t clamp(float v)
{
	if (v < 0)
		return 0;
	if (v > 255)
		return 255;
	return v;
}

static void yuv2rgb(uint8_t *yuv, uint8_t *rgb, int w, int h)
{
	int i, j;

	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
//V4L2_PIX_FMT_NV12
#if 0 // ITU-R YCbCr to RGB
			float y, u, v;
			y = yuv[i * w + j];
			u = yuv[w * h + (i >> 1) * w + (j & ~0x1)] - 128.0;
			v = yuv[w * h + (i >> 1) * w + (j & ~0x1) + 1] - 128.0;
			rgb[i * w * 3 + j * 3 + 0] = clamp(y + 1.402 * v); // R
			rgb[i * w * 3 + j * 3 + 1] = clamp(y - 0.344 * u - 0.714 * v); // G
			rgb[i * w * 3 + j * 3 + 2] = clamp(y + 1.772 * u); // B
#endif
#if 0 // NTSC YUV to RGB
			int y, u, v;
			y = yuv[i * w + j] - 16;
			u = yuv[w * h + (i >> 1) * w + (j & ~0x1)] - 128;
			v = yuv[w * h + (i >> 1) * w + (j & ~0x1) + 1] - 128;
			rgb[i * w * 3 + j * 3 + 2] = clamp((298 * y + 409 * v + 128) >> 8);
			rgb[i * w * 3 + j * 3 + 1] = clamp((298 * y - 100 * u - 208 * v + 128) >> 8);
			rgb[i * w * 3 + j * 3 + 0] = clamp((298 * y + 516 * u + 128) >> 8);
#endif
#if 1
			rgb[i * w * 3 + j * 3 + 2] = yuv[w * h + (i >> 1) * w + (j & ~0x1) + 1];
			rgb[i * w * 3 + j * 3 + 1] = yuv[w * h + (i >> 1) * w + (j & ~0x1)];
			rgb[i * w * 3 + j * 3 + 0] = yuv[i * w + j];
#endif
		}
	}
}

void update_texture(void *data, int width, int height)
{
	struct timeval ctime;
	static void *image = NULL;
	static int first_time = 1;

	if (!image)
	  assert((image = malloc(width * height * 3)) != NULL);

	yuv2rgb(data, image, width, height);

	if (first_time) {
		printf("create texture %d %d\n", width, height);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		first_time = 0;
	}
	else {
		printf("update texture %d %d\n", width, height);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);
	}

	assert(glGetError() == 0);
}


