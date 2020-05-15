/*
 * textured-cube version 0.1
 * 
 * Part of the MALI Memory Testing tool (https://github.com/dimitar-kunchev/mali-memtester)
 *
 * This creates a spinning textured cube and displays it on the framebuffer
 * Code is based on bits and pieces from various sources, including:
 *	https://github.com/ssvb/lima-memtester (including the textured-cube demo from the driver)
 * 	http://www.opengl-tutorial.org/beginners-tutorials/tutorial-5-a-textured-cube/
 * 
 * Copyright (C) 2018 Dimitar Kunchev <d.kunchev@gmail.com>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 */

#include "textured-cube.h"

#include "texture.c"

static GLint u_matrix = -1;
static GLint attr_in_position = 0, attr_in_texture_coord = 1, attr_in_texture = 2;

static int view_rotx = 0, view_rotz = 0, view_roty = 0;

#define CUBE_VERTEX_COUNT 36

static const GLfloat g_vertex_buffer_data[] = {
	-1, -1, 1,
	-1, -1, -1,
	-1, 1, -1,	
	-1, 1, -1,
	-1, 1, 1,
	-1, -1, 1,

	-1, 1, 1,
	-1, 1, -1,
	1, 1, -1,
	1, 1, -1,
	1, 1, 1,
	-1, 1, 1,

	1, 1, 1,
	1, 1, -1,
	1, -1, -1,
	1, -1, -1,
	1, -1, 1,
	1, 1, 1,
	
	1, -1, 1,
	1, -1, -1,
	-1, -1, -1,
	-1, -1, -1,
	-1, -1, 1,
	1, -1, 1,

	-1, -1, 1,
	-1, 1, 1,
	1, 1, 1,
	1, 1, 1,
	1, -1, 1,
	-1, -1, 1,
	
	-1, 1, -1,
	1, 1, -1,
	1, -1, -1,
	1, -1, -1,
	-1, -1, -1,
	-1 ,1 ,-1
};

static const GLfloat g_uv_buffer_data[] = {
	0.0f, 		0.0f,
	0.0f, 		1.0f,
	1.0f, 		1.0f,
	1.0f, 		1.0f,
	1.0f, 		0,
	0,		0,

	0.0f, 		0.0f,
	0.0f, 		1.0f,
	1.0f, 		1.0f,
	1.0f, 		1.0f,
	1.0f, 		0,
	0,		0,

	0.0f, 		0.0f,
	0.0f, 		1.0f,
	1.0f, 		1.0f,
	1.0f, 		1.0f,
	1.0f, 		0,
	0,		0,

	0.0f, 		0.0f,
	0.0f, 		1.0f,
	1.0f, 		1.0f,
	1.0f, 		1.0f,
	1.0f, 		0,
	0,		0,

	0.0f, 		0.0f,
	0.0f, 		1.0f,
	1.0f, 		1.0f,
	1.0f, 		1.0f,
	1.0f, 		0,
	0,		0,

	0.0f, 		0.0f,
	0.0f, 		1.0f,
	1.0f, 		1.0f,
	1.0f, 		1.0f,
	1.0f, 		0,
	0,		0,
};

static EGLint const attribute_list[] = {
	EGL_BUFFER_SIZE, 16,
	EGL_RENDERABLE_TYPE,
	EGL_OPENGL_ES2_BIT,
        EGL_NONE
};

static void make_x_rot_matrix(GLfloat angle, GLfloat *m) {
	float c = cos(angle * M_PI / 180.0);
	float s = sin(angle * M_PI / 180.0);
	int i;
	for (i = 0; i < 16; i++)
		m[i] = 0.0;
	m[0] = m[5] = m[10] = m[15] = 1.0;

	m[5] = c;
	m[6] = s;
	m[9] = -s;
	m[10] = c;
}

static void make_y_rot_matrix(GLfloat angle, GLfloat *m) {
	float c = cos(angle * M_PI / 180.0);
	float s = sin(angle * M_PI / 180.0);
	int i;
	for (i = 0; i < 16; i++)
		m[i] = 0.0;
	m[0] = m[5] = m[10] = m[15] = 1.0;

	m[0] = c;
	m[2] = -s;
	m[8] = s;
	m[10] = c;
}

