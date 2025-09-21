#ifndef RSGL_H
#include "RSGL.h"
#endif

#ifndef GL_TEXTURE_SWIZZLE_RGBA
	#define GL_TEXTURE_SWIZZLE_RGBA 0x8E46
#endif

#ifndef RSGL_soft_H
#define RSGL_soft_H

typedef struct RSGL_softSurface {
	u8* data;
	size_t width;
	size_t height;
} RSGL_softSurface;

typedef struct RSGL_softRenderer {
	RSGL_softSurface* surf;
} RSGL_softRenderer;

RSGLDEF RSGL_rendererProc RSGL_soft_rendererProc(void);
RSGLDEF size_t RSGL_soft_size(void);

RSGLDEF RSGL_renderer* RSGL_soft_renderer_init(void* loader);
RSGLDEF void RSGL_soft_renderer_initPtr(void* loader, RSGL_softRenderer* ptr, RSGL_renderer* renderer);

RSGLDEF void RSGL_soft_render(RSGL_softRenderer* ctx, const RSGL_renderPass* pass);
RSGLDEF void RSGL_soft_initPtr(RSGL_softRenderer* ctx, void* proc); /* init render backend */
RSGLDEF void RSGL_soft_freePtr(RSGL_softRenderer* ctx); /* free render backend */
RSGLDEF void RSGL_soft_clear(RSGL_softRenderer* ctx, RSGL_framebuffer framebuffer, float r, float g, float b, float a);
RSGLDEF void RSGL_soft_viewport(RSGL_softRenderer* ctx, i32 x, i32 y, i32 w, i32 h);
RSGLDEF void RSGL_soft_createBuffer(RSGL_softRenderer* ctx, size_t size, const void* data, size_t* buffer);
RSGLDEF void RSGL_soft_updateBuffer(RSGL_softRenderer* ctx, size_t buffer, const void* data, size_t start, size_t end);
RSGLDEF void RSGL_soft_deleteBuffer(RSGL_softRenderer* ctx, size_t buffer);
RSGLDEF void RSGL_soft_setSurface(RSGL_softRenderer* ctx, RSGL_softSurface* surf);
/* create a texture based on a given bitmap, this must be freed later using RSGL_deleteTexture or opengl*/
RSGLDEF RSGL_texture RSGL_soft_createTexture(RSGL_softRenderer* ctx, const RSGL_textureBlob* blob);
RSGLDEF void RSGL_soft_copyToTexture(RSGL_softRenderer* ctx, RSGL_texture texture, size_t x, size_t y, const RSGL_textureBlob* blob);
/* delete a texture */
RSGLDEF void RSGL_soft_deleteTexture(RSGL_softRenderer* ctx, RSGL_texture tex);
/* starts scissoring */
RSGLDEF void RSGL_soft_scissorStart(RSGL_softRenderer* ctx, float x, float y, float w, float h, float renderer_height);
/* stops scissoring */
RSGLDEF void RSGL_soft_scissorEnd(RSGL_softRenderer* ctx);
#endif

#ifdef RSGL_IMPLEMENTATION

typedef struct RSGL_softTexture {
	u8* data;
	size_t width;
	size_t height;
	size_t depth;
} RSGL_softTexture;

void RSGL_soft_setSurface(RSGL_softRenderer* ctx, RSGL_softSurface* surf) {
	ctx->surf = surf;
}

typedef struct RSGL_softRenderMap {
	float* verts;
	float* colors;

	float* texCoords;
	u16* elements;
	size_t index;
	size_t count;
	RSGL_softTexture* tex;
	RSGL_mat4 matrix;
} RSGL_softRenderMap;

typedef struct RSGL_softRenderCall {
	RSGL_vec3D* points;
	RSGL_color* colors;
	RSGL_softTexture* tex;
	RSGL_vec2D* texPoints;
} RSGL_softRenderCall;

