#pragma once

#include "gfx_command_list.h"
#include <util/allocator.h> //allocator
#include <util/array.h>     //farray<gfx_cmd_list>

class gfx_device;
class gfx_command_list;

struct ID3D12CommandQueue;
struct ID3D12Fence;


enum class gfx_command_queue_type : u8
{
	none,     //default init state, not valid to use
	graphics,
	compute,
	copy
};

enum class gfx_fence_result
{
	success,
};

class gfx_command_queue
{
public:
	gfx_command_queue() = default;
	gfx_command_queue(const allocator& Allocator, gfx_command_queue_type Type, gfx_device* Device);
	~gfx_command_queue() { Deinit(); }

	gfx_command_queue(const gfx_command_queue& Other) = delete;
	gfx_command_queue(gfx_command_queue&& Other);

	gfx_command_queue& operator=(const gfx_command_queue& Other) = delete;
	gfx_command_queue& operator=(gfx_command_queue&& Other);

	inline ID3D12CommandQueue* AsHandle() const { return mQueueHandle; }

	void              Deinit();

	void              Flush(); // flushes the command list so that no work is being done

	// Command Lists
	gfx_command_list* GetCommandList(gfx_command_list_type Type = gfx_command_list_type::graphics);
	u64               ExecuteCommandLists(farray<gfx_command_list*> CommandLists);
	void              ProcessCommandLists();                                                        // Call once per frame, checks if in-flight commands lists are completed

	// Synchronization
	u64               Signal();                                  // (nonblocking) signals the fence and returns its current value
	bool              IsFenceComplete(u64 FenceValue);           // (nonblocking) check the currnt value of a fence
	gfx_fence_result  WaitForFence(u64 FenceValue);              // (blocking)    wait for a fence to have passed value
	void              Wait(const gfx_command_queue* OtherQueue); // (blocking)    wait for another command queue to finish executing

private:
	allocator                 mAllocator    = {};
	gfx_device*               mDevice       = nullptr;
	gfx_command_queue_type    mType         = gfx_command_queue_type::none;

	ID3D12CommandQueue*       mQueueHandle   = nullptr;

	// Synchronization
	ID3D12Fence*              mQueueFence   = nullptr;
	u64                       mFenceValue   = 0;

	// TODO:
	// - Determine if creating command lists on the fly is cheap
	// - Determine if command lists can be used after submitting
	// - Determine resource ownsership. Previously command lists were used as sudo-GC.
	struct in_flight_list
	{
		gfx_command_list* CmdList;
		u64               FenceValue;
	};

	darray<in_flight_list>    mInFlightCommandLists                                           = {};
	darray<gfx_command_list*> mAvailableFlightCommandLists[u32(gfx_command_list_type::count)] = {}; // slot for each array type
};