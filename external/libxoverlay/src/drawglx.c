/*
 * drawglx.c
 *
 *  Created on: Nov 9, 2017
 *      Author: nullifiedcat
 */

#include "internal/drawglx.h"

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <xoverlay.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h>

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092
typedef GLXContext (*glXCreateContextAttribsARBfn)(Display *, GLXFBConfig,
                                                   GLXContext, Bool,
                                                   const int *);

xoverlay_glx_state glx_state;

// Helper to check for extension string presence.  Adapted from:
//   http://www.opengl.org/resources/features/OGLextensions/
int glx_is_extension_supported(const char *list, const char *extension)
{
    const char *start;
    const char *where, *terminator;

    where = strchr(extension, ' ');
    if (where || *extension == '\0')
        return 0;

    start = list;
    while (1)
    {
        where = strstr(start, extension);

        if (!where)
            break;

        terminator = where + strlen(extension);

        if (where == start || *(where - 1) == ' ')
            if (*terminator == ' ' || *terminator == '\0')
                return 1;

        start = terminator;
    }

    return 0;
}

int xoverlay_glx_init()
{
    glXQueryVersion(xoverlay_library.display, &glx_state.version_major,
                    &glx_state.version_minor);
    return 0;
}

int xoverlay_glx_create_window()
{
    GLint attribs[] = { GLX_X_RENDERABLE,
                        GL_TRUE,
                        GLX_DRAWABLE_TYPE,
                        GLX_WINDOW_BIT,
                        GLX_RENDER_TYPE,
                        GLX_RGBA_BIT,
                        GLX_X_VISUAL_TYPE,
                        GLX_TRUE_COLOR,
                        GLX_DEPTH_SIZE,
                        24,
                        GLX_STENCIL_SIZE,
                        8,
                        GLX_RED_SIZE,
                        8,
                        GLX_GREEN_SIZE,
                        8,
                        GLX_BLUE_SIZE,
                        8,
                        GLX_ALPHA_SIZE,
                        8,
                        GLX_DOUBLEBUFFER,
                        GL_TRUE,
                        None };

    int fbc_count;
    GLXFBConfig *fbc = glXChooseFBConfig(
        xoverlay_library.display, xoverlay_library.screen, attribs, &fbc_count);
    if (fbc == NULL)
    {
        return -1;
    }
    int fbc_best         = -1;
    int fbc_best_samples = -1;
    for (int i = 0; i < fbc_count; ++i)
    {
        XVisualInfo *info =
            glXGetVisualFromFBConfig(xoverlay_library.display, fbc[i]);
        if (info->depth >= 32)
            continue;
        int samples;
        glXGetFBConfigAttrib(xoverlay_library.display, fbc[i], GLX_SAMPLES,
                             &samples);
        if (fbc_best < 0 || samples > fbc_best_samples)
        {
            fbc_best         = i;
            fbc_best_samples = samples;
        }
        XFree(info);
    }
    if (fbc_best == -1)
    {
        return -1;
    }
    GLXFBConfig fbconfig = fbc[fbc_best];
    XFree(fbc);

    XVisualInfo *info =
        glXGetVisualFromFBConfig(xoverlay_library.display, fbconfig);
    if (info == NULL)
    {
        return -1;
    }
    Window root               = DefaultRootWindow(xoverlay_library.display);
    xoverlay_library.colormap = XCreateColormap(xoverlay_library.display, root,
                                                info->visual, AllocNone);
    XSetWindowAttributes attr;
    attr.background_pixel  = 0x0;
    attr.border_pixel      = 0;
    attr.save_under        = 1;
    attr.override_redirect = 1;
    attr.colormap          = xoverlay_library.colormap;
    attr.event_mask        = 0x0;
    attr.do_not_propagate_mask =
        (KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
         PointerMotionMask | ButtonMotionMask);

    unsigned long mask = CWBackPixel | CWBorderPixel | CWSaveUnder |
                         CWOverrideRedirect | CWColormap | CWEventMask |
                         CWDontPropagate;
    xoverlay_library.window =
        XCreateWindow(xoverlay_library.display, root, 0, 0,
                      xoverlay_library.width, xoverlay_library.height, 0,
                      info->depth, InputOutput, info->visual, mask, &attr);
    if (xoverlay_library.window == 0)
    {
        return -1;
    }

    XShapeCombineMask(xoverlay_library.display, xoverlay_library.window,
                      ShapeInput, 0, 0, None, ShapeSet);
    XShapeSelectInput(xoverlay_library.display, xoverlay_library.window,
                      ShapeNotifyMask);

    XserverRegion region =
        XFixesCreateRegion(xoverlay_library.display, NULL, 0);
    XFixesSetWindowShapeRegion(xoverlay_library.display,
                               xoverlay_library.window, ShapeInput, 0, 0,
                               region);
    XFixesDestroyRegion(xoverlay_library.display, region);

    XFree(info);
    XStoreName(xoverlay_library.display, xoverlay_library.window,
               "OverlayWindow");

    xoverlay_show();

    const char *extensions = glXQueryExtensionsString(xoverlay_library.display,
                                                      xoverlay_library.screen);
    glXCreateContextAttribsARBfn glXCreateContextAttribsARB =
        (glXCreateContextAttribsARBfn) glXGetProcAddressARB(
            (const GLubyte *) "glXCreateContextAttribsARB");

    if (!glx_is_extension_supported(extensions, "GLX_ARB_create_context"))
    {
        glx_state.context = glXCreateNewContext(
            xoverlay_library.display, fbconfig, GLX_RGBA_TYPE, NULL, GL_TRUE);
    }
    else
    {
        int ctx_attribs[] = { GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
                              GLX_CONTEXT_MINOR_VERSION_ARB, 0, None };
        glx_state.context = glXCreateContextAttribsARB(
            xoverlay_library.display, fbconfig, NULL, GL_TRUE, ctx_attribs);
        XSync(xoverlay_library.display, GL_FALSE);
    }
    if (glx_state.context == NULL)
    {
        return -1;
    }
    if (!glXIsDirect(xoverlay_library.display, glx_state.context))
        ;
    glXMakeCurrent(xoverlay_library.display, xoverlay_library.window,
                   glx_state.context);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        return -1;
    }
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT =
        (PFNGLXSWAPINTERVALEXTPROC) glXGetProcAddressARB(
            (const GLubyte *) "glXSwapIntervalEXT");
    if (glXSwapIntervalEXT)
        glXSwapIntervalEXT(xoverlay_library.display, xoverlay_library.window,
                           0);
    glXSwapBuffers(xoverlay_library.display, xoverlay_library.window);

    return 0;
}

int xoverlay_glx_destroy()
{
    return 0;
}