void RSGL_softRenderMapElement(RSGL_softRenderer* ctx, const RSGL_softRenderMap* map, RSGL_softRenderCall* call) {
	size_t i;

	for (i = 0; i < map->count; i++) {
		size_t eIndex = map->elements[map->index + i];
		size_t vIndex = eIndex * 3;
		size_t cIndex = eIndex * 4;
		size_t tIndex = eIndex * 2;

		RSGL_vec3D point = {map->verts[vIndex + 0], map->verts[vIndex + 1], map->verts[vIndex + 2]};
		point = RSGL_mat4_multiplyPoint(map->matrix, point);

		point.x = ((point.x + 1.0f) / 2.0f) * ctx->surf->width;
		point.y = (1.0f - ((point.y + 1.0f) / 2.0f)) * ctx->surf->height;

		call->points[i] = point;


		RSGL_color color = RSGL_RGBA((u8)(map->colors[cIndex + 0] * 255.0f),
						  (u8)(map->colors[cIndex + 1] * 255.0f),
							(u8)(map->colors[cIndex + 2] * 255.0f),
							(u8)(map->colors[cIndex + 3] * 255.0f)
						  );

		call->colors[i] = color;

		RSGL_vec2D texCoord = RSGL_VEC2D(map->texCoords[tIndex], map->texCoords[tIndex + 1]);
		texCoord.x *= map->tex->width;
		texCoord.y *= map->tex->height;
		call->texPoints[i] = texCoord;
	}
}

void RSGL_softDrawPoint(RSGL_softRenderer* ctx, const RSGL_softRenderCall* call) {
	RSGL_vec3D point = call->points[0];

	//size_t index = ((point.y) * ctx->surf->width * 4) + (point.x) * 4;
	size_t index = (((u32)point.y) * ctx->surf->width * 4) + ((u32)point.x) * 4;
	if ((index + 3) >= (ctx->surf->width * ctx->surf->height * 4))
		return;

	point = RSGL_VEC3D(call->texPoints[0].x, call->texPoints[0].y, 0);
	size_t tIndex = (size_t)((point.y * ((float)call->tex->width) * 4.0f) + (point.x) * 4.0f);

	RSGL_UNUSED(tIndex);
//printf("%f %f %li\n", point.x, point.y, tIndex);
/*	float blending_texture[4] = {
		call->tex->data[tIndex] / 255.0f,
		call->tex->data[tIndex + 1] / 255.0f,
		call->tex->data[tIndex + 2] / 255.0f,
		call->tex->data[tIndex + 3] / 255.0f
	};
*/
	//RSGL_UNUSED(blending_texture);

	float blending_background[4] = {
		ctx->surf->data[index + 0] * (1.0f - ((float)call->colors[0].a / 255.0f)),
		ctx->surf->data[index + 1] * (1.0f - ((float)call->colors[0].a / 255.0f)),
		ctx->surf->data[index + 2] * (1.0f - ((float)call->colors[0].a / 255.0f)),
		ctx->surf->data[index + 3] * (1.0f - ((float)call->colors[0].a / 255.0f))
	};

	float blending_color[4] = {
		call->colors[0].r * (((float)call->colors[0].a / 255.0f)),
		call->colors[0].g * (((float)call->colors[0].a / 255.0f)),
		call->colors[0].b * ((float)call->colors[0].a / 255.0f),
		call->colors[0].a * ((float)call->colors[0].a / 255.0f)
	};


	RSGL_color color2 = RSGL_RGBA(blending_color[0] + blending_background[0],
							   blending_color[1] + blending_background[1],
							   blending_color[2] + blending_background[2],
							   blending_color[3] + blending_background[3]);

	ctx->surf->data[index + 0] = color2.r;
	ctx->surf->data[index + 1] = color2.g;
	ctx->surf->data[index + 2] = color2.b;
	ctx->surf->data[index + 3] = color2.a;
}

