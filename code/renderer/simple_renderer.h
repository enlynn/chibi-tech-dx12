#pragma once

struct simple_renderer_info
{
	class allocator*             HeapAllocator; // optional, if not provided, will use malloc/free internally
	// TODO: Temp Allocator?
	const class platform_window& ClientWindow;
};

void SimpleRendererInit(simple_renderer_info &RenderInfo);
void SimpleRendererDeinit();
void SimpleRendererRender();