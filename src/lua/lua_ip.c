/* Copyright (c) 2013, Vsevolod Stakhov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *       * Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 *       * Redistributions in binary form must reproduce the above copyright
 *         notice, this list of conditions and the following disclaimer in the
 *         documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "lua_common.h"

LUA_FUNCTION_DEF (ip, to_string);
LUA_FUNCTION_DEF (ip, to_table);
LUA_FUNCTION_DEF (ip, str_octets);
LUA_FUNCTION_DEF (ip, inversed_str_octets);
LUA_FUNCTION_DEF (ip, from_string);
LUA_FUNCTION_DEF (ip, destroy);

static const struct luaL_reg    iplib_m[] = {
	LUA_INTERFACE_DEF (ip, to_string),
	LUA_INTERFACE_DEF (ip, to_table),
	LUA_INTERFACE_DEF (ip, str_octets),
	LUA_INTERFACE_DEF (ip, inversed_str_octets),
	{"__tostring", lua_ip_to_string},
	{"__gc", lua_ip_destroy},
	{NULL, NULL}
};

static const struct luaL_reg    iplib_f[] = {
	LUA_INTERFACE_DEF (ip, from_string),
	{NULL, NULL}
};

static struct rspamd_lua_ip	*
lua_check_ip (lua_State * L, gint pos)
{
	void *ud = luaL_checkudata (L, pos, "rspamd{ip}");

	luaL_argcheck (L, ud != NULL, pos, "'ip' expected");
	return ud ? *((struct rspamd_lua_ip **)ud) : NULL;
}

static gint
lua_ip_to_table (lua_State *L)
{
	struct rspamd_lua_ip *ip = lua_check_ip (L, 1);
	int max, i;
	guint8 *ptr;

	if (ip != NULL) {
		lua_newtable (L);
		if (ip->af == AF_INET) {
			max = 32 / 8;
		}
		else {
			max = 128 / 8;
		}
		ptr = (guint8 *)&ip->data;
		for (i = 1; i <= max; i ++, ptr ++) {
			lua_pushnumber (L, *ptr);
			lua_rawseti (L, -2, i);
		}
	}
	else {
		lua_pushnil (L);
	}

	return 1;
}

static gint
lua_ip_str_octets (lua_State *L)
{
	struct rspamd_lua_ip *ip = lua_check_ip (L, 1);
	int max, i;
	guint8 *ptr;
	char numbuf[8];

	if (ip != NULL) {
		lua_newtable (L);
		if (ip->af == AF_INET) {
			max = 32 / 8;
		}
		else {
			max = 128 / 8;
		}
		ptr = (guint8 *)&ip->data;
		for (i = 1; i <= max; i ++, ptr ++) {
			if (ip->af == AF_INET) {
				rspamd_snprintf (numbuf, sizeof (numbuf), "%d", *ptr);
				lua_pushstring (L, numbuf);
				lua_rawseti (L, -2, i);
			}
			else {
				rspamd_snprintf (numbuf, sizeof (numbuf), "%xd", (*ptr & 0xf0) >> 4);
				lua_pushstring (L, numbuf);
				lua_rawseti (L, -2, i*2 - 1);
				rspamd_snprintf (numbuf, sizeof (numbuf), "%xd", *ptr & 0x0f);
				lua_pushstring (L, numbuf);
				lua_rawseti (L, -2, i*2);
			}
		}
	}
	else {
		lua_pushnil (L);
	}

	return 1;
}

static gint
lua_ip_inversed_str_octets (lua_State *L)
{
	struct rspamd_lua_ip *ip = lua_check_ip (L, 1);
	int max, i;
	guint8 *ptr;
	char numbuf[4];

	if (ip != NULL) {
		lua_newtable (L);
		if (ip->af == AF_INET) {
			max = 32 / 8;
		}
		else {
			max = 128 / 8;
		}
		ptr = (guint8 *)&ip->data;
		ptr += max - 1;
		for (i = 1; i <= max; i ++, ptr --) {
			if (ip->af == AF_INET) {
				rspamd_snprintf (numbuf, sizeof (numbuf), "%d", *ptr);
				lua_pushstring (L, numbuf);
				lua_rawseti (L, -2, i);
			}
			else {
				rspamd_snprintf (numbuf, sizeof (numbuf), "%xd", *ptr & 0x0f);
				lua_pushstring (L, numbuf);
				lua_rawseti (L, -2, i*2 - 1);
				rspamd_snprintf (numbuf, sizeof (numbuf), "%xd", (*ptr & 0xf0) >> 4);
				lua_pushstring (L, numbuf);
				lua_rawseti (L, -2, i*2);
			}
		}
	}
	else {
		lua_pushnil (L);
	}

	return 1;
}

static gint
lua_ip_to_string (lua_State *L)
{
	struct rspamd_lua_ip *ip = lua_check_ip (L, 1);
	gchar dst[INET6_ADDRSTRLEN + 1];

	if (ip != NULL) {
		lua_pushstring (L, inet_ntop (ip->af, &ip->data, dst, sizeof (dst)));
	}
	else {
		lua_pushnil (L);
	}

	return 1;
}

static gint
lua_ip_from_string (lua_State *L)
{
	struct rspamd_lua_ip *ip, **pip;
	const gchar *ip_str;

	ip_str = luaL_checkstring (L, 1);
	if (ip_str) {
		ip = g_slice_alloc (sizeof (struct rspamd_lua_ip));
		if (inet_pton (AF_INET, ip_str, &ip->data.ip4) == 1) {
			ip->af = AF_INET;
		}
		else if (inet_pton (AF_INET6, ip_str, &ip->data.ip6) == 1) {
			ip->af = AF_INET6;
		}
		else {
			g_slice_free1 (sizeof (struct rspamd_lua_ip), ip);
			lua_pushnil (L);
			return 1;
		}
		pip = lua_newuserdata (L, sizeof (struct rspamd_lua_ip *));
		lua_setclass (L, "rspamd{ip}", -1);
		*pip = ip;
	}
	else {
		lua_pushnil (L);
	}

	return 1;
}

static gint
lua_ip_destroy (lua_State *L)
{
	struct rspamd_lua_ip *ip = lua_check_ip (L, 1);

	if (ip) {
		g_slice_free1 (sizeof (struct rspamd_lua_ip), ip);
	}

	return 0;
}

void
lua_ip_push (lua_State *L, int af, gpointer data)
{
	struct rspamd_lua_ip *ip, **pip;

	ip = g_slice_alloc (sizeof (struct rspamd_lua_ip));

	ip->af = af;
	if (af == AF_INET6) {
		memcpy (&ip->data, data, sizeof (struct in6_addr));
	}
	else {
		memcpy (&ip->data, data, sizeof (struct in_addr));
	}
	pip = lua_newuserdata (L, sizeof (struct rspamd_lua_ip *));
	lua_setclass (L, "rspamd{ip}", -1);
	*pip = ip;
}

void
lua_ip_push_fromstring (lua_State *L, const gchar *ip_str)
{
	struct rspamd_lua_ip *ip, **pip;

	if (ip_str == NULL) {
		lua_pushnil (L);
	}
	else {
		ip = g_slice_alloc (sizeof (struct rspamd_lua_ip));
		if (inet_pton (AF_INET, ip_str, &ip->data.ip4) == 1) {
			ip->af = AF_INET;
		}
		else if (inet_pton (AF_INET6, ip_str, &ip->data.ip6) == 1) {
			ip->af = AF_INET6;
		}
		else {
			g_slice_free1 (sizeof (struct rspamd_lua_ip), ip);
			lua_pushnil (L);
			return;
		}

		pip = lua_newuserdata (L, sizeof (struct rspamd_lua_ip *));
		lua_setclass (L, "rspamd{ip}", -1);
		*pip = ip;
	}
}

gint
luaopen_ip (lua_State * L)
{
	luaL_newmetatable (L, "rspamd{ip}");
	lua_pushstring (L, "__index");
	lua_pushvalue (L, -2);
	lua_settable (L, -3);

	lua_pushstring (L, "class");
	lua_pushstring (L, "rspamd{ip}");
	lua_rawset (L, -3);

	luaL_register (L, NULL, iplib_m);
	luaL_register (L, "rspamd_ip", iplib_f);

	lua_pop (L, 1);                      /* remove metatable from stack */

	return 1;
}