static void make_z_rot_matrix(GLfloat angle, GLfloat *m) {
	float c = cos(angle * M_PI / 180.0);
	float s = sin(angle * M_PI / 180.0);
	int i;
	for (i = 0; i < 16; i++)
		m[i] = 0.0;
	m[0] = m[5] = m[10] = m[15] = 1.0;

	m[0] = c;
	m[1] = s;
	m[4] = -s;
	m[5] = c;
}

static void make_scale_matrix(GLfloat xs, GLfloat ys, GLfloat zs, GLfloat *m) {
	int i;
	for (i = 0; i < 16; i++)
		m[i] = 0.0;
	m[0] = xs;
	m[5] = ys;
	m[10] = zs;
	m[15] = 1.0;
}

static void mul_matrix(GLfloat *prod, const GLfloat *a, const GLfloat *b) {
#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define P(row,col)  p[(col<<2)+row]
	GLfloat p[16];
	GLint i;
	for (i = 0; i < 4; i++) {
		const GLfloat ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
		P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
		P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
		P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
		P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
	}
	memcpy(prod, p, sizeof(p));
#undef A
#undef B
#undef PROD
}

static GLuint loadCubeTexture () {
	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cube_texture.width, cube_texture.height, 0, GL_RGB, GL_UNSIGNED_BYTE, cube_texture.pixel_data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	return textureID;
}

static void create_shaders(void) {
	static const char *fragShaderText =
		"precision mediump float;\n"
		"varying vec2 coord;\n"
		"uniform sampler2D in_texture;\n"
		"void main() {\n"
		"   gl_FragColor = texture2D(in_texture, coord);\n"
		"}\n";
	static const char *vertShaderText =
		"uniform mat4 modelViewProjection;\n"
		"attribute vec4 in_position;\n"
		"attribute vec2 in_texture_coord;\n"
		"varying vec2 coord;\n"
		"void main() {\n"
		"   gl_Position = modelViewProjection * in_position;\n"
		"   coord = in_texture_coord;\n"
	"}\n";

	GLuint fragShader, vertShader, program;
	GLint stat;

	fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	if (fragShader < 1) {
		printf("Error fragment sharer create %d\n", fragShader);
		exit(1);
	}
	glShaderSource(fragShader, 1, (const char **) &fragShaderText, NULL);
	glCompileShader(fragShader);
	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &stat);
	if (!stat) {
		printf("Error: fragment shader did not compile!\n");
		exit(1);
	}

	vertShader = glCreateShader(GL_VERTEX_SHADER);
	if (vertShader < 1) {
		printf("Err vertex shared create %d\n", vertShader);
		exit(1);
	}
	glShaderSource(vertShader, 1, (const char **) &vertShaderText, NULL);
	glCompileShader(vertShader);
	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &stat);
	if (!stat) {
		printf("Error: vertex shader did not compile!\n");
		exit(1);
	}

	program = glCreateProgram();
	glAttachShader(program, fragShader);
	glAttachShader(program, vertShader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &stat);
	if (!stat) {
		char log[1000];
		GLsizei len;
		glGetProgramInfoLog(program, 1000, &len, log);
		printf("Error: linking:\n%s\n", log);
		exit(1);
	}

	glUseProgram(program);

	glBindAttribLocation(program, attr_in_position, "in_position");
	glBindAttribLocation(program, attr_in_texture_coord, "in_texture_coord");
	glBindAttribLocation(program, attr_in_texture, "in_texture");\
	glLinkProgram(program);  /* needed to put attribs into effect */

	u_matrix = glGetUniformLocation(program, "modelViewProjection");
	/*
   	printf("Uniform modelViewProjection at %d\n", u_matrix);
	printf("Attrib in_position at %d\n", attr_in_position);
	printf("Attrib in_texture_coord at %d\n", attr_in_texture_coord);
	printf("Attrib in_texture at %d\n", attr_in_texture); */

	// Prepare the texture
	if (loadCubeTexture() == 0) {
		printf("Error loading texture\n");
		exit(1);
	}
}

