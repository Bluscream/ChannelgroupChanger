from ts3plugin import ts3plugin, PluginHost
import ts3, ts3defines, datetime


class channelGroupChanger(ts3plugin):
    name = "Channel Group Changer"
    apiVersion = 20
    requestAutoload = True
    version = "1.0"
    author = "Bluscream"
    description = "Change Channelgroup of clients that are not in the target channel.\n\nCheck out https://r4p3.net/forums/plugins.68/ for more plugins."
    offersConfigure = False
    commandKeyword = ""
    infoTitle = ""
    #menuItems = [(ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CHANNEL, 0, "Set as target channel", ""),(ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CLIENT, 0, "Change Channel Group", "")]
    # menuItems = [(ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CHANNEL, 0, "Set as target channel", ""), # GommeHD
    #                 (ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CLIENT, 0, "=== Channel Group ===", ""),
    #                 (ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CLIENT, 1, "Channel Admin", "ressources/channelGroupChanger/icons/GommeHD/Channel Admin.png"),
    #                 (ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CLIENT, 2, "Operator", "ressources/channelGroupChanger/icons/GommeHD/Channel Admin.png"),
    #                 (ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CLIENT, 3, "Guest", ""),
    #                 (ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CLIENT, 4, "Channel Bann", "ressources/channelGroupChanger/icons/GommeHD/Channel Bann.png"),
    #                 (ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CLIENT, 5, "Joinpower", ""),
    #                 (ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CLIENT, 6, "=== Channel Group ===", ""),
    #             ]
    menuItems = [(ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CHANNEL, 0, "Set as target channel", ""), #Rewinside
                (ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CLIENT, 0, "=== Channel Group ===", ""),
                (ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CLIENT, 1, "Channel Admin", ""),
                (ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CLIENT, 2, "Channel Mod", ""),
                (ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CLIENT, 3, "Channel User", ""),
                (ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CLIENT, 4, "Not Welcome", ""),
                (ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CLIENT, 5, "No Write", ""),
                (ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CLIENT, 6, "=== Channel Group ===", ""),
                ]
    hotkeys = []
    debug = False
    toggle = True
    channels = []


    def __init__(self):
        ts3.printMessageToCurrentTab('[{:%Y-%m-%d %H:%M:%S}]'.format(datetime.datetime.now())+" [color=orange]"+self.name+"[/color] Plugin for pyTSon by [url=https://github.com/Bluscream]Bluscream[/url] loaded.")

    def onMenuItemEvent(self, schid, atype, menuItemID, selectedItemID):
        if atype == ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CHANNEL:
            if menuItemID == 0:
                self.channels = [selectedItemID]
                ts3.printMessageToCurrentTab('[{:%Y-%m-%d %H:%M:%S}] '.format(datetime.datetime.now())+" Set target channel to [color=yellow]"+str(self.channels)+"[/color]")
            elif menuItemID == 1:
                self.ownchannels.extend([selectedItemID])
                ts3.printMessageToCurrentTab('[{:%Y-%m-%d %H:%M:%S}] '.format(datetime.datetime.now())+" Added [color=yellow]"+str(selectedItemID)+"[/color] to target channels: "+str(self.channels))
            elif menuItemID == 2:
                self.ownchannels.remove(selectedItemID)
                ts3.printMessageToCurrentTab('[{:%Y-%m-%d %H:%M:%S}] '.format(datetime.datetime.now())+" Removed [color=yellow]"+str(selectedItemID)+"[/color] from target channels: "+str(self.channels))
        elif atype == ts3defines.PluginMenuType.PLUGIN_MENU_TYPE_CLIENT:
            # if menuItemID == 1:
                # self.setClientChannelGroup(selectedItemID, 10, "Channel Admin")
            #     return
            # elif menuItemID == 2:
                # self.setClientChannelGroup(selectedItemID, 11, "Operator")
            #     return
            # elif menuItemID == 3:
                # self.setClientChannelGroup(selectedItemID, 9, "Guest")
            #     return
            # elif menuItemID == 4:
                # self.setClientChannelGroup(selectedItemID, 12, "Channel Bann")
            #     return
            # elif menuItemID == 5:
                # self.setClientChannelGroup(selectedItemID, 13, "Joinpower")
            #     return
            if menuItemID == 1:
                self.setClientChannelGroup(selectedItemID, 9, "Channel Admin")
                return
            elif menuItemID == 2:
                self.setClientChannelGroup(selectedItemID, 14, "Channel Mod")
                return
            elif menuItemID == 3:
                self.setClientChannelGroup(selectedItemID, 10, "User")
                return
            elif menuItemID == 4:
                self.setClientChannelGroup(selectedItemID, 11, "Not Welcome")
                return
            elif menuItemID == 5:
                self.setClientChannelGroup(selectedItemID, 13, "No Write")
                return

    def setClientChannelGroup(self, selectedItemID, channelGroupID, channelGroupName):
        schid = ts3.getCurrentServerConnectionHandlerID()
        (error, dbid) = ts3.getClientVariableAsUInt64(schid, selectedItemID, ts3defines.ClientPropertiesRare.CLIENT_DATABASE_ID)
        ts3.requestSetClientChannelGroup(schid, [channelGroupID], self.channels, [dbid])
        ts3.printMessageToCurrentTab("Client "+str(dbid)+" has now the group \""+channelGroupName+"\" in Channel #"+str(self.channel))
