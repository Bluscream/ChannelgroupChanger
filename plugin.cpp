/*
 * NameFaker plugin for Teamspeak 3
 *
 * Developed by Bluscream
 */

#ifdef _WIN32
#pragma warning (disable : 4100)  /* Disable unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <time.h>
#include <cctype>
#include <iomanip>
#include <sstream>
#include "public_errors.h"
#include "public_errors_rare.h"
#include "public_definitions.h"
#include "public_rare_definitions.h"
#include "ts3_functions.h"
#include "plugin.h"

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 20

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

#define GOMME

#define PLUGIN_NAME "Set Channelgroup"
#define PLUGIN_AUTHOR "Bluscream"
#define PLUGIN_VERSION "1"
#define PLUGIN_CONTACT "www.r4p3.net"


static char* pluginID = NULL;

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if(WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif

using namespace std;

string url_encode(const string &value) {
	ostringstream escaped;
	escaped.fill('0');
	escaped << hex;

	for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
		string::value_type c = (*i);

		// Keep alphanumeric and other accepted characters intact
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
			escaped << c;
			continue;
		}

		// Any other characters are percent-encoded
		escaped << '%' << setw(2) << int((unsigned char)c);
	}

	return escaped.str();
}


/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if(!result) {
		const wchar_t* name = TEXT(PLUGIN_NAME);
		if(wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = PLUGIN_NAME;  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return PLUGIN_NAME;
#endif
}

/* Plugin version */
const char* ts3plugin_version() {
    return PLUGIN_VERSION;
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return PLUGIN_AUTHOR;
}

/* Plugin description */
const char* ts3plugin_description() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
#ifdef MTG
    return PLUGIN_NAME "(ts3.mtg-esport.de)\n\n"
		"Purpose:\n"
		"Click on a channel to set as target channel.\n"
		"Then click on a client to set his channel group.\n\n"
		"Developed by " PLUGIN_AUTHOR " (" PLUGIN_CONTACT ")\n";
#endif
#ifdef GOMME
	return PLUGIN_NAME "(GommeHD.net)\n\n"
		"Purpose:\n"
		"Click on a channel to set as target channel.\n"
		"Then click on a client to set his channel group.\n\n"
		"Developed by " PLUGIN_AUTHOR " (" PLUGIN_CONTACT ")\n";
#endif
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init() {
    char appPath[PATH_BUFSIZE];
    char resourcesPath[PATH_BUFSIZE];
    char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

    /* Your plugin init code here */
    printf("PLUGIN: init\n");

    /* Example on how to query application, resources and configuration paths from client */
    /* Note: Console client returns empty string for app and resources path */
    ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
    ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
    ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE);

	printf("PLUGIN: App path: %s\nResources path: %s\nConfig path: %s\nPlugin path: %s\n", appPath, resourcesPath, configPath, pluginPath);

    return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
	/* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
	 * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
	 * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
    /* Your plugin cleanup code here */
    printf("PLUGIN: shutdown\n");

	/*
	 * Note:
	 * If your plugin implements a settings dialog, it must be closed and deleted here, else the
	 * TeamSpeak client will most likely crash (DLL removed but dialog from DLL code still open).
	 */

	/* Free pluginID if we registered it */
	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

/****************************** Optional functions ********************************/
/*
 * Following functions are optional, if not needed you don't need to implement them.
 */

/* Tell client if plugin offers a configuration window. If this function is not implemented, it's an assumed "does not offer" (PLUGIN_OFFERS_NO_CONFIGURE). */
int ts3plugin_offersConfigure() {
	printf("PLUGIN: offersConfigure\n");
	/*
	 * Return values:
	 * PLUGIN_OFFERS_NO_CONFIGURE         - Plugin does not implement ts3plugin_configure
	 * PLUGIN_OFFERS_CONFIGURE_NEW_THREAD - Plugin does implement ts3plugin_configure and requests to run this function in an own thread
	 * PLUGIN_OFFERS_CONFIGURE_QT_THREAD  - Plugin does implement ts3plugin_configure and requests to run this function in the Qt GUI thread
	 */
	return PLUGIN_OFFERS_NO_CONFIGURE;  /* In this case ts3plugin_configure does not need to be implemented */
}

/* Plugin might offer a configuration window. If ts3plugin_offersConfigure returns 0, this function does not need to be implemented. */
void ts3plugin_configure(void* handle, void* qParentWidget) {
    printf("PLUGIN: configure\n");
}

/*
 * If the plugin wants to use error return codes, plugin commands, hotkeys or menu items, it needs to register a command ID. This function will be
 * automatically called after the plugin was initialized. This function is optional. If you don't use these features, this function can be omitted.
 * Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
 */
void ts3plugin_registerPluginID(const char* id) {
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
	printf("PLUGIN: registerPluginID: %s\n", pluginID);
}

/* Plugin command keyword. Return NULL or "" if not used. */
const char* ts3plugin_commandKeyword() {
	return NULL;
}
/*
 * Implement the following three functions when the plugin should display a line in the server/channel/client info.
 * If any of ts3plugin_infoTitle, ts3plugin_infoData or ts3plugin_freeMemory is missing, the info text will not be displayed.
 */

/* Static title shown in the left column in the info frame */
//const char* ts3plugin_infoTitle() {
//	return "Poke Blocker";
//}

/*
 * Dynamic content shown in the right column in the info frame. Memory for the data string needs to be allocated in this
 * function. The client will call ts3plugin_freeMemory once done with the string to release the allocated memory again.
 * Check the parameter "type" if you want to implement this feature only for specific item types. Set the parameter
 * "data" to NULL to have the client ignore the info data.
 */
//char* status;
//void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data) {
//	if (g_bBlockPokes) {
//		status = "ON";
//	}
//	else {
//		status = "OFF";
//	}
//	*data = (char*)malloc(INFODATA_BUFSIZE * sizeof(char));
//	snprintf(*data, INFODATA_BUFSIZE, "%s", status);
//	ts3Functions.freeMemory(status);
//}
//
/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData and ts3plugin_initMenus */
void ts3plugin_freeMemory(void* data) {
	free(data);
}

/*
 * Plugin requests to be always automatically loaded by the TeamSpeak 3 client unless
 * the user manually disabled it in the plugin dialog.
 * This function is optional. If missing, no autoload is assumed.
 */
int ts3plugin_requestAutoload() {
	return 1;  /* 1 = request autoloaded, 0 = do not request autoload */
}

/* Helper function to create a menu item */
static struct PluginMenuItem* createMenuItem(enum PluginMenuType type, int id, const char* text, const char* icon) {
	struct PluginMenuItem* menuItem = (struct PluginMenuItem*)malloc(sizeof(struct PluginMenuItem));
	menuItem->type = type;
	menuItem->id = id;
	_strcpy(menuItem->text, PLUGIN_MENU_BUFSZ, text);
	_strcpy(menuItem->icon, PLUGIN_MENU_BUFSZ, icon);
	return menuItem;
}

/* Some makros to make the code to create menu items a bit more readable */
#define BEGIN_CREATE_MENUS(x) const size_t sz = x + 1; size_t n = 0; *menuItems = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
#define CREATE_MENU_ITEM(a, b, c, d) (*menuItems)[n++] = createMenuItem(a, b, c, d);
#define END_CREATE_MENUS (*menuItems)[n++] = NULL; assert(n == sz);

/*
 * Menu IDs for this plugin. Pass these IDs when creating a menuitem to the TS3 client. When the menu item is triggered,
 * ts3plugin_onMenuItemEvent will be called passing the menu ID of the triggered menu item.
 * These IDs are freely choosable by the plugin author. It's not really needed to use an enum, it just looks prettier.
 */
enum {
	MENU_ID_CLIENT_1,
	MENU_ID_CLIENT_2,
	MENU_ID_CLIENT_3,
	MENU_ID_CLIENT_4,
	MENU_ID_CLIENT_5,
#ifdef MTG
	MENU_ID_CLIENT_6,
#endif
	MENU_ID_CHANNEL_1
};

/*
 * Initialize plugin menus.
 * This function is called after ts3plugin_init and ts3plugin_registerPluginID. A pluginID is required for plugin menus to work.
 * Both ts3plugin_registerPluginID and ts3plugin_freeMemory must be implemented to use menus.
 * If plugin menus are not used by a plugin, do not implement this function or return NULL.
 */
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
	/*
	 * Create the menus
	 * There are three types of menu items:
	 * - PLUGIN_MENU_TYPE_CLIENT:  Client context menu
	 * - PLUGIN_MENU_TYPE_CHANNEL: Channel context menu
	 * - PLUGIN_MENU_TYPE_GLOBAL:  "Plugins" menu in menu bar of main window
	 *
	 * Menu IDs are used to identify the menu item when ts3plugin_onMenuItemEvent is called
	 *
	 * The menu text is required, max length is 128 characters
	 *
	 * The icon is optional, max length is 128 characters. When not using icons, just pass an empty string.
	 * Icons are loaded from a subdirectory in the TeamSpeak client plugins folder. The subdirectory must be named like the
	 * plugin filename, without dll/so/dylib suffix
	 * e.g. for "test_plugin.dll", icon "1.png" is loaded from <TeamSpeak 3 Client install dir>\plugins\test_plugin\1.png
	 */

	/*
	 * Specify an optional icon for the plugin. This icon is used for the plugins submenu within context and main menus
	 * If unused, set menuIcon to NULL
	 */
	*menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
#ifdef GOMME
	_strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "gomme_main.png");
