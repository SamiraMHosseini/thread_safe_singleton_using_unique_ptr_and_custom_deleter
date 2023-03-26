#include <conio.h>
#include <thread>
#include <future>
#include <chrono>

struct ThreadCountProxy
{
	typedef std::unique_ptr<ThreadCountProxy, void(*)(ThreadCountProxy*)> smartpoiner;
public:


	static void Increment() 
	{
		smartpoiner& pInstance = ThreadCountProxy::privGetInstance();
		std::lock_guard<std::mutex> lock(pInstance->mtx);
		++pInstance->count;
		Debug::out("TC:%d \n", pInstance->count);
	}


	static void Decrement() 
	{
		smartpoiner& pInstance = ThreadCountProxy::privGetInstance();
		std::lock_guard<std::mutex> lock(pInstance->mtx);
		--pInstance->count;
		Debug::out("TC:%d \n", pInstance->count);
		if (pInstance->count == 0)
		{
			pInstance->cv_thread_count.notify_one();
		}

	}
	static void WaitUntilThreadsDone()  
	{
		smartpoiner& pInstance = ThreadCountProxy::privGetInstance();
		std::unique_lock<std::mutex> lock_ct(pInstance->mtx_thread_count);
		pInstance->cv_thread_count.wait(lock_ct);
	}
	
private:
	
	~ThreadCountProxy() = default;
	ThreadCountProxy() :
		count(0), mtx(), mtx_thread_count(), cv_thread_count()
	{

		std::cout << "Default Constructor\n"; //This line shows how many instances of class ThreadCountProxy are being created! 

	}
	static void CleanUp(ThreadCountProxy* pObject)
	{
		
	    delete pObject;
	}
private:
	
	static void CallOnce(smartpoiner& pInstance)
	{
		
		if (pInstance == nullptr)
		{
			pInstance = smartpoiner(new ThreadCountProxy(), &CleanUp);
		}
	}

	static std::unique_ptr<ThreadCountProxy, void(*)(ThreadCountProxy*)>& privGetInstance()
	{
		std::this_thread::sleep_for(3s); //Make it very competitive
		static std::once_flag flag;
		static smartpoiner  pInstance(nullptr, &CleanUp);
		std::call_once(flag, CallOnce, pInstance);
		if (pInstance == nullptr)
		{
			std::cout << "It is nullptr\n"; //It is used to test whether the Singleton Pattern is thread-safe and works properly or not
		}
		return pInstance;
	}

	//Shared Data
	int count;
	std::mutex mtx; //This mutex is used for the 'count' variable
	std::mutex				 mtx_thread_count; //This mutex is used for the signal
	std::condition_variable  cv_thread_count;
};

class ThreadCount
{
public:
	ThreadCount()
	{
		ThreadCountProxy::Increment();
	}

	~ThreadCount()
	{
		ThreadCountProxy::Decrement();

	}
};


class A : public BannerBase
{
public:
	A() = delete;
	A(const char* const pName) : BannerBase(pName) {}

	void operator()(std::future<void>& tFuture)
	{
		START_BANNER;

		ThreadCount TC;

		int count = 0;
		while (true)
		{
			Debug::out("%d\n", count);
			count++;


			std::future_status status = tFuture.wait_for(1ms);
			if (status == std::future_status::ready)
			{
				// can't read this multiple times
				//tFuture.get();
				break;
			}
		}
		//std::this_thread::sleep_for(2s); //Just for testing purposes
	}
};

class B : public BannerBase
{
public:
	B() = delete;
	B(const char* const pName) : BannerBase(pName) {}

	void operator()(std::future<void>& tFuture)
	{
		START_BANNER

			ThreadCount TC;

		int count = 0x10000;
		while (true)
		{
			Debug::out("0x%x\n", count);
			count--;

			std::future_status status = tFuture.wait_for(1ms);
			if (status == std::future_status::ready)
			{
				// can't read this multiple times
				//tFuture.get();
				break;
			}
		}
	}
};


class C : public BannerBase
{
public:
	C() = delete;
	C(const char* const pName) : BannerBase(pName) {}

	void operator()(std::future<void>& tFuture)
	{
		START_BANNER

			ThreadCount TC;

		const char* pFruit[] =
		{
			{"apple"},
			{"orange"},
			{"banana"},
			{"lemon"}
		};

		int count = 0;
		while (true)
		{
			int i = count++ % 4;
			Debug::out("%s\n", pFruit[i]);


			std::future_status status =
				//tFuture.wait_until(std::chrono::system_clock::now());
				tFuture.wait_for(500ms);
			if (status == std::future_status::ready)
			{
				break;
			}
		}
	}
};

class D : public BannerBase
{
public:
	D() = delete;
	D(const char* const pName) : BannerBase(pName) {}

	void operator()(std::future<void>& tFuture)
	{
		START_BANNER

			ThreadCount TC;

		const char* pStoryOriginal = "<0><1><2><3><4><5><6><7><8>";
		size_t  len = strlen(pStoryOriginal);
		char* pString = new char[len + 1];
		strcpy_s(pString, len + 1, pStoryOriginal);

		int tmpLen = (int)(len - 1);

		while (true)
		{
			if (tmpLen <= 0)
			{
				tmpLen = (int)(len - 1);
				strcpy_s(pString, len + 1, pStoryOriginal);
			}

			Debug::out("%s\n", pString);
			pString[tmpLen--] = 0;
			pString[tmpLen--] = 0;
			pString[tmpLen--] = 0;

			std::future_status status = tFuture.wait_for(1ms);
			if (status == std::future_status::ready)
			{
				// can't read this multiple times
				//tFuture.get();
				break;
			}
		}

		delete pString;
	}
};

class Controller : public BannerBase
{


public:
	Controller() = delete;
	Controller(const char* const pName) : BannerBase(pName) //, keyPress()
	{}

	void operator()(std::promise<void>&& tPromise)
	{
		START_BANNER;

		// kill them now
		tPromise.set_value();

		// wait until all are done
		ThreadCountProxy::WaitUntilThreadsDone();

	}


};


int main()
{


	START_BANNER_MAIN("main");

	// start threads
	A oA("A");
	B oB("B");
	C oC("C");
	D oD("D");
	Controller oController("Controller");

	// Spawn threads 
	std::promise<void> tPromise;
	std::future<void> tFuture = tPromise.get_future();

	std::thread tA(oA, std::ref(tFuture));
	tA.detach();

	std::thread tB(oB, std::ref(tFuture));
	tB.detach();

	std::thread tC(oC, std::ref(tFuture));
	tC.detach();

	std::thread tD(oD, std::ref(tFuture));
	tD.detach();

	//Version 1
	std::future<void> fuController = async(std::launch::deferred,
		std::ref(oController),
		std::move(tPromise));

	//OR
	//Version 2 
	/*std::future<void> fuController = async(std::launch::deferred,
		std::ref(oController),
		std::ref(tPromise)); */

		// Key press
	_getch();
	Debug::out("key pressed <-----\n");
	// Controller are you done?
	fuController.get();

}

// ---  End of File ---
