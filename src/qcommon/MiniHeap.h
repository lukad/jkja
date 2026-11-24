#if !defined(MINIHEAP_H_INC)
#define MINIHEAP_H_INC

#include <cstddef>
#include <cstdint>

class CMiniHeap
{
	char	*mHeap;
	char	*mCurrentHeap;
	int		mSize;
#if _DEBUG
	int		mMaxAlloc;
#endif
public:

// reset the heap back to the start
void ResetHeap()
{
#if _DEBUG
	if ((int)(mCurrentHeap - mHeap)>mMaxAlloc)
	{
		mMaxAlloc=(int)(mCurrentHeap - mHeap);
	}
#endif
	mCurrentHeap = mHeap;
}

// initialise the heap
CMiniHeap(int size)
{
	mHeap = (char *)Z_Malloc(size, TAG_GHOUL2, qtrue);
	mSize = size;
#if _DEBUG
	mMaxAlloc=0;
#endif
	if (mHeap)
	{
		ResetHeap();
	}
}

// free up the heap
~CMiniHeap()
{
	if (mHeap)
	{
		// the quake heap will be long gone, no need to free it Z_Free(mHeap);
	}
}

// give me some space from the heap please
//
// The space is aligned for any type. The release handed out the next free byte, which
// was enough while its callers kept 4-byte values there; their arrays of size_t need
// 8 bytes on 64-bit.
char *MiniHeapAlloc(int size)
{
	const std::uintptr_t misalignment = reinterpret_cast<std::uintptr_t>(mCurrentHeap) % alignof(std::max_align_t);
	char *start = mCurrentHeap + (misalignment ? alignof(std::max_align_t) - misalignment : 0);
	if (size < (mSize - (int)(start - mHeap)))
	{
		mCurrentHeap = start + size;
		return start;
	}
	return NULL;
}

};

extern CMiniHeap *G2VertSpaceServer;


#endif	//MINIHEAP_H_INC
