#ifndef SINGLETON_H
#define SINGLETON_H

// Шаблонный класс синглетона
template<typename T> class Singleton : public QObject
{
public:

	// Конструктор
	Singleton()
	{
		Q_ASSERT(mSingleton == NULL);
		mSingleton = static_cast<T *>(this);
	}

	// Деструктор
	virtual ~Singleton()
	{
		Q_ASSERT(mSingleton != NULL);
		mSingleton = NULL;
	}

	// Возвращает ссылку на глобальный экземпляр класса
	static T &getSingleton()
	{
		Q_ASSERT(mSingleton != NULL);
		return *mSingleton;
	}

	// Возвращает указатель на глобальный экземпляр класса
	static T *getSingletonPtr()
	{
		return mSingleton;
	}

	// Удаляет глобальный экземпляр класса
	static void destroy()
	{
		delete mSingleton;
		mSingleton = NULL;
	}

protected:

	static T *mSingleton;
};

#endif // SINGLETON_H
