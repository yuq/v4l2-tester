#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <epoxy/gl.h>
#include <epoxy/egl.h>

#include <linux/videodev2.h>

#include <drm/drm_fourcc.h>

#include <X11/Xlib.h>

EGLDisplay display;
EGLSurface surface;
GLuint program;
int windowWidth = 1280 / 2;
int windowHeight = 1024;
int fd;
GLuint textures[5];

#define FRAME_RATE  20
#define PI  3.14159265758


EGLNativeWindowType CreateNativeWindow(void)
{
    Display *display;
    assert((display = XOpenDisplay(NULL)) != NULL);

    int screen = DefaultScreen(display);
    Window root = DefaultRootWindow(display);
    Window window =  XCreateWindow(display, root, 0, 0, windowWidth, windowHeight, 0,
								   DefaultDepth(display, screen), InputOutput,
								   DefaultVisual(display, screen), 
								   0, NULL);
    XMapWindow(display, window);
    XFlush(display);
    return window;
}

void RenderTargetInit(EGLNativeWindowType nativeWindow)
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

GLuint LoadShader(const char *name, GLenum type)
{
    FILE *f;
    int size;
    char *buff;
    GLuint shader;
    GLint compiled;
    const GLchar *source[1];

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

void InitGLES(void)
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
    glViewport(0, 0, windowWidth, windowHeight);
    glEnable(GL_DEPTH_TEST);

    glUseProgram(program);

	GLfloat vertex[] = {
		-1, -1, 0,
		-1, 1, 0,
		1, 1, 0,
		1, -1, 0,
	};

	GLfloat texcoord[] = {
		0, 1,
		0, 0,
		1, 0,
		1, 1,
	};

	GLushort index[] = {
		0, 1, 3,
		1, 2, 3,
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

	GLuint texid;
	GLint texMap = glGetUniformLocation(program, "texMap");
	glUniform1i(texMap, 0); // GL_TEXTURE0
    glActiveTexture(GL_TEXTURE0);
}

void InitVideo(void)
{
	assert(epoxy_has_egl_extension(display, "EGL_MESA_image_dma_buf_export"));
	assert(epoxy_has_egl_extension(display, "EGL_EXT_image_dma_buf_import"));
	assert(epoxy_has_gl_extension("GL_OES_EGL_image"));

	fd = open("/dev/video0", O_RDWR);
	assert(fd >= 0);

	struct v4l2_capability cap;
    assert(!ioctl(fd, VIDIOC_QUERYCAP, &cap));
	assert(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE);
	assert(cap.capabilities & V4L2_CAP_STREAMING);

	//struct v4l2_dbg_chip_info chip;
	//assert(!ioctl(fd, VIDIOC_DBG_G_CHIP_INFO, &chip));
	//printf("chip info %s\n", chip.name);

	int support_fmt;
    struct v4l2_fmtdesc ffmt;
    memset(&ffmt, 0, sizeof(ffmt));
    ffmt.index = 0;
    ffmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    support_fmt = 0;
    while (!ioctl(fd, VIDIOC_ENUM_FMT, &ffmt)) {
        printf("fmt %d %s\n", ffmt.pixelformat, ffmt.description);
        if (ffmt.pixelformat == V4L2_PIX_FMT_YUYV)
            support_fmt = 1;
        ffmt.index++;
    }
    assert(support_fmt);

	int support_size;
    struct v4l2_frmsizeenum fsize;
    memset(&fsize, 0, sizeof(fsize));
    fsize.index = 0;
    fsize.pixel_format = V4L2_PIX_FMT_YUYV;
    support_size = 0;
    while (!ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsize)) {
        printf("frame size %dx%d\n", fsize.discrete.width, fsize.discrete.height);
        if (fsize.discrete.width == 1280 && fsize.discrete.height == 1024)
            support_size = 1;
        fsize.index++;
    }
    assert(support_size);

	struct v4l2_input input;
    memset(&input, 0, sizeof(input));
    input.index = 0;
    while (!ioctl(fd, VIDIOC_ENUMINPUT, &input)) {
        printf("input name=%s type=%d status=%d std=%d\n",
			   input.name, input.type, input.status, input.std);
        input.index++;
    }

	int index;
	assert(!ioctl(fd, VIDIOC_G_INPUT, &index));
	printf("current input index=%d\n", index);

	struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    assert(!ioctl(fd, VIDIOC_G_FMT, &fmt));
	printf("fmt width=%d height=%d pfmt=%d\n",
		   fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.pixelformat);

    fmt.fmt.pix.width = 1280;
    fmt.fmt.pix.height = 1024;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    assert(!ioctl(fd, VIDIOC_S_FMT, &fmt));

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    assert(!ioctl(fd, VIDIOC_G_FMT, &fmt));
    assert(fmt.fmt.pix.width == 1280);
    assert(fmt.fmt.pix.height == 1024);
    assert(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV);

	struct v4l2_requestbuffers reqbuf;
    reqbuf.count = 5;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    assert(!ioctl(fd, VIDIOC_REQBUFS, &reqbuf));
    assert(reqbuf.count == 5);

    int i;
	glGenTextures(5, textures);
	for (i = 0; i < 5; i++) {
		struct v4l2_exportbuffer expbuf;
		memset(&expbuf, 0, sizeof(expbuf));
		expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		expbuf.index = i;
		expbuf.flags = O_RDWR;
		assert(!ioctl(fd, VIDIOC_EXPBUF, &expbuf));

		EGLint attrib_list[] = {
			EGL_WIDTH, windowWidth,
			EGL_HEIGHT, windowHeight,
			EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_ARGB8888,
			EGL_DMA_BUF_PLANE0_FD_EXT, expbuf.fd,
			EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
			EGL_DMA_BUF_PLANE0_PITCH_EXT, windowWidth * 4,
			EGL_NONE
		};
		EGLImageKHR image = eglCreateImageKHR(
			display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT,
			NULL, attrib_list);
		assert(image != EGL_NO_IMAGE_KHR);

		glBindTexture(GL_TEXTURE_2D, textures[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
		glBindTexture(GL_TEXTURE_2D, 0);

		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.index = i;
		buf.memory = V4L2_MEMORY_MMAP;
		//assert(!ioctl(fd, VIDIOC_QUERYBUF, &buf));
		assert(!ioctl(fd, VIDIOC_QBUF, &buf));
	}
}

void Render(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	eglSwapBuffers(display, surface);

	int a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	assert(!ioctl(fd, VIDIOC_STREAMON, &a));
    
    while (1) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		assert(!ioctl(fd, VIDIOC_DQBUF, &buf));

		glBindTexture(GL_TEXTURE_2D, textures[buf.index]);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

		eglSwapBuffers(display, surface);

		glBindTexture(GL_TEXTURE_2D, 0);

		assert(!ioctl(fd, VIDIOC_QBUF, &buf));
    }

	a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	assert(!ioctl(fd, VIDIOC_STREAMOFF, &a));
}

int main(void)
{
    EGLNativeWindowType window;
    window = CreateNativeWindow();
    RenderTargetInit(window);
    InitGLES();
	InitVideo();
    Render();
    return 0;
}