#endif
#ifdef MTG
	_strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "mtg_main.png");
#endif

	//Add menus
#ifdef GOMME
	BEGIN_CREATE_MENUS(6);  /* IMPORTANT: Number of menu items must be correct! */
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_1, "Channel Admin", "admin.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_2, "Operator", "operator.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_3, "Spam", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_4, "Guest", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_5, "Channel Bann", "gomme_ban.png");
#endif
#ifdef MTG
	BEGIN_CREATE_MENUS(7);  /* IMPORTANT: Number of menu items must be correct! */
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_1, "PChannel Admin", "admin.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_2, "Channel Admin", "admin.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_3, "Operator", "operator.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_4, "Guest", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_5, "not-welcome", "mtg_ban.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_6, "not-write/no-tp", "");
#endif
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_1, "Set as target Channel", "target.png");
	END_CREATE_MENUS;  /* Includes an assert checking if the number of menu items matched */
}

//void uint64_to_string(uint64 value, std::string& result) {
//	result.clear();
//	result.reserve(20); // max. 20 digits possible
//	uint64 q = value;
//	do {
//		result += "0123456789"[q % 10];
//		q /= 10;
//	} while (q);
//	std::reverse(result.begin(), result.end());
//}

template <typename T>
std::string tostring(const T& t)
{
	std::ostringstream ss;
	ss << t;
	return ss.str();
}