static void draw(void) {
	GLfloat mat[16], rot_x[16], rot_y[16], rot_z[16], scale[16];

	/* Set modelview/projection matrix */
	make_x_rot_matrix(view_rotx, rot_x);
	make_y_rot_matrix(view_roty, rot_y);
	make_z_rot_matrix(view_rotz, rot_z);
	make_scale_matrix(0.5, 0.5, 0.5, scale);
	
	mul_matrix(mat, rot_x, rot_y);
	mul_matrix(mat, mat, rot_z);
	mul_matrix(mat, mat, scale);
	glUniformMatrix4fv(u_matrix, 1, GL_FALSE, mat);

	glClearColor(0, 0, 0, 1.0);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glVertexAttribPointer(attr_in_position, 3, GL_FLOAT, GL_FALSE, 0, g_vertex_buffer_data);
	glVertexAttribPointer(attr_in_texture_coord, 2, GL_FLOAT, GL_FALSE,0 , g_uv_buffer_data);
	glEnableVertexAttribArray(attr_in_position);
	glEnableVertexAttribArray(attr_in_texture_coord);

	glDrawArrays(GL_TRIANGLES, 0, CUBE_VERTEX_COUNT);

	glDisableVertexAttribArray(attr_in_position);
	glDisableVertexAttribArray(attr_in_texture_coord);

	view_rotx = (view_rotx + 3) % 360;
	view_roty = (view_roty + 1) % 360;
	view_rotz = (view_rotz + 2) % 360;
}

//******************************************
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <errno.h>

static struct {
    EGLDisplay display;
    EGLConfig config;
    EGLContext context;
    EGLSurface surface;
    GLuint program;
    GLint modelviewmatrix, modelviewprojectionmatrix, normalmatrix;
    GLuint vbo;
    GLuint positionsoffset, colorsoffset, normalsoffset;
} gl;

static struct {
    struct gbm_device *dev;
    struct gbm_surface *surface;
} gbm;

static struct {
    int fd;
    drmModeModeInfo *mode;
    uint32_t crtc_id;
    uint32_t connector_id;
} drm;

struct drm_fb {
    struct gbm_bo *bo;
    uint32_t fb_id;
};

static uint32_t find_crtc_for_encoder(const drmModeRes *resources,
                      const drmModeEncoder *encoder) {
    int i;

    for (i = 0; i < resources->count_crtcs; i++) {
        /* possible_crtcs is a bitmask as described here:
         * https://dvdhrm.wordpress.com/2012/09/13/linux-drm-mode-setting-api
         */
        const uint32_t crtc_mask = 1 << i;
        const uint32_t crtc_id = resources->crtcs[i];
        if (encoder->possible_crtcs & crtc_mask) {
            return crtc_id;
        }
    }

    /* no match found */
    return -1;
}

static uint32_t find_crtc_for_connector(const drmModeRes *resources,
                    const drmModeConnector *connector) {
    int i;

    for (i = 0; i < connector->count_encoders; i++) {
        const uint32_t encoder_id = connector->encoders[i];
        drmModeEncoder *encoder = drmModeGetEncoder(drm.fd, encoder_id);

        if (encoder) {
            const uint32_t crtc_id = find_crtc_for_encoder(resources, encoder);

            drmModeFreeEncoder(encoder);
            if (crtc_id != 0) {
                return crtc_id;
            }
        }
    }

    /* no match found */
    return -1;
}

static void
drm_fb_destroy_callback(struct gbm_bo *bo, void *data)
{
    struct drm_fb *fb = data;
    struct gbm_device *gbm = gbm_bo_get_device(bo);

    if (fb->fb_id)
        drmModeRmFB(drm.fd, fb->fb_id);

    free(fb);
}

static struct drm_fb * drm_fb_get_from_bo(struct gbm_bo *bo)
{
    struct drm_fb *fb = gbm_bo_get_user_data(bo);
    uint32_t width, height, stride, handle;
    int ret;

    if (fb)
        return fb;

    fb = calloc(1, sizeof *fb);
    fb->bo = bo;

    width = gbm_bo_get_width(bo);
    height = gbm_bo_get_height(bo);
    stride = gbm_bo_get_stride(bo);
    handle = gbm_bo_get_handle(bo).u32;

