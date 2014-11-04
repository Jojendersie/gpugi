#pragma once

#include <cassert>

namespace Jo {
namespace Memory {

	/**************************************************************************//**
	* \brief	A fast memory allocation unit to get objects of the same size.
	* \details	This unit provides only plain memory. There are no constructor and
	*			destructor calls in Alloc and Free (Replaces malloc/free).
	*
	*			The template variants New and Delete will call the constructor and
	*			destructor of the instance.
	*****************************************************************************/
	class PoolAllocator
	{
		void** m_pools;			///< Array of memory blocks with sizes of 256 * instance size.
		void* m_nextFree;		///< The next free instance or 0 if there is no place in the current pools.
		int m_numPools;			///< Number of large blocks in m_pPools.
		int m_instanceSize;		///< Size of the allocable instances
		int m_blockSize;		///< Size of on block in the pool
	public:
		PoolAllocator( int _instanceSize );
		~PoolAllocator();

		void* Alloc();
		void Free( void* _instance );

		/// \brief Allocate a new instance and call its standard constructor.
		/// \return A new instance.
		template<typename T> T* New()
		{
			// This poolallocator is initialized to an other size than that
			// of the requested type. Allocation not possible!
			assert(sizeof(T) == m_instanceSize);

			T* pNew = Alloc();
			return new (pNew) T;
		}

		/// \brief Calls the destructor and frees the memory of the given instance.
		/// \details If the memory was not allocated by this PoolAllocator there
		///		might be hard crashes or memory leaks later.
		template<typename T> void Delete( T* _instance )
		{
			_instance->~T();
			Free( _instance );
		}

		/// \brief Keep the allocated blocks but reset all instances to empty.
		///
		void FreeAll();
	};

} // namespace Memory
} // namespace Jo