void RSGL_softDrawLine(RSGL_softRenderer* ctx, const RSGL_softRenderCall* call) {
	RSGL_softRenderCall callee;
	RSGL_vec3D points[2];
	RSGL_MEMCPY(points, call->points, sizeof(points));

	RSGL_vec3D start = call->points[0];
	RSGL_vec3D end = call->points[1];

	float slopeX = (end.x - start.x);
	float slopeY = (end.y - start.y);

	float steps = fabs(slopeY);
	if (steps < fabs(slopeX)) {
		steps = fabs(slopeX);
	}

	slopeX /= steps;
	slopeY /= steps;

	RSGL_vec3D point = start;
	for (float i = 0; i < steps; i++) {
		callee.points = &point;
		callee.tex = call->tex;
		callee.texPoints = call->texPoints;
		callee.colors = call->colors;
		RSGL_softDrawPoint(ctx, &callee);

		point.x += slopeX;
		point.y += slopeY;
	}
}

void RSGL_vec3DSwap(RSGL_vec3D* v1, RSGL_vec3D* v2) {
	RSGL_vec3D save = *v1;
	*v1 = *v2;
	*v2 = *(&save);
}


RSGL_vec2D RSGL_softGetTexCoordInTriangle(RSGL_vec3D point, RSGL_vec3D* points, RSGL_vec2D* texCoords, RSGL_softTexture* tex) {
	RSGL_vec2D texCoord = RSGL_VEC2D(0, 0);

	size_t top_index = 0;
	float top_value = 100000000;

	for (size_t i = 0; i < 3; i++) {
		float value = fabsf(point.x - points[i].x) + fabsf(point.y - points[i].y) + fabsf(point.z - points[i].z);
		if (value < top_value) {
			top_index = i;
			value = top_value;
		}
	}

	RSGL_vec3D closest = points[top_index];
	texCoord = texCoords[top_index];

	texCoord.x += (closest.x - point.x) / tex->width;
	texCoord.y += (closest.y - point.y) / tex->height;

	printf("%f\n", (closest.x - point.x) / (float)tex->width);

	return texCoord;
}

void RSGL_softDrawTriangle(RSGL_softRenderer* ctx, const RSGL_softRenderCall* call) {
	RSGL_softRenderCall callee;
	RSGL_vec3D points[3];
	RSGL_MEMCPY(points, call->points, sizeof(points));

	callee.colors = call->colors;
	callee.texPoints = call->texPoints;
	callee.tex = call->tex;

    if(points[0].y > points[1].y) RSGL_vec3DSwap((RSGL_vec3D*)&points[0], (RSGL_vec3D*)&points [1]);
    if(points[0].y > points[2].y) RSGL_vec3DSwap((RSGL_vec3D*)&points[0], (RSGL_vec3D*)&points[2]);
    if(points[1].y > points[2].y) RSGL_vec3DSwap((RSGL_vec3D*)&points[1], (RSGL_vec3D*)&points[2]);

    RSGL_vec3D delta_vector_ab = {
        points[1].x - points[0].x,
        points[1].y - points[0].y
    };

    RSGL_vec3D delta_vector_ac = {
        points[2].x - points[0].x,
        points[2].y - points[0].y
    };

    RSGL_vec3D delta_vector_cb = {
        points[1].x - points[2].x,
        points[1].y - points[2].y
    };

    RSGL_vec3D delta_vector_ca = {
        points[0].x - points[2].x,
        points[0].y - points[2].y
    };

    for(float y = points[0].y; y < points[1].y; y++) {
		if(y < 0 || y > ctx->surf->height)
			continue;

		float s1 = delta_vector_ab.y != 0 ?
			(y - points[0].y) * delta_vector_ab.x / delta_vector_ab.y + points[0].x :
			points[0].x;

		float s2 = delta_vector_ac.y != 0 ?
			(y - points[0].y) * delta_vector_ac.x / delta_vector_ac.y + points[0].x :
			points[0].x;

		if(s1 > s2) {
			float b = s1;
			s1 = s2;
			s2 = b;
		}

		for(float x = s1; x <= s2; x++) {
			RSGL_vec3D point = RSGL_VEC3D(x, y, 0);
			RSGL_vec2D texPoint = RSGL_softGetTexCoordInTriangle(point, call->points, call->texPoints, call->tex);
			callee.points = &point;
			callee.texPoints = &texPoint;
			RSGL_softDrawPoint(ctx, &callee);
		}
    }

    for(float y = points[1].y; y < points[2].y; y++) {
        if(y < 0 || y > ctx->surf->height)
			continue;

		float s1 = delta_vector_cb.y != 0 ?
			(y - points[2].y) * delta_vector_cb.x / delta_vector_cb.y + points[2].x :
			points[2].x;

		float s2 = delta_vector_ca.y != 0 ?
			(y - points[2].y) * delta_vector_ca.x / delta_vector_ca.y + points[2].x :
			points[2].x;

		if(s1 > s2) {
			float b = s1;
			s1 = s2;
			s2 = b;
		}

		for(float x = s1; x <= s2; x++) {
			RSGL_vec3D point = RSGL_VEC3D(x, y, 0);
			RSGL_vec2D texPoint = RSGL_softGetTexCoordInTriangle(point, call->points, call->texPoints, call->tex);
			callee.points = &point;
			callee.texPoints = &texPoint;
			RSGL_softDrawPoint(ctx, &callee);
		}
	}
}