    ret = drmModeAddFB(drm.fd, width, height, 24, 32, stride, handle, &fb->fb_id);
    if (ret) {
        printf("failed to create fb: %s\n", strerror(errno));
        free(fb);
        return NULL;
    }

    gbm_bo_set_user_data(bo, fb, drm_fb_destroy_callback);

    return fb;
}

static void page_flip_handler(int fd, unsigned int frame,
          unsigned int sec, unsigned int usec, void *data)
{
    int *waiting_for_flip = data;
    *waiting_for_flip = 0;
}

static int init_drm(void)
{
    drmModeRes *resources;
    drmModeConnector *connector = NULL;
    drmModeEncoder *encoder = NULL;
    int i, area;

    drm.fd = open("/dev/dri/card1", O_RDWR);
    
    if (drm.fd < 0) {
        printf("could not open drm device\n");
        return -1;
    }

    resources = drmModeGetResources(drm.fd);
    if (!resources) {
        printf("drmModeGetResources failed: %s\n", strerror(errno));
        return -1;
    }

    /* find a connected connector: */
    for (i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(drm.fd, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED) {
            /* it's connected, let's use this! */
            break;
        }
        drmModeFreeConnector(connector);
        connector = NULL;
    }

    if (!connector) {
        /* we could be fancy and listen for hotplug events and wait for
         * a connector..
         */
        printf("no connected connector!\n");
        return -1;
    }

    /* find prefered mode or the highest resolution mode: */
    for (i = 0, area = 0; i < connector->count_modes; i++) {
        drmModeModeInfo *current_mode = &connector->modes[i];

        if (current_mode->type & DRM_MODE_TYPE_PREFERRED) {
            drm.mode = current_mode;
        }

        int current_area = current_mode->hdisplay * current_mode->vdisplay;
        if (current_area > area) {
            drm.mode = current_mode;
            area = current_area;
        }
    }

    if (!drm.mode) {
        printf("could not find mode!\n");
        return -1;
    }

    /* find encoder: */
    for (i = 0; i < resources->count_encoders; i++) {
        encoder = drmModeGetEncoder(drm.fd, resources->encoders[i]);
        if (encoder->encoder_id == connector->encoder_id)
            break;
        drmModeFreeEncoder(encoder);
        encoder = NULL;
    }

    if (encoder) {
        drm.crtc_id = encoder->crtc_id;
    } else {
        uint32_t crtc_id = find_crtc_for_connector(resources, connector);
        if (crtc_id == 0) {
            printf("no crtc found!\n");
            return -1;
        }

        drm.crtc_id = crtc_id;
    }

    drm.connector_id = connector->connector_id;

    return 0;
}

static int init_gbm(void)
{
    gbm.dev = gbm_create_device(drm.fd);

    gbm.surface = gbm_surface_create(gbm.dev,
            drm.mode->hdisplay, drm.mode->vdisplay,
            GBM_FORMAT_XRGB8888,
            GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!gbm.surface) {
        printf("failed to create gbm surface\n");
        return -1;
    }

    return 0;
}

static int init_gl(void)
{
    EGLint major, minor, n;
    GLuint vertex_shader, fragment_shader;
    GLint ret;

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    static const EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display = NULL;
    get_platform_display = (void *) eglGetProcAddress("eglGetPlatformDisplayEXT");
    assert(get_platform_display != NULL);

    gl.display = get_platform_display(EGL_PLATFORM_GBM_KHR, gbm.dev, NULL);

    if (!eglInitialize(gl.display, &major, &minor)) {
        printf("failed to initialize\n");
        return -1;
    }

    printf("Using display %p with EGL version %d.%d\n",
            gl.display, major, minor);

    printf("EGL Version \"%s\"\n", eglQueryString(gl.display, EGL_VERSION));
    printf("EGL Vendor \"%s\"\n", eglQueryString(gl.display, EGL_VENDOR));
    printf("EGL Renderer: \"%s\"\n", eglQueryString(gl.display, GL_RENDERER));
    printf("EGL Extensions \"%s\"\n", eglQueryString(gl.display, EGL_EXTENSIONS));

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        printf("failed to bind api EGL_OPENGL_ES_API\n");
        return -1;
    }

    if (!eglChooseConfig(gl.display, config_attribs, &gl.config, 1, &n) || n != 1) {
        printf("failed to choose config: %d\n", n);
        return -1;
    }

    gl.context = eglCreateContext(gl.display, gl.config, EGL_NO_CONTEXT, context_attribs);
    if (gl.context == NULL) {
        printf("failed to create context\n");
        return -1;
    }

    gl.surface = eglCreateWindowSurface(gl.display, gl.config, gbm.surface, NULL);
    if (gl.surface == EGL_NO_SURFACE) {
        printf("failed to create egl surface\n");
        return -1;
    }

    /* connect the context to the surface */
    eglMakeCurrent(gl.display, gl.surface, gl.surface, gl.context);

    return 0;
}
//******************************************