uint64 cid;
uint64 targetChannel[1];
uint64 channelGroup[1];
void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID) {
	if (type == PLUGIN_MENU_TYPE_CLIENT) {
		uint64 clientDBID[1];
		switch (menuItemID) {
#ifdef GOMME
		case MENU_ID_CLIENT_1: {
			channelGroup[0] = { 9 };
			uint64 dbid = 0;
			ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, selectedItemID, CLIENT_DATABASE_ID, &dbid);
			clientDBID[0] = dbid;
			ts3Functions.requestSetClientChannelGroup(serverConnectionHandlerID, channelGroup, targetChannel, clientDBID, 1, NULL);
			ts3Functions.printMessageToCurrentTab("Selected Client is now Admin of your target Channel.");
			break;
		}
		case MENU_ID_CLIENT_2: {
			channelGroup[0] = { 10 };
			uint64 dbid = 0;
			ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, selectedItemID, CLIENT_DATABASE_ID, &dbid);
			clientDBID[0] = dbid;
			ts3Functions.requestSetClientChannelGroup(serverConnectionHandlerID, channelGroup, targetChannel, clientDBID, 1, NULL);
			ts3Functions.printMessageToCurrentTab("Selected Client is now Operator of your target Channel.");
			break;
		}
		case MENU_ID_CLIENT_3: {
			channelGroup[0] = { 13 };
			uint64 dbid = 0;
			ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, selectedItemID, CLIENT_DATABASE_ID, &dbid);
			clientDBID[0] = dbid;
			ts3Functions.requestSetClientChannelGroup(serverConnectionHandlerID, channelGroup, targetChannel, clientDBID, 1, NULL);
			ts3Functions.printMessageToCurrentTab("Selected Client has now the group \"Spam\" in your target Channel.");
			break;
		}
		case MENU_ID_CLIENT_4: {
			channelGroup[0] = { 11 };
			uint64 dbid = 0;
			ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, selectedItemID, CLIENT_DATABASE_ID, &dbid);
			clientDBID[0] = dbid;
			ts3Functions.requestSetClientChannelGroup(serverConnectionHandlerID, channelGroup, targetChannel, clientDBID, 1, NULL);
			ts3Functions.printMessageToCurrentTab("Selected Client is now Guest in your target Channel.");
			break;
		}
		case MENU_ID_CLIENT_5: {
			channelGroup[0] = { 12 };
			uint64 dbid = 0;
			ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, selectedItemID, CLIENT_DATABASE_ID, &dbid);
			clientDBID[0] = dbid;
			ts3Functions.requestSetClientChannelGroup(serverConnectionHandlerID, channelGroup, targetChannel, clientDBID, 1, NULL);
			ts3Functions.printMessageToCurrentTab("Selected Client is now banned from your target Channel.");
			break;
		}
