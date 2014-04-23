#ifndef __IGN_TRANSPORT_SINGLETON_HH_INCLUDED__
#define __IGN_TRANSPORT_SINGLETON_HH_INCLUDED__

template< class C >
class Singleton {

public:
	static C* getInstance(bool _verbose)
	{
		if( Singleton<C>::uniqueInstance == NULL )
			Singleton<C>::uniqueInstance = new C(_verbose);

		return Singleton<C>::uniqueInstance;
	}

	static void removeInstance()
	{
		if( Singleton<C>::uniqueInstance != NULL )
		{
			delete Singleton<C>::uniqueInstance;
			Singleton<C>::uniqueInstance = NULL;
		}
	}

private:
	static C *uniqueInstance;
};

// Initialize the static member CurrentInstance
template< class C >
C* Singleton<C>::uniqueInstance = NULL;

#endif