int textured_cube_main (int * stop_flag) {
	int width = 480, height = 272;
    fd_set fds;
    drmEventContext evctx = {
            .version = DRM_EVENT_CONTEXT_VERSION,
            .page_flip_handler = page_flip_handler,
    };

    struct gbm_bo *bo;
    struct drm_fb *fb;
    uint32_t i = 0, w, h;
    int ret;

    ret = init_drm();
    if (ret) {
        printf("failed to initialize DRM\n");
        return ret;
    }

    FD_ZERO(&fds);
    FD_SET(0, &fds);
    FD_SET(drm.fd, &fds);

    ret = init_gbm();
    if (ret) {
        printf("failed to initialize GBM\n");
        return ret;
    }

    ret = init_gl();
    if (ret) {
        printf("failed to initialize EGL\n");
        return ret;
    }
        
    /* clear the color buffer */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(gl.display, gl.surface);
    bo = gbm_surface_lock_front_buffer(gbm.surface);
    fb = drm_fb_get_from_bo(bo);

    /* set mode: */
    ret = drmModeSetCrtc(drm.fd, drm.crtc_id, fb->fb_id, 0, 0,
            &drm.connector_id, 1, drm.mode);
    if (ret) {
        printf("failed to set mode: %s\n", strerror(errno));
        return ret;
    }

    w = gbm_bo_get_width(bo);
    h = gbm_bo_get_height(bo);

    int x = (w - width)/2;
    int y = (h - height)/2;

	glViewport(x, y, width, height);
	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);    // Set the type of depth-test
	glShadeModel(GL_SMOOTH);   // Enable smooth shading
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  // Nice perspective corrections
	
	create_shaders();
	
    /* clear the color buffer */
    glClearColor(0, .6, .3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
    eglSwapBuffers(gl.display, gl.surface);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLfloat aspect = 480.0/272.0; // (GLfloat)width / (GLfloat)height;
	float m_w = 4;
	float m_h = m_w / aspect;
	glFrustumf(-m_w/2.0, m_w/2.0, -m_h/2.0, m_h/2.0, -2, 20);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//printf("Starting loop\n");
    while (!(*stop_flag)) {
    	struct gbm_bo *next_bo;
        int waiting_for_flip = 1;

		draw();
		eglSwapBuffers(gl.display, gl.surface);

		next_bo = gbm_surface_lock_front_buffer(gbm.surface);
        fb = drm_fb_get_from_bo(next_bo);
		
		ret = drmModePageFlip(drm.fd, drm.crtc_id, fb->fb_id,
                DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip);
        if (ret) {
            printf("failed to queue page flip: %s\n", strerror(errno));
            return -1;
        }

        while (waiting_for_flip) {
            ret = select(drm.fd + 1, &fds, NULL, NULL, NULL);
            if (ret < 0) {
                printf("select err: %s\n", strerror(errno));
                return ret;
            } else if (ret == 0) {
                printf("select timeout!\n");
                return -1;
            } else if (FD_ISSET(0, &fds)) {
                printf("user interrupted!\n");
                break;
            }
            drmHandleEvent(drm.fd, &evctx);
        }

        /* release last buffer to render on again: */
        gbm_surface_release_buffer(gbm.surface, bo);
        bo = next_bo;
	};

	glClearColor(0, 0, 0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
    eglSwapBuffers(gl.display, gl.surface);

    return EXIT_SUCCESS;
}

