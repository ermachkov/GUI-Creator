#ifndef LUA_SCRIPT_H
#define LUA_SCRIPT_H

// Класс Lua-скрипта
class LuaScript
{
public:

	// Конструктор
	LuaScript();

	// Деструктор
	~LuaScript();

	// Загружает Lua-скрипт
	bool load(const QString &fileName);

	// Возвращает строковое значение по имени
	bool getString(const QString &name, QString &value) const;

	// Возвращает целочисленное значение по имени
	bool getInt(const QString &name, int &value) const;

	// Возвращает целочисленное значение по индексу
	bool getInt(int index, int &value) const;

	// Возвращает вещественное значение по имени
	bool getReal(const QString &name, qreal &value) const;

	// Возвращает булевское значение по имени
	bool getBool(const QString &name, bool &value) const;

	// Возвращает длину текущей таблицы
	int getLength() const;

	// Помещает таблицу в стек по имени
	bool pushTable(const QString &name);

	// Помещает таблицу в стек по индексу
	bool pushTable(int index);

	// Извлекает таблицу из стека
	void popTable();

private:

	lua_State   *mLuaState;     // Состояние Lua
	int         mTableIndex;    // Индекс текущей таблицы в стеке
};

#endif // LUA_SCRIPT_H
