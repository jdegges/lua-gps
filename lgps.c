#define LUA_LIB

#include <lua.h>
#include <lauxlib.h>

#include <gps.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define LUA_GPS_NAME "gps"
#define MT_GPS "gps mt"


#define print_error(fmt, ...){ \
  char err[256]; \
  int pos = snprintf (err, 256, "[%s:%d in %s] ", __FILE__, __LINE__, __func__); \
  snprintf (err+pos, 256-pos, fmt, ##__VA_ARGS__); \
  lua_pushstring (L, err); \
  lua_error (L); \
}

/*
 * server (string)
 * port (string)
 * device (string)
 */
static int
lgps_open (lua_State *L)
{
  int top = lua_gettop (L);
  struct gps_data_t *ud = NULL;
  const char *server = luaL_checkstring (L, 1);
  const char *port = luaL_checkstring (L, 2);

  ud = lua_newuserdata (L, sizeof *ud);
  if (NULL == ud)
    print_error ("out of memory");

  if (0 != gps_open (server, port, ud))
    print_error ("no gpsd running or network error: %d, %s", errno, gps_errstr (errno));

  luaL_getmetatable (L, MT_GPS);
  lua_setmetatable (L, -2);

  return 1;
}

static int
lgps_close (lua_State *L)
{
  struct gps_data_t *ud = luaL_checkudata (L, 1, MT_GPS);

  gps_close (ud);

  lua_pushboolean (L, 1);

  return 1;
}

/*
 * gps object (userdata)
 * flags (integer)
 * device (string) *temporarily disabled*
 */
static int
lgps_stream (lua_State *L)
{
  struct gps_data_t *ud = luaL_checkudata (L, 1, MT_GPS);
  lua_Integer flags = luaL_checkinteger (L, 2);
  /*const char *device = luaL_checkstring (L, 3);*/

  (void) gps_stream (ud, flags, NULL);

  return 0;
}

/*
 * gps object (userdata)
 * timeout (integer)
 * returns true if data is waiting, false if not.
 */
static int
lgps_waiting (lua_State *L)
{
  struct gps_data_t *ud = luaL_checkudata (L, 1, MT_GPS);
  lua_Integer timeout = luaL_checkinteger (L, 2);

  lua_pushboolean (L, gps_waiting (ud, timeout));

  return 1;
}

#define SET_TABLE_INT(key, value) lua_pushliteral (L, key); lua_pushinteger (L, value); lua_settable (L, -3)
#define SET_TABLE_DBL(key, value) lua_pushliteral (L, key); lua_pushnumber (L, value); lua_settable (L, -3)
#define SET_TABLE_STR(key, value) lua_pushliteral (L, key); lua_pushstring (L, value); lua_settable (L, -3)

/*
 * gps object (userdata)
 * returns a table with gps data
 */