#endif
#ifdef MTG
			case MENU_ID_CLIENT_1: {
				channelGroup[0] = { 9 };
				uint64 dbid = 0;
				ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, selectedItemID, CLIENT_DATABASE_ID, &dbid);
				clientDBID[0] = dbid;
				ts3Functions.requestSetClientChannelGroup(serverConnectionHandlerID, channelGroup, targetChannel, clientDBID, 1, NULL);
				ts3Functions.printMessageToCurrentTab("Selected Client is now Permanent Admin of your target Channel.");
				break;
			}
			case MENU_ID_CLIENT_2: {
				channelGroup[0] = { 5 };
				uint64 dbid = 0;
				ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, selectedItemID, CLIENT_DATABASE_ID, &dbid);
				clientDBID[0] = dbid;
				ts3Functions.requestSetClientChannelGroup(serverConnectionHandlerID, channelGroup, targetChannel, clientDBID, 1, NULL);
				ts3Functions.printMessageToCurrentTab("Selected Client is now Admin of your target Channel.");
				break;
			}
			case MENU_ID_CLIENT_3: {
				channelGroup[0] = { 6 };
				uint64 dbid = 0;
				ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, selectedItemID, CLIENT_DATABASE_ID, &dbid);
				clientDBID[0] = dbid;
				ts3Functions.requestSetClientChannelGroup(serverConnectionHandlerID, channelGroup, targetChannel, clientDBID, 1, NULL);
				ts3Functions.printMessageToCurrentTab("Selected Client is now Operator in your target Channel.");
				break;
			}
			case MENU_ID_CLIENT_4: {
				channelGroup[0] = { 8 };
				uint64 dbid = 0;
				ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, selectedItemID, CLIENT_DATABASE_ID, &dbid);
				clientDBID[0] = dbid;
				ts3Functions.requestSetClientChannelGroup(serverConnectionHandlerID, channelGroup, targetChannel, clientDBID, 1, NULL);
				ts3Functions.printMessageToCurrentTab("Selected Client is now Guest in your target Channel.");
				break;
			}
			case MENU_ID_CLIENT_5: {
				channelGroup[0] = { 16 };
				uint64 dbid = 0;
				ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, selectedItemID, CLIENT_DATABASE_ID, &dbid);
				clientDBID[0] = dbid;
				ts3Functions.requestSetClientChannelGroup(serverConnectionHandlerID, channelGroup, targetChannel, clientDBID, 1, NULL);
				ts3Functions.printMessageToCurrentTab("Selected Client is now banned from your target Channel.");
				break;
			}
			case MENU_ID_CLIENT_6: {
				channelGroup[0] = { 17 };
				uint64 dbid = 0;
				ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, selectedItemID, CLIENT_DATABASE_ID, &dbid);
				clientDBID[0] = dbid;
				ts3Functions.requestSetClientChannelGroup(serverConnectionHandlerID, channelGroup, targetChannel, clientDBID, 1, NULL);
				ts3Functions.printMessageToCurrentTab("Selected Client can not send msgs in your target Channel anymore.");
				break;
			}
#endif
			default: {
				break;
			}
		}
	}
	else if (type == PLUGIN_MENU_TYPE_CHANNEL) {
		switch (menuItemID) {
			case MENU_ID_CHANNEL_1: {
				targetChannel[0] = { selectedItemID };
				char *test = new char[255];
				snprintf(test, sizeof(uint64), "%d", targetChannel[0]);
				ts3Functions.printMessageToCurrentTab("New target channel is:");
				ts3Functions.printMessageToCurrentTab(test);
				delete[] test;
				break;
			}
			default: {
				break;
			}
		}
	}
}