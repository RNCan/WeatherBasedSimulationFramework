#pragma once
using namespace System;
namespace BioSIM_Wrapper {

	template<class T>
	public ref class ManagedObject
	{
	public:
		ManagedObject(T* instance)
			: m_Instance(instance)
		{
		}
		virtual ~ManagedObject()
		{
			if (m_Instance != nullptr)
			{
				delete m_Instance;
			}
		}
		!ManagedObject()
		{
			if (m_Instance != nullptr)
			{
				delete m_Instance;
			}
		}
		T* GetInstance()
		{
			return m_Instance;
		}

	protected:
		T* m_Instance;

	};
}

