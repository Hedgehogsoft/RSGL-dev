#define RSGL_IMPLEMENTATION
#include "RSGL.h"
#include "RSGL_soft.h"

#define RGFW_INT_DEFINED
#define RGFW_IMPLEMENTATION
#include "examples/deps/RGFW.h"

int main() {
	RGFW_window* window = RGFW_createWindow("window", 0, 0, 500, 500, RGFW_windowNoResize | RGFW_windowCenter);

    u8* buffer = (u8*)RGFW_ALLOC((u32)(500 * 500 * 4));
    RGFW_surface* surface = RGFW_createSurface(buffer, 500, 500, RGFW_formatRGBA8);

	RSGL_renderer* renderer = RSGL_renderer_init(RSGL_soft_rendererProc(), NULL);
    RSGL_renderer_viewport(renderer, RSGL_RECT(0, 0, 500, 500));
	RSGL_renderer_updateSize(renderer, 500, 500);

	RSGL_softSurface softSurface;
	softSurface.data = buffer;
	softSurface.width = 500;
	softSurface.height = 500;

	RSGL_renderer_setSurface(renderer, &softSurface);

    RSGL_vec3D rotate = RSGL_VEC3D(0, 0, 0);
	while (RGFW_window_shouldClose(window) == RGFW_FALSE) {
		RGFW_pollEvents();

		RSGL_renderer_clear(renderer, RSGL_RGB(10, 50, 100));

        rotate.z += 1;

		RSGL_renderer_setRotate(renderer, rotate);
		RSGL_renderer_setColor(renderer, RSGL_RGB(0, 255, 0));
		RSGL_drawPolygon(renderer, RSGL_RECT(20, 20, 50, 50), 8);

		RSGL_renderer_setColor(renderer, RSGL_RGBA(255, 0, 0, 255));
		RSGL_drawRect(renderer, RSGL_RECT(250, 300, 40, 40));

		RSGL_renderer_setRotate(renderer, rotate);
		RSGL_renderer_setColor(renderer, RSGL_RGBA(255, 0, 0, 200));
		RSGL_drawRect(renderer, RSGL_RECT(200, 200, 200, 200));

		RSGL_renderer_setRotate(renderer, rotate);
		RSGL_renderer_setColor(renderer, RSGL_RGBA(0, 0, 255, 255));

		RSGL_vec3D points[3] = {RSGL_VEC3D(0, 500, 0), RSGL_VEC3D(200, 500, 0), RSGL_VEC3D(100, 250, 0)};
		RSGL_drawTriangle(renderer, points);

		RSGL_renderer_render(renderer);

        RGFW_window_blitSurface(window, surface);
	}

    RGFW_surface_free(surface);
	RGFW_FREE(buffer);

	RSGL_renderer_free(renderer);
	RGFW_window_close(window);
}