RSGL_renderer* RSGL_soft_renderer_init(void* loader) { return RSGL_renderer_init(RSGL_soft_rendererProc(), loader); }
void RSGL_soft_renderer_initPtr(void* loader, RSGL_softRenderer* ptr, RSGL_renderer* renderer) { return RSGL_renderer_initPtr(RSGL_soft_rendererProc(), loader, ptr, renderer); }

size_t RSGL_soft_size(void) {
	return sizeof(RSGL_softRenderer);
}

RSGL_rendererProc RSGL_soft_rendererProc() {
	RSGL_rendererProc proc;
	RSGL_MEMSET(&proc, 0, sizeof(proc));

	proc.render = (void (*)(void*, const RSGL_renderPass* pass))RSGL_soft_render;
    proc.size = (size_t (*)(void))RSGL_soft_size;
    proc.initPtr = (void (*)(void*, void*))RSGL_soft_initPtr;
    proc.freePtr = (void (*)(void*))RSGL_soft_freePtr;
    proc.clear = (void (*)(void*, RSGL_framebuffer, float, float, float, float))RSGL_soft_clear;
    proc.viewport = (void (*)(void*, i32, i32, i32, i32))RSGL_soft_viewport;
    proc.createTexture = (RSGL_texture (*)(void*, const RSGL_textureBlob*))RSGL_soft_createTexture;
    proc.copyToTexture = (void (*)(void*, RSGL_texture, size_t, size_t, const RSGL_textureBlob* blob))RSGL_soft_copyToTexture;
    proc.deleteTexture = (void (*)(void*, RSGL_texture))RSGL_soft_deleteTexture;
    proc.scissorStart = (void (*)(void*, float, float, float, float, float))RSGL_soft_scissorStart;
    proc.scissorEnd =  (void (*)(void*))RSGL_soft_scissorEnd;
	proc.createBuffer = (void (*)(void*, size_t, const void*, size_t*))RSGL_soft_createBuffer;
	proc.updateBuffer = (void (*)(void*, size_t, void*, size_t, size_t))RSGL_soft_updateBuffer;
	proc.deleteBuffer = (void (*)(void*, size_t))RSGL_soft_deleteBuffer;
	proc.setSurface = (void (*)(void*, void*))RSGL_soft_setSurface;
	return proc;
}

void RSGL_soft_viewport(RSGL_softRenderer* ctx, i32 x, i32 y, i32 w, i32 h) {

}

void RSGL_soft_clear(RSGL_softRenderer* ctx, RSGL_framebuffer framebuffer, float r, float g, float b, float a) {
	RSGL_softSurface surf = *ctx->surf;
	if (framebuffer) {
		RSGL_softSurface framebufferSurface = {0};
		/* TODO */
		surf = framebufferSurface;
	}

	u8 color[] = {(u8)(r * 255.0f), (u8)(g * 255.0f), (u8)(b * 255.0f), (u8)(a * 255.0f)};

	for (size_t i = 0; i < surf.width * surf.height * 4; i += 4) {
		RSGL_MEMCPY(&surf.data[i], color, sizeof(color));
	}
}

