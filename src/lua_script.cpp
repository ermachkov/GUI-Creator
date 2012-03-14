#include "pch.h"
#include "lua_script.h"

LuaScript::LuaScript()
: mLuaState(NULL), mTableIndex(0)
{
}

LuaScript::~LuaScript()
{
	if (mLuaState != NULL)
		lua_close(mLuaState);
}

bool LuaScript::load(const QString &fileName)
{
	// открываем файл скрипта
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
		return false;

	// считываем его содержимое
	QByteArray buf = file.readAll();
	if (buf.isEmpty())
		return false;

	// создаем и инициализируем новое состояние Lua
	mLuaState = luaL_newstate();
	if (mLuaState == NULL)
		return false;
	luaL_openlibs(mLuaState);

	// загружаем и выполняем скрипт
	if (luaL_loadbuffer(mLuaState, buf.data(), buf.size(), fileName.toStdString().c_str()) != 0 || lua_pcall(mLuaState, 0, 0, 0) != 0)
	{
		qDebug() << lua_tostring(mLuaState, -1);
		lua_pop(mLuaState, 1);
		return false;
	}

	return true;
}

bool LuaScript::getString(const QString &name, QString &value) const
{
	// читаем значение строковой переменной
	lua_getfield(mLuaState, mTableIndex == 0 ? LUA_GLOBALSINDEX : -1, name.toStdString().c_str());
	if (lua_isstring(mLuaState, -1) != 0)
	{
		value = lua_tostring(mLuaState, -1);
		lua_pop(mLuaState, 1);
		return true;
	}

	// удаляем неверный элемент из стека
	lua_pop(mLuaState, 1);
	return false;
}

bool LuaScript::getInt(const QString &name, int &value) const
{
	// читаем значение целочисленной переменной
	lua_getfield(mLuaState, mTableIndex == 0 ? LUA_GLOBALSINDEX : -1, name.toStdString().c_str());
	if (lua_isnumber(mLuaState, -1) != 0)
	{
		value = lua_tointeger(mLuaState, -1);
		lua_pop(mLuaState, 1);
		return true;
	}

	// удаляем неверный элемент из стека
	lua_pop(mLuaState, 1);
	return false;
}

bool LuaScript::getInt(int index, int &value) const
{
	// читаем значение целочисленной переменной
	lua_pushinteger(mLuaState, index);
	lua_gettable(mLuaState, mTableIndex == 0 ? LUA_GLOBALSINDEX : -2);
	if (lua_isnumber(mLuaState, -1) != 0)
	{
		value = lua_tointeger(mLuaState, -1);
		lua_pop(mLuaState, 1);
		return true;
	}

	// удаляем неверный элемент из стека
	lua_pop(mLuaState, 1);
	return false;
}

bool LuaScript::getReal(const QString &name, qreal &value) const
{
	// читаем значение вещественной переменной
	lua_getfield(mLuaState, mTableIndex == 0 ? LUA_GLOBALSINDEX : -1, name.toStdString().c_str());
	if (lua_isnumber(mLuaState, -1) != 0)
	{
		value = lua_tonumber(mLuaState, -1);
		lua_pop(mLuaState, 1);
		return true;
	}

	// удаляем неверный элемент из стека
	lua_pop(mLuaState, 1);
	return false;
}

bool LuaScript::getReal(int index, qreal &value) const
{
	// читаем значение вещественной переменной
	lua_pushinteger(mLuaState, index);
	lua_gettable(mLuaState, mTableIndex == 0 ? LUA_GLOBALSINDEX : -2);
	if (lua_isnumber(mLuaState, -1) != 0)
	{
		value = lua_tonumber(mLuaState, -1);
		lua_pop(mLuaState, 1);
		return true;
	}

	// удаляем неверный элемент из стека
	lua_pop(mLuaState, 1);
	return false;
}

bool LuaScript::getBool(const QString &name, bool &value) const
{
	// читаем значение булевской переменной
	lua_getfield(mLuaState, mTableIndex == 0 ? LUA_GLOBALSINDEX : -1, name.toStdString().c_str());
	if (lua_isboolean(mLuaState, -1) != 0)
	{
		value = lua_toboolean(mLuaState, -1);
		lua_pop(mLuaState, 1);
		return true;
	}

	// удаляем неверный элемент из стека
	lua_pop(mLuaState, 1);
	return false;
}

int LuaScript::getLength() const
{
	return lua_objlen(mLuaState, mTableIndex == 0 ? LUA_GLOBALSINDEX : -1);
}

bool LuaScript::pushTable(const QString &name)
{
	// помещаем таблицу в стек
	lua_getfield(mLuaState, mTableIndex == 0 ? LUA_GLOBALSINDEX : -1, name.toStdString().c_str());
	if (lua_istable(mLuaState, -1) != 0)
	{
		++mTableIndex;
		return true;
	}

	// удаляем неверный элемент из стека
	lua_pop(mLuaState, 1);
	return false;
}

bool LuaScript::pushTable(int index)
{
	// помещаем таблицу в стек
	lua_pushinteger(mLuaState, index);
	lua_gettable(mLuaState, mTableIndex == 0 ? LUA_GLOBALSINDEX : -2);
	if (lua_istable(mLuaState, -1) != 0)
	{
		++mTableIndex;
		return true;
	}

	// удаляем неверный элемент из стека
	lua_pop(mLuaState, 1);
	return false;
}

void LuaScript::popTable()
{
	// извлекаем таблицу из стека и уменьшаем индекс текущей таблицы
	lua_pop(mLuaState, 1);
	--mTableIndex;
}