static int
lgps_read (lua_State *L)
{
  struct gps_data_t *ud = luaL_checkudata (L, 1, MT_GPS);
  int count;
  int i;

  count = gps_read (ud);
  if (count <= 0)
    print_error ("either no data or error reading data");

  lua_createtable (L, 0, 0);

  SET_TABLE_DBL ("online", ud->online);

  lua_pushliteral (L, "fix");
  {
    lua_createtable (L, 0, 0);
    SET_TABLE_DBL ("time", ud->fix.time);
    SET_TABLE_INT ("mode", ud->fix.mode);
    SET_TABLE_DBL ("ept", ud->fix.ept);
    SET_TABLE_DBL ("latitude", ud->fix.latitude);
    SET_TABLE_DBL ("epy", ud->fix.epy);
    SET_TABLE_DBL ("longitude", ud->fix.longitude);
    SET_TABLE_DBL ("epx", ud->fix.epx);
    SET_TABLE_DBL ("altitude", ud->fix.altitude);
    SET_TABLE_DBL ("epv", ud->fix.epv);
    SET_TABLE_DBL ("track", ud->fix.track);
    SET_TABLE_DBL ("epd", ud->fix.epd);
    SET_TABLE_DBL ("speed", ud->fix.speed);
    SET_TABLE_DBL ("eps", ud->fix.eps);
    SET_TABLE_DBL ("climb", ud->fix.climb);
    SET_TABLE_DBL ("epc", ud->fix.epc);
  }
  lua_settable (L, -3);

  SET_TABLE_DBL ("separation", ud->separation);
  SET_TABLE_INT ("status", ud->status);
  SET_TABLE_INT ("satellites_used", ud->satellites_used);

  lua_pushliteral (L, "used");
  {
    lua_createtable (L, 0, 0);
    for (i = 0; i < ud->satellites_used && i < MAXCHANNELS; i++) {
      lua_pushinteger (L, i + 1);
      lua_pushinteger (L, ud->used[i]);
      lua_settable (L, -3);
    }
  }
  lua_settable (L, -3);
  
  lua_pushliteral (L, "dop");
  {
    lua_createtable (L, 0, 0);
    SET_TABLE_DBL ("xdop", ud->dop.xdop);
    SET_TABLE_DBL ("ydop", ud->dop.ydop);
    SET_TABLE_DBL ("pdop", ud->dop.pdop);
    SET_TABLE_DBL ("hdop", ud->dop.hdop);
    SET_TABLE_DBL ("vdop", ud->dop.vdop);
    SET_TABLE_DBL ("tdop", ud->dop.tdop);
    SET_TABLE_DBL ("gdop", ud->dop.gdop);
  }
  lua_settable (L, -3);

  SET_TABLE_DBL ("epe", ud->epe);

  SET_TABLE_DBL ("skyview_time", ud->skyview_time);
  SET_TABLE_INT ("satellites_visible", ud->satellites_visible);

  lua_pushliteral (L, "PRN");
  {
    lua_createtable (L, 0, 0);
    for (i = 0; i < ud->satellites_visible && i < MAXCHANNELS; i++) {
      lua_pushinteger (L, i + 1);
      lua_pushinteger (L, ud->PRN[i]);
      lua_settable (L, -3);
    }
  }
  lua_settable (L, -3);

  lua_pushliteral (L, "elevation");
  {
    lua_createtable (L, 0, 0);
    for (i = 0; i < ud->satellites_visible && i < MAXCHANNELS; i++) {
      lua_pushinteger (L, i + 1);
      lua_pushinteger (L, ud->elevation[i]);
      lua_settable (L, -3);
    }
  }
  lua_settable (L, -3);

  lua_pushliteral (L, "azimuth");
  {
    lua_createtable (L, 0, 0);
    for (i = 0; i < ud->satellites_visible && i < MAXCHANNELS; i++) {
      lua_pushinteger (L, i + 1);
      lua_pushinteger (L, ud->azimuth[i]);
      lua_settable (L, -3);
    }
  }
  lua_settable (L, -3);

  lua_pushliteral (L, "ss");
  {
    lua_createtable (L, 0, 0);
    for (i = 0; i < ud->satellites_visible && i < MAXCHANNELS; i++) {
      lua_pushinteger (L, i + 1);
      lua_pushnumber (L, ud->ss[i]);
      lua_settable (L, -3);
    }
  }
  lua_settable (L, -3);

  SET_TABLE_STR ("dev", ud->dev.path);

  return 1;
}

static const luaL_Reg gpslib[] = {
  {"open", lgps_open},
  {NULL, NULL}
};

static const luaL_Reg gps_methods[] = {
  {"__gc", lgps_close},
  {"close", lgps_close},
  {"stream", lgps_stream},
  {"waiting", lgps_waiting},
  {"read", lgps_read},
  {NULL, NULL}
};

#define SET_CONST_INT(s) lua_pushinteger (L, s); lua_setfield (L, -2, #s)
#define SET_CONST_DBL(s) lua_pushnumber (L, s); lua_setfield (L, -2, #s)

LUALIB_API int luaopen_gps (lua_State *L)
{
  luaL_register (L, LUA_GPS_NAME, gpslib);

  SET_CONST_INT (WATCH_ENABLE);
  SET_CONST_INT (WATCH_DISABLE);
  SET_CONST_INT (WATCH_JSON);
  SET_CONST_INT (WATCH_NMEA);
  SET_CONST_INT (WATCH_RARE);
  SET_CONST_INT (WATCH_RAW);
  SET_CONST_INT (WATCH_SCALED);
  SET_CONST_INT (WATCH_TIMING);
  SET_CONST_INT (WATCH_DEVICE);
  SET_CONST_INT (WATCH_NEWSTYLE);
  SET_CONST_INT (WATCH_OLDSTYLE);

  SET_CONST_DBL (METERS_TO_FEET);
  SET_CONST_DBL (METERS_TO_MILES);
  SET_CONST_DBL (KNOTS_TO_MPH);
  SET_CONST_DBL (KNOTS_TO_KPH);
  SET_CONST_DBL (KNOTS_TO_MPS);
  SET_CONST_DBL (MPS_TO_KPH);
  SET_CONST_DBL (MPS_TO_MPH);
  SET_CONST_DBL (MPS_TO_KNOTS);

  if (!luaL_newmetatable (L, MT_GPS))
    print_error ("unable to create gps metatable");
  lua_createtable (L, 0, sizeof (gps_methods) / sizeof (luaL_Reg) - 1);
  luaL_register (L, NULL, gps_methods);
  lua_setfield (L, -2, "__index");

  return 1;
}