void RSGL_soft_createBuffer(RSGL_softRenderer* ctx, size_t size, const void* data, size_t* buffer) {
	void* rawBuffer = RSGL_MALLOC(size);

	if (data)
		RSGL_soft_updateBuffer(ctx, (size_t)rawBuffer, data, 0, size);

	if (buffer) *buffer = (size_t)rawBuffer;
}

void RSGL_soft_updateBuffer(RSGL_softRenderer* ctx, size_t buffer, const void* data, size_t start, size_t end) {
	RSGL_MEMCPY(&((u8*)buffer)[start], data, end - start);
}

void RSGL_soft_deleteBuffer(RSGL_softRenderer* ctx, size_t buffer) {
	RSGL_FREE((void*)buffer);
}

void RSGL_soft_initPtr(RSGL_softRenderer* ctx, void* proc) {
	RSGL_UNUSED(proc); RSGL_UNUSED(ctx);
}

void RSGL_soft_freePtr(RSGL_softRenderer* ctx) { RSGL_UNUSED(ctx); }

void RSGL_soft_render(RSGL_softRenderer* ctx, const RSGL_renderPass* pass) {
	size_t i, j;

	RSGL_softRenderMap map;
	map.colors =  (float*)pass->buffers->color;
	map.verts = (float*)pass->buffers->vertex;
	map.texCoords = (float*)pass->buffers->texture;
	map.elements = (u16*)pass->buffers->elements;

	RSGL_softRenderCall call;

	RSGL_vec3D points_src[3];
	RSGL_vec2D texCoords_src[3];
	RSGL_color colors_src[3];

	call.points = points_src;
	call.colors = colors_src;
	call.texPoints = texCoords_src;

	for (i = 0; i < pass->buffers->batchCount; i++) {
		map.tex = (RSGL_softTexture*)pass->buffers->batches[i].tex;
		call.tex = map.tex;

		map.matrix = RSGL_mat4_multiply(pass->matrix, pass->buffers->batches[i].matrix.m);

		/* pass->buffers->batches[i].lineWidth */
		for (j = pass->buffers->batches[i].elmStart; j < pass->buffers->batches[i].elmStart + pass->buffers->batches[i].elmCount; j += map.count) {
			map.index = j;
			switch (pass->buffers->batches[i].type) {
				case RSGL_TRIANGLES:
					map.count = 3;
					RSGL_softRenderMapElement(ctx, &map, &call);
					RSGL_softDrawTriangle(ctx, &call);
					break;
				case RSGL_POINTS:
					map.count = 1;
					RSGL_softRenderMapElement(ctx, &map, &call);
					RSGL_softDrawPoint(ctx, &call);
					break;
				case RSGL_LINES:
					map.count = 2;
					RSGL_softRenderMapElement(ctx, &map, &call);
					RSGL_softDrawLine(ctx, &call);
					break;
				default:
					map.count = 1;
					break;
			}
				}
	}
}

void RSGL_soft_scissorStart(RSGL_softRenderer* ctx, float x, float y, float w, float h, float renderer_height) {

}

void RSGL_soft_scissorEnd(RSGL_softRenderer* ctx) {

}

RSGL_texture RSGL_soft_createTexture(RSGL_softRenderer* ctx, const RSGL_textureBlob* blob) {
	RSGL_softTexture* tex = RSGL_MALLOC(sizeof(RSGL_softTexture));
	tex->width = blob->width;
	tex->height = blob->height;

	return (RSGL_texture)tex;
}

void RSGL_soft_copyToTexture(RSGL_softRenderer* ctx, RSGL_texture texture, size_t x, size_t y, const RSGL_textureBlob* blob) {

}

void RSGL_soft_deleteTexture(RSGL_softRenderer* ctx, RSGL_texture tex) {
	RSGL_FREE((RSGL_softTexture*)tex);
}

#endif /* RSGL_IMPLEMENTATION